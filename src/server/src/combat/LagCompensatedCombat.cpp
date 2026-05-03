// [COMBAT_AGENT] Lag-compensated combat system implementation - Phase 3C
// Validates hits against historical positions using the lag compensator

#include "combat/LagCompensatedCombat.hpp"
#include "Constants.hpp"
#include <cmath>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

#include "combat/HitboxComponent.hpp"
#include "combat/HurtboxComponent.hpp"
#include "combat/CollisionLayerManager.hpp"

namespace DarkAges {

using namespace DarkAges::combat;

// ============================================================================
// Constructor
// ============================================================================

LagCompensatedCombat::LagCompensatedCombat(CombatSystem& combat, LagCompensator& lagComp)
    : combatSystem_(combat), lagCompensator_(lagComp) {}

// ============================================================================
// Main Attack Processing
// ============================================================================

std::vector<HitResult> LagCompensatedCombat::processAttackWithRewind(
    Registry& registry, const LagCompensatedAttack& attack) {
    
    std::vector<HitResult> results;
    bool createdHitbox = false;
    
    // Check if attacker can attack (cooldown, dead, etc.)
    if (!combatSystem_.canAttack(registry, attack.attacker, attack.serverTimestamp)) {
        HitResult result;
        result.hit = false;
        result.hitType = "cooldown";
        results.push_back(result);
        return results;
    }
    
    // Check if rewind is valid (latency within acceptable limits)
    bool useRewind = isRewindValid(attack.rttMs);
    
    // Calculate when the attack was initiated
    uint32_t attackTime = calculateAttackTime(attack.clientTimestamp, attack.rttMs);
    
    // Log if we're not using rewind due to high latency
    if (!useRewind) {
        // For high latency, fall back to current-time processing
        // This is a QoS degradation for high-ping players
        HitResult result = combatSystem_.processAttack(registry, attack.attacker, 
                                                       attack.input, attack.serverTimestamp);
        results.push_back(result);
        return results;
    }
    
    // Find valid targets at historical positions
    std::vector<EntityID> targets;

    // If a specific target is forced (e.g., via confirmed lock-on), validate and use it exclusively
    if (attack.input.targetEntity != 0 && attack.input.type != AttackInput::ABILITY) {
        EntityID forced = static_cast<EntityID>(attack.input.targetEntity);
        bool valid = false;
        switch (attack.input.type) {
            case AttackInput::MELEE:
                valid = validateMeleeHitAtTime(registry, attack.attacker, forced, attackTime);
                break;
            case AttackInput::RANGED:
                valid = validateRangedHitAtTime(registry, attack.attacker, forced,
                                                attack.input.aimDirection, attackTime);
                break;
            default:
                valid = false;
        }
        if (valid) {
            targets.push_back(forced);
        }
    } else {
        // Normal area-based target search
        targets = findHistoricalTargets(registry, attack.attacker, attackTime);
    }
    
    // Validate each potential hit at historical position
    for (EntityID target : targets) {
        bool validHit = false;
        
        switch (attack.input.type) {
            case AttackInput::MELEE:
                validHit = validateMeleeHitAtTime(registry, attack.attacker, target, attackTime);
                break;
                
            case AttackInput::RANGED:
                validHit = validateRangedHitAtTime(registry, attack.attacker, target,
                                                   attack.input.aimDirection, attackTime);
                break;
                
            case AttackInput::ABILITY:
                // Abilities have their own validation - pass through for now
                // In full implementation, ability system would handle this
                validHit = true;  // Assume valid for ability attacks
                break;
        }
        
        if (validHit) {
            // Apply damage at CURRENT time (damage happens now, not in past)
            // This is critical: we validate at historical time but apply at current time
            bool isCritical = false;
            int16_t baseDamage = (attack.input.type == AttackInput::MELEE) 
                ? combatSystem_.getConfig().baseMeleeDamage 
                : combatSystem_.getConfig().baseRangedDamage;
            
            // Note: We need to expose calculateDamage in CombatSystem or use applyDamage
            // For now, we'll use applyDamage with base damage
            int16_t damage = baseDamage;
            
            if (combatSystem_.applyDamage(registry, target, attack.attacker, 
                                         damage, attack.serverTimestamp)) {
                HitResult result;
                result.hit = true;
                result.target = target;
                result.damageDealt = damage;
                result.isCritical = isCritical;
                result.hitType = (attack.input.type == AttackInput::MELEE) ? "melee" : "ranged";
                
                // Record current position as hit location (where they are NOW)
                if (const Position* pos = registry.try_get<Position>(target)) {
                    result.hitLocation = *pos;
                }
                
                results.push_back(result);
                
                // Melee attacks only hit one target - stop after first hit
                if (attack.input.type == AttackInput::MELEE) {
                    break;
                }
            }
        }
    }
    
    // Update attacker's cooldown (record that they attacked)
    if (CombatState* combat = registry.try_get<CombatState>(attack.attacker)) {
        combat->lastAttackTime = attack.serverTimestamp;
    }
    
    // If no hits, record a miss
    if (results.empty()) {
        HitResult result;
        result.hit = false;
        result.hitType = "miss";
        results.push_back(result);
    }
    
    if (createdHitbox) {
        registry.remove<HitboxComponent>(attack.attacker);
    }
    return results;
}

// ============================================================================
// Hit Claim Validation
// ============================================================================

bool LagCompensatedCombat::validateHitClaim(Registry& registry, EntityID attacker,
                                           EntityID claimedTarget, const Position& claimedPosition,
                                           uint32_t attackTimestamp, uint32_t rttMs) {
    // Check if rewind is valid
    if (!isRewindValid(rttMs)) {
        return false;  // Can't validate with excessive latency
    }
    
    // Calculate attack time
    uint32_t attackTime = calculateAttackTime(attackTimestamp, rttMs);
    
    // Get target's historical position
    PositionHistoryEntry targetEntry;
    if (!lagCompensator_.getHistoricalPosition(claimedTarget, attackTime, targetEntry)) {
        return false;  // No history for that time
    }
    
    // Check if claimed position matches historical position (within tolerance)
    // Convert fixed-point to float for distance calculation
    float dx = (claimedPosition.x - targetEntry.position.x) * Constants::FIXED_TO_FLOAT;
    float dy = (claimedPosition.y - targetEntry.position.y) * Constants::FIXED_TO_FLOAT;
    float dz = (claimedPosition.z - targetEntry.position.z) * Constants::FIXED_TO_FLOAT;
    float distSq = dx*dx + dy*dy + dz*dz;
    
    // Allow 2m tolerance for:
    // - Movement during latency
    // - Position interpolation errors
    // - Client-side prediction differences
    constexpr float TOLERANCE_METERS = 2.0f;
    
    return distSq <= (TOLERANCE_METERS * TOLERANCE_METERS);
}

// ============================================================================
// Area-of-Effect Rewind
// ============================================================================

void LagCompensatedCombat::rewindEntitiesInArea(Registry& registry, const Position& center,
                                                float radius, uint32_t targetTimestamp,
                                                std::vector<EntityID>& outAffectedEntities,
                                                EntityID excludeEntity) {
    // Clear output vector
    outAffectedEntities.clear();
    
    // Find all entities with position and combat state
    auto view = registry.view<Position, CombatState>();
    
    view.each([&](EntityID entity, const Position& currentPos, const CombatState& combat) {
        // Skip excluded entity (e.g., the caster of an AOE attack)
        if (entity == excludeEntity) return;
        
        // Skip dead entities
        if (combat.isDead) return;
        
        // Get historical position
        PositionHistoryEntry histEntry;
        if (lagCompensator_.getHistoricalPosition(entity, targetTimestamp, histEntry)) {
            // Check if within radius at historical position
            float dx = (histEntry.position.x - center.x) * Constants::FIXED_TO_FLOAT;
            float dy = (histEntry.position.y - center.y) * Constants::FIXED_TO_FLOAT;
            float dz = (histEntry.position.z - center.z) * Constants::FIXED_TO_FLOAT;
            float distSq = dx*dx + dy*dy + dz*dz;
            
            if (distSq <= (radius * radius)) {
                outAffectedEntities.push_back(entity);
            }
        }
    });
}

// ============================================================================
// History Query
// ============================================================================

bool LagCompensatedCombat::hasHistoryForEntityAtTime(EntityID entity, uint32_t timestamp) const {
    PositionHistoryEntry entry;
    return lagCompensator_.getHistoricalPosition(entity, timestamp, entry);
}

// ============================================================================
// Private Helper Methods
// ============================================================================

uint32_t LagCompensatedCombat::calculateAttackTime(uint32_t clientTimestamp, uint32_t rttMs) const {
    (void)rttMs;  // one-way latency already subtracted by caller (processAttackInput)
    return clientTimestamp;
}

bool LagCompensatedCombat::isRewindValid(uint32_t rttMs) const {
    // Don't rewind more than MAX_REWIND_MS (500ms)
    // This prevents:
    // - Excessive memory usage for position history lookups
    // - Gameplay issues with very high latency players
    // - Potential exploits
    return (rttMs / 2) <= Constants::MAX_REWIND_MS;
}

std::vector<EntityID> LagCompensatedCombat::findHistoricalTargets(Registry& registry,
                                                                 EntityID attacker,
                                                                 uint32_t targetTimestamp) {
    std::vector<EntityID> targets;
    
    // Get attacker's position at attack time
    PositionHistoryEntry attackerEntry;
    if (!lagCompensator_.getHistoricalPosition(attacker, targetTimestamp, attackerEntry)) {
        return targets;  // Can't determine attack origin
    }
    
    // Find all potential targets (entities with CombatState)
    auto view = registry.view<Position, CombatState>();
    
    view.each([&](EntityID target, const Position& currentPos, const CombatState& combat) {
        // Skip self
        if (target == attacker) return;
        
        // Skip dead targets
        if (combat.isDead) return;
        
        // Get target's historical position
        PositionHistoryEntry targetEntry;
        if (!lagCompensator_.getHistoricalPosition(target, targetTimestamp, targetEntry)) {
            return;  // No history for this target
        }
        
        // Check range using AOI_RADIUS_NEAR (50m) as max search distance
        // This is an optimization - we don't need to check entities far away
        float dx = (targetEntry.position.x - attackerEntry.position.x) * Constants::FIXED_TO_FLOAT;
        float dy = (targetEntry.position.y - attackerEntry.position.y) * Constants::FIXED_TO_FLOAT;
        float dz = (targetEntry.position.z - attackerEntry.position.z) * Constants::FIXED_TO_FLOAT;
        float distSq = dx*dx + dy*dy + dz*dz;
        
        if (distSq <= (Constants::AOI_RADIUS_NEAR * Constants::AOI_RADIUS_NEAR)) {
            targets.push_back(target);
        }
    });
    
    return targets;
}

bool LagCompensatedCombat::validateMeleeHitAtTime(Registry& registry, EntityID attacker,
                                                 EntityID target, uint32_t targetTimestamp) {
    // Get historical positions for both attacker and target
    PositionHistoryEntry attackerEntry, targetEntry;
    if (!lagCompensator_.getHistoricalPosition(attacker, targetTimestamp, attackerEntry)) {
        return false;  // No attacker history
    }
    if (!lagCompensator_.getHistoricalPosition(target, targetTimestamp, targetEntry)) {
        return false;  // No target history
    }

    // Retrieve attacker's HitboxComponent (must be active)
    const HitboxComponent* hb = registry.try_get<HitboxComponent>(attacker);
    if (!hb || !hb->isActive) return false;

    // Retrieve target's HurtboxComponent (must be active)
    const HurtboxComponent* hurt = registry.try_get<HurtboxComponent>(target);
    if (!hurt || !hurt->isActive) return false;

    // Convert fixed-point positions to world coordinates
    auto toVec3 = [](const Position& p) -> glm::vec3 {
        return glm::vec3(
            p.x * Constants::FIXED_TO_FLOAT,
            p.y * Constants::FIXED_TO_FLOAT,
            p.z * Constants::FIXED_TO_FLOAT
        );
    };
    glm::vec3 hbWorldPos = toVec3(attackerEntry.position) + hb->offset;
    glm::vec3 hurtWorldPos = toVec3(targetEntry.position) + hurt->offset;

    // Layer-based collision check via global manager
    const CollisionLayerManager& layerMgr = getGlobalCollisionLayerManager();
    if (!checkHitboxHurtboxCollision(hbWorldPos, hb->radius, hb->height,
                                     hurtWorldPos, hurt->radius, hurt->height,
                                     hb->layer, hurt->layer, layerMgr)) {
        return false;
    }

    // Facing cone check (120 degree cone)
    const Rotation* rot = registry.try_get<Rotation>(attacker);
    if (!rot) return false;
    glm::vec3 aimDir = glm::vec3(std::sin(rot->yaw), 0.0f, std::cos(rot->yaw));
    glm::vec3 dirToTarget = glm::normalize(glm::vec3(
        targetEntry.position.x - attackerEntry.position.x,
        0.0f,
        targetEntry.position.z - attackerEntry.position.z));
    float dot = glm::dot(aimDir, dirToTarget);
    const float MELEE_HALF_ANGLE = combatSystem_.getConfig().meleeAngle / 2.0f;
    float halfConeRad = MELEE_HALF_ANGLE * static_cast<float>(M_PI) / 180.0f;
    float halfConeCos = std::cos(halfConeRad);
    if (dot < halfConeCos) return false;

    return true;
}

bool LagCompensatedCombat::validateRangedHitAtTime(Registry& registry, EntityID attacker,
                                                  EntityID target, const glm::vec3& aimDir,
                                                  uint32_t targetTimestamp) {
    // Get historical positions for both attacker and target
    PositionHistoryEntry attackerEntry, targetEntry;
    
    if (!lagCompensator_.getHistoricalPosition(attacker, targetTimestamp, attackerEntry)) {
        return false;  // No attacker history
    }
    
    if (!lagCompensator_.getHistoricalPosition(target, targetTimestamp, targetEntry)) {
        return false;  // No target history
    }
    
    // Convert to float vectors
    glm::vec3 attackerPos(
        attackerEntry.position.x * Constants::FIXED_TO_FLOAT,
        attackerEntry.position.y * Constants::FIXED_TO_FLOAT,
        attackerEntry.position.z * Constants::FIXED_TO_FLOAT
    );
    
    glm::vec3 targetPos(
        targetEntry.position.x * Constants::FIXED_TO_FLOAT,
        targetEntry.position.y * Constants::FIXED_TO_FLOAT,
        targetEntry.position.z * Constants::FIXED_TO_FLOAT
    );
    
    // Validate aim direction (should be normalized)
    glm::vec3 normalizedAim = aimDir;
    if (glm::length(normalizedAim) < 0.001f) {
        return false;  // Invalid aim direction
    }
    normalizedAim = glm::normalize(normalizedAim);
    
    // Ray-sphere intersection
    // Ray: P = attackerPos + t * aimDir
    // Sphere: |P - targetPos|^2 = r^2
    // Solve for t
    
    glm::vec3 oc = attackerPos - targetPos;
    float a = glm::dot(normalizedAim, normalizedAim);  // Should be 1.0 if normalized
    float b = 2.0f * glm::dot(oc, normalizedAim);
    float c = glm::dot(oc, oc) - (0.5f * 0.5f);  // 0.5m radius for player collision
    
    float discriminant = b*b - 4*a*c;
    
    if (discriminant < 0) {
        return false;  // No intersection - ray misses target sphere
    }
    
    // Calculate intersection distance
    float sqrtDisc = std::sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);
    
    // We need intersection in front of attacker (t > 0)
    // Either intersection point is valid
    if (t1 >= 0 || t2 >= 0) {
        return true;
    }
    
    // Both intersections are behind attacker
    return false;
}

glm::vec3 LagCompensatedCombat::getForwardVector(float yaw) const {
    // Yaw: 0 = +Z, PI/2 = +X
    return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw));
}

bool LagCompensatedCombat::isValidTarget(Registry& registry, EntityID attacker, EntityID target,
                                        uint32_t currentTimeMs) const {
    // Check if target exists
    if (!registry.valid(target)) {
        return false;
    }
    
    // Skip self
    if (target == attacker) {
        return false;
    }
    
    // Check combat state
    const CombatState* combat = registry.try_get<CombatState>(target);
    if (!combat) {
        return false;  // No combat component
    }
    
    // Skip dead targets
    if (combat->isDead) {
        return false;
    }
    
    return true;
}

} // namespace DarkAges
