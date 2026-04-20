#include "combat/NPCAISystem.hpp"
#include "combat/CombatSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <cmath>
#include <cstdlib>

namespace DarkAges {

// ============================================================================
// NPC AI System Implementation
// ============================================================================

void NPCAISystem::update(Registry& registry, uint32_t currentTimeMs) {
    // Tick all NPCs with AI state
    auto view = registry.view<NPCAIState, NPCStats, Position, NPCTag>();

    // Use a fixed delta-time based on 60Hz tick (16.67ms)
    constexpr uint32_t dtMs = 17;

    view.each([&](EntityID npc, NPCAIState& ai, NPCStats& stats, Position& pos) {
        // Update behavior timer
        ai.behaviorTimerMs += dtMs;

        switch (ai.behavior) {
            case NPCBehavior::Idle:
                updateIdle(registry, npc, ai, pos, currentTimeMs, dtMs);
                break;
            case NPCBehavior::Wander:
                updateWander(registry, npc, ai, pos, currentTimeMs, dtMs);
                break;
            case NPCBehavior::Chase:
                updateChase(registry, npc, ai, pos, currentTimeMs, dtMs);
                break;
            case NPCBehavior::Attack:
                updateAttack(registry, npc, ai, pos, currentTimeMs, dtMs);
                break;
            case NPCBehavior::Flee:
                updateFlee(registry, npc, ai, pos, currentTimeMs, dtMs);
                break;
            case NPCBehavior::Dead:
                updateDead(registry, npc, ai, currentTimeMs, dtMs);
                break;
        }
    });
}

// ============================================================================
// Idle State — scan for players to aggro, or start wandering
// ============================================================================

void NPCAISystem::updateIdle(Registry& registry, EntityID npc, NPCAIState& ai,
                              const Position& pos, uint32_t currentTimeMs, uint32_t dtMs) {
    stopMovement(registry, npc);

    // Check health for flee
    if (const auto* combat = registry.try_get<CombatState>(npc)) {
        if (combat->maxHealth > 0) {
            float hpPercent = static_cast<float>(combat->health) / combat->maxHealth * 100.0f;
            if (hpPercent <= ai.fleeHealthPercent && !combat->isDead) {
                ai.behavior = NPCBehavior::Flee;
                ai.behaviorTimerMs = 0;
                return;
            }
        }
    }

    // Scan for nearby players
    EntityID target = findNearestPlayer(registry, pos, ai.aggroRange);
    if (target != entt::null) {
        ai.target = target;
        // Check if target is already in attack range
        const Position* targetPos = registry.try_get<Position>(target);
        if (targetPos) {
            float distSq = distanceSq(pos, *targetPos);
            float attackRangeSq = ai.attackRange * ai.attackRange;
            if (distSq <= attackRangeSq) {
                ai.behavior = NPCBehavior::Attack;
                ai.behaviorTimerMs = 0;
                return;
            }
        }
        ai.behavior = NPCBehavior::Chase;
        ai.behaviorTimerMs = 0;
        return;
    }

    // Wander if idle long enough
    if (ai.behaviorTimerMs >= ai.wanderCooldownMs) {
        ai.behavior = NPCBehavior::Wander;
        ai.behaviorTimerMs = 0;
    }
}

// ============================================================================
// Wander State — random movement within leash radius
// ============================================================================

void NPCAISystem::updateWander(Registry& registry, EntityID npc, NPCAIState& ai,
                                const Position& pos, uint32_t currentTimeMs, uint32_t dtMs) {
    // Check health for flee
    if (const auto* combat = registry.try_get<CombatState>(npc)) {
        if (combat->maxHealth > 0) {
            float hpPercent = static_cast<float>(combat->health) / combat->maxHealth * 100.0f;
            if (hpPercent <= ai.fleeHealthPercent && !combat->isDead) {
                ai.behavior = NPCBehavior::Flee;
                ai.behaviorTimerMs = 0;
                return;
            }
        }
    }

    // Scan for players while wandering
    EntityID target = findNearestPlayer(registry, pos, ai.aggroRange);
    if (target != entt::null) {
        ai.target = target;
        ai.behavior = NPCBehavior::Chase;
        ai.behaviorTimerMs = 0;
        return;
    }

    // Check if we've wandered too far from spawn — return
    float distFromSpawn = distanceSq(pos, ai.spawnPoint);
    float leashSq = ai.wanderRadius * ai.wanderRadius;

    if (distFromSpawn > leashSq) {
        // Move back toward spawn
        moveToward(registry, npc, ai.spawnPoint, NPC_WANDER_SPEED);
    } else if (ai.behaviorTimerMs >= 2000) {
        // Wander for 2 seconds then go back to idle
        // Pick a random direction and move briefly
        float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * 3.14159f;
        Position wanderTarget;
        wanderTarget.x = pos.x + static_cast<Constants::Fixed>(std::cos(angle) * 5.0f);
        wanderTarget.y = pos.y;
        wanderTarget.z = pos.z + static_cast<Constants::Fixed>(std::sin(angle) * 5.0f);

        moveToward(registry, npc, wanderTarget, NPC_WANDER_SPEED);

        if (ai.behaviorTimerMs >= 4000) {
            ai.behavior = NPCBehavior::Idle;
            ai.behaviorTimerMs = 0;
        }
    }
}

// ============================================================================
// Chase State — pursue target until in attack range
// ============================================================================

void NPCAISystem::updateChase(Registry& registry, EntityID npc, NPCAIState& ai,
                                const Position& pos, uint32_t currentTimeMs, uint32_t dtMs) {
    // Validate target
    if (!isTargetValid(registry, ai.target)) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    // Check leash — if too far from spawn, give up
    float distFromSpawn = distanceSq(pos, ai.spawnPoint);
    float leashSq = ai.leashRange * ai.leashRange;
    if (distFromSpawn > leashSq) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    // Check health for flee
    if (const auto* combat = registry.try_get<CombatState>(npc)) {
        if (combat->maxHealth > 0) {
            float hpPercent = static_cast<float>(combat->health) / combat->maxHealth * 100.0f;
            if (hpPercent <= ai.fleeHealthPercent && !combat->isDead) {
                ai.behavior = NPCBehavior::Flee;
                ai.behaviorTimerMs = 0;
                return;
            }
        }
    }

    const Position* targetPos = registry.try_get<Position>(ai.target);
    if (!targetPos) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    float distSq = distanceSq(pos, *targetPos);
    float attackRangeSq = ai.attackRange * ai.attackRange;

    if (distSq <= attackRangeSq) {
        // In attack range — switch to attack
        ai.behavior = NPCBehavior::Attack;
        ai.behaviorTimerMs = 0;
        stopMovement(registry, npc);
        return;
    }

    // Chase the target
    moveToward(registry, npc, *targetPos, NPC_MOVE_SPEED);
}

// ============================================================================
// Attack State — melee attack target, chase if out of range
// ============================================================================

void NPCAISystem::updateAttack(Registry& registry, EntityID npc, NPCAIState& ai,
                                const Position& pos, uint32_t currentTimeMs, uint32_t dtMs) {
    // Validate target
    if (!isTargetValid(registry, ai.target)) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    const Position* targetPos = registry.try_get<Position>(ai.target);
    if (!targetPos) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    float distSq = distanceSq(pos, *targetPos);
    float attackRangeSq = ai.attackRange * ai.attackRange;

    if (distSq > attackRangeSq * 1.5f) {
        // Target moved out of range — chase again
        ai.behavior = NPCBehavior::Chase;
        ai.behaviorTimerMs = 0;
        return;
    }

    // Check health for flee
    if (const auto* combat = registry.try_get<CombatState>(npc)) {
        if (combat->maxHealth > 0) {
            float hpPercent = static_cast<float>(combat->health) / combat->maxHealth * 100.0f;
            if (hpPercent <= ai.fleeHealthPercent && !combat->isDead) {
                ai.behavior = NPCBehavior::Flee;
                ai.behaviorTimerMs = 0;
                return;
            }
        }
    }

    // Face the target (set velocity to zero while attacking)
    stopMovement(registry, npc);

    // Check attack cooldown
    const NPCStats* stats = registry.try_get<NPCStats>(npc);
    if (!stats) return;

    if (currentTimeMs - ai.lastAttackTimeMs >= ai.attackCooldownMs) {
        performNPCAttack(registry, npc, ai.target, ai, *stats, currentTimeMs);
    }
}

// ============================================================================
// Flee State — run away from target
// ============================================================================

void NPCAISystem::updateFlee(Registry& registry, EntityID npc, NPCAIState& ai,
                              const Position& pos, uint32_t currentTimeMs, uint32_t dtMs) {
    // If we've fled far enough from the target, go back to idle
    if (ai.target == entt::null || !isTargetValid(registry, ai.target)) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    const Position* targetPos = registry.try_get<Position>(ai.target);
    if (!targetPos) {
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    float distSq = distanceSq(pos, *targetPos);
    float aggroRangeSq = ai.aggroRange * ai.aggroRange * 2.0f;  // Double aggro to stop fleeing

    if (distSq > aggroRangeSq) {
        // Far enough away — stop fleeing
        ai.target = entt::null;
        ai.behavior = NPCBehavior::Idle;
        ai.behaviorTimerMs = 0;
        return;
    }

    // Check if health recovered enough to re-engage
    if (const auto* combat = registry.try_get<CombatState>(npc)) {
        if (combat->maxHealth > 0) {
            float hpPercent = static_cast<float>(combat->health) / combat->maxHealth * 100.0f;
            if (hpPercent > ai.fleeHealthPercent * 2.0f) {
                // Health recovered — re-engage
                ai.behavior = NPCBehavior::Chase;
                ai.behaviorTimerMs = 0;
                return;
            }
        }
    }

    // Run away from threat
    moveAway(registry, npc, *targetPos, NPC_FLEE_SPEED);
}

// ============================================================================
// Dead State — waiting for respawn
// ============================================================================

void NPCAISystem::updateDead(Registry& registry, EntityID npc, NPCAIState& ai,
                              uint32_t currentTimeMs, uint32_t dtMs) {
    stopMovement(registry, npc);
    // Respawn is handled by the CombatEventHandler's respawn system
}

// ============================================================================
// Helper Methods
// ============================================================================

EntityID NPCAISystem::findNearestPlayer(Registry& registry, const Position& pos,
                                         float aggroRange) const {
    float nearestDistSq = aggroRange * aggroRange;
    EntityID nearest = entt::null;

    auto view = registry.view<PlayerTag, Position, CombatState>();
    view.each([&](EntityID player, Position& playerPos, CombatState& combat) {
        if (combat.isDead) return;

        float distSq = distanceSq(pos, playerPos);
        if (distSq < nearestDistSq) {
            nearestDistSq = distSq;
            nearest = player;
        }
    });

    return nearest;
}

bool NPCAISystem::isTargetValid(Registry& registry, EntityID target) const {
    if (target == entt::null) return false;
    if (!registry.valid(target)) return false;

    const auto* combat = registry.try_get<CombatState>(target);
    if (combat && combat->isDead) return false;

    return true;
}

float NPCAISystem::distanceSq(const Position& a, const Position& b) const {
    // Convert fixed-point to float for distance calculation
    float ax = static_cast<float>(a.x) * Constants::FIXED_TO_FLOAT;
    float az = static_cast<float>(a.z) * Constants::FIXED_TO_FLOAT;
    float bx = static_cast<float>(b.x) * Constants::FIXED_TO_FLOAT;
    float bz = static_cast<float>(b.z) * Constants::FIXED_TO_FLOAT;

    float dx = ax - bx;
    float dz = az - bz;
    return dx * dx + dz * dz;
}

void NPCAISystem::moveToward(Registry& registry, EntityID npc, const Position& target, float speed) {
    Position* pos = registry.try_get<Position>(npc);
    Velocity* vel = registry.try_get<Velocity>(npc);
    if (!pos || !vel) return;

    float dx = static_cast<float>(target.x - pos->x) * Constants::FIXED_TO_FLOAT;
    float dz = static_cast<float>(target.z - pos->z) * Constants::FIXED_TO_FLOAT;
    float dist = std::sqrt(dx * dx + dz * dz);

    if (dist < 0.1f) {
        vel->dx = 0;
        vel->dy = 0;
        vel->dz = 0;
        return;
    }

    // Normalize and scale by speed
    float speedFixed = speed * Constants::FLOAT_TO_FIXED;
    vel->dx = static_cast<Constants::Fixed>((dx / dist) * speedFixed);
    vel->dy = 0;
    vel->dz = static_cast<Constants::Fixed>((dz / dist) * speedFixed);
}

void NPCAISystem::moveAway(Registry& registry, EntityID npc, const Position& threat, float speed) {
    Position* pos = registry.try_get<Position>(npc);
    if (!pos) return;

    // Calculate direction away from threat
    Position awayTarget;
    awayTarget.x = pos->x + (pos->x - threat.x);
    awayTarget.y = pos->y;
    awayTarget.z = pos->z + (pos->z - threat.z);

    moveToward(registry, npc, awayTarget, speed);
}

void NPCAISystem::performNPCAttack(Registry& registry, EntityID npc, EntityID target,
                                     NPCAIState& ai, const NPCStats& stats,
                                     uint32_t currentTimeMs) {
    CombatState* targetCombat = registry.try_get<CombatState>(target);
    if (!targetCombat || targetCombat->isDead) return;

    // Calculate damage
    int16_t damage = static_cast<int16_t>(stats.baseDamage);

    // Apply damage
    targetCombat->health -= damage;
    if (targetCombat->health <= 0) {
        targetCombat->health = 0;
        targetCombat->isDead = true;
    }
    targetCombat->lastAttacker = npc;
    targetCombat->lastAttackTime = currentTimeMs;

    // Update attack timestamp
    ai.lastAttackTimeMs = currentTimeMs;

    // Fire damage callback
    if (damageCallback_) {
        damageCallback_(npc, target, damage);
    }
}

void NPCAISystem::stopMovement(Registry& registry, EntityID npc) {
    Velocity* vel = registry.try_get<Velocity>(npc);
    if (vel) {
        vel->dx = 0;
        vel->dy = 0;
        vel->dz = 0;
    }
}

} // namespace DarkAges