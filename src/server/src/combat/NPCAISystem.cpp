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

    // Track live NPC IDs for path state cleanup
    std::vector<uint32_t> liveNpcIds;
    liveNpcIds.reserve(64);

    view.each([&](EntityID npc, NPCAIState& ai, NPCStats& stats, Position& pos) {
        liveNpcIds.push_back(static_cast<uint32_t>(npc));

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

    // Clean up path states for destroyed NPCs
    if (!pathStates_.empty()) {
        for (auto it = pathStates_.begin(); it != pathStates_.end();) {
            bool found = false;
            for (uint32_t id : liveNpcIds) {
                if (id == it->first) { found = true; break; }
            }
            if (!found) {
                it = pathStates_.erase(it);
            } else {
                ++it;
            }
        }
    }
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
        if (navGrid_) {
            moveTowardWithPathfinding(registry, npc, ai.spawnPoint, NPC_WANDER_SPEED, currentTimeMs);
        } else {
            moveToward(registry, npc, ai.spawnPoint, NPC_WANDER_SPEED);
        }
    } else if (ai.behaviorTimerMs >= 2000) {
        // Wander for 2 seconds then go back to idle
        // Pick a random direction and move briefly
        float angle = static_cast<float>(std::rand()) / RAND_MAX * 2.0f * 3.14159f;
        Position wanderTarget;
        wanderTarget.x = pos.x + static_cast<Constants::Fixed>(std::cos(angle) * 5.0f);
        wanderTarget.y = pos.y;
        wanderTarget.z = pos.z + static_cast<Constants::Fixed>(std::sin(angle) * 5.0f);

        if (navGrid_) {
            moveTowardWithPathfinding(registry, npc, wanderTarget, NPC_WANDER_SPEED, currentTimeMs);
        } else {
            moveToward(registry, npc, wanderTarget, NPC_WANDER_SPEED);
        }

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
    float preferredRangeSq = ai.preferredRange * ai.preferredRange;

    // Ranged/caster: stop at preferred range, not melee range
    float engageRangeSq = (ai.preferredRange > ai.attackRange)
        ? preferredRangeSq
        : attackRangeSq;

    if (distSq <= engageRangeSq) {
        // In engagement range — switch to attack
        ai.behavior = NPCBehavior::Attack;
        ai.behaviorTimerMs = 0;
        stopMovement(registry, npc);
        return;
    }

    // Chase the target
    if (navGrid_) {
        moveTowardWithPathfinding(registry, npc, *targetPos, NPC_MOVE_SPEED, currentTimeMs);
    } else {
        moveToward(registry, npc, *targetPos, NPC_MOVE_SPEED);
    }
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

    // Out of attack range — chase again
    if (distSq > attackRangeSq * 1.5f) {
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

    // Ranged/caster: maintain distance — retreat if target too close
    if (ai.retreatRange > 0.0f && distSq < ai.retreatRange * ai.retreatRange) {
        moveAway(registry, npc, *targetPos, NPC_WANDER_SPEED);
    } else {
        // Face the target (stop movement)
        stopMovement(registry, npc);
    }

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

    // Run away from threat — flee toward spawn point
    if (navGrid_) {
        moveAwayWithPathfinding(registry, npc, *targetPos, ai.spawnPoint,
                                NPC_FLEE_SPEED, currentTimeMs);
    } else {
        moveAway(registry, npc, *targetPos, NPC_FLEE_SPEED);
    }
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

    // Calculate damage with archetype scaling
    int16_t damage = static_cast<int16_t>(stats.baseDamage);

    // Archetype-specific behavior
    switch (stats.archetype) {
        case NPCArchetype::Melee:
            // Standard melee attack — no scaling
            break;

        case NPCArchetype::Ranged:
            // Ranged attack — slightly less damage than melee
            damage = static_cast<int16_t>(damage * 0.8f);
            break;

        case NPCArchetype::Caster: {
            // Caster: occasionally use an "ability" (bonus damage + potential debuff)
            bool useAbility = (currentTimeMs - ai.lastAbilityTimeMs >= 3000) &&
                             (static_cast<float>(std::rand()) / RAND_MAX < 0.3f);
            if (useAbility) {
                damage = static_cast<int16_t>(damage * 1.5f);
                ai.lastAbilityTimeMs = currentTimeMs;

                // Apply a slow debuff via status effects if available
                if (combatSystem_) {
                    // Use combat system to apply a status effect via the damage callback
                    // (ability-like behavior handled through damage)
                }
            }
            break;
        }

        case NPCArchetype::Boss: {
            // Boss scaling applied to base damage
            damage = static_cast<int16_t>(damage * 1.5f);

            // Use BossProfile for phase-based ability logic if present
            const BossProfile* bossProfile = registry.try_get<BossProfile>(npc);
            if (bossProfile && bossProfile->phaseCount > 0) {
                uint32_t phaseIdx = bossProfile->currentPhase;
                if (phaseIdx < bossProfile->phaseCount && bossProfile->phaseAbilityCount[phaseIdx] > 0) {
                    uint32_t abilityId = bossProfile->phaseAbilityIds[phaseIdx][0];
                    const AbilityDefinition* abilityDef = abilitySystem_->getAbility(abilityId);
                    if (abilityDef) {
                        // Compute ability damage from manaCost (damage = manaCost * 10)
                        damage = static_cast<int16_t>(abilityDef->manaCost * 10);
                        // Apply phase damage multiplier
                        damage = static_cast<int16_t>(damage * bossProfile->phaseDamageMult[phaseIdx]);
                        // Cooldown check before using ability
                        if (currentTimeMs - ai.lastAbilityTimeMs >= abilityDef->cooldownMs) {
                            ai.lastAbilityTimeMs = currentTimeMs;
                            // TODO: trigger telegraph & visual effects via NetworkManager
                        }
                        // Done with boss ability handling — skip generic logic
                        break;
                    }
                }
            }

            // Fallback: random chance ability if no BossProfile or ability data missing
            bool useAbility = (currentTimeMs - ai.lastAbilityTimeMs >= 2000) &&
                             (static_cast<float>(std::rand()) / RAND_MAX < 0.4f);
            if (useAbility) {
                damage = static_cast<int16_t>(damage * 2.0f);
                ai.lastAbilityTimeMs = currentTimeMs;
            }
            break;
        }
    }

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

// ============================================================================
// Pathfinding Methods
// ============================================================================

void NPCAISystem::clearPaths() {
    pathStates_.clear();
}

void NPCAISystem::clearPath(EntityID npc) {
    pathStates_.erase(static_cast<uint32_t>(npc));
}

void NPCAISystem::posToFloat(const Position& pos, float& x, float& z) {
    x = static_cast<float>(pos.x) * Constants::FIXED_TO_FLOAT;
    z = static_cast<float>(pos.z) * Constants::FIXED_TO_FLOAT;
}

bool NPCAISystem::hasLineOfSight(const Position& a, const Position& b) const {
    if (!navGrid_) return true;  // No grid = no obstacles = always LOS

    float ax, az, bx, bz;
    posToFloat(a, ax, az);
    posToFloat(b, bx, bz);

    return navGrid_->hasLineOfSight(ax, az, bx, bz);
}

const Path& NPCAISystem::getOrCalcPath(EntityID npc, const Position& from,
                                        float toX, float toZ, uint32_t currentTimeMs,
                                        uint32_t targetEntity) {
    static Path emptyPath;  // Fallback when no grid

    if (!navGrid_) {
        return emptyPath;
    }

    uint32_t npcId = static_cast<uint32_t>(npc);
    auto& state = pathStates_[npcId];

    // Check if we need to recalculate
    float fromX, fromZ;
    posToFloat(from, fromX, fromZ);

    float targetMoved = (toX - state.targetX) * (toX - state.targetX) +
                        (toZ - state.targetZ) * (toZ - state.targetZ);
    bool targetFar = targetMoved > (TARGET_MOVED_THRESHOLD * TARGET_MOVED_THRESHOLD);

    uint32_t elapsed = currentTimeMs - state.lastPathCalcMs;
    bool timeExpired = elapsed >= PATH_RECALC_MS;

    if (state.currentPath.valid && !timeExpired && !targetFar) {
        return state.currentPath;
    }

    // Recalculate path
    state.currentPath = navGrid_->findPath(fromX, fromZ, toX, toZ);
    state.lastPathCalcMs = currentTimeMs;
    state.targetX = toX;
    state.targetZ = toZ;
    state.pathTargetEntity = targetEntity;

    return state.currentPath;
}

bool NPCAISystem::followPath(Registry& registry, EntityID npc,
                              NPCPathState& pathState, float speed,
                              uint32_t currentTimeMs) {
    if (!pathState.currentPath.valid || pathState.currentPath.empty()) {
        return false;
    }

    // Get NPC position
    Position* pos = registry.try_get<Position>(npc);
    if (!pos) return false;

    float npcX, npcZ;
    posToFloat(*pos, npcX, npcZ);

    // Get next waypoint
    Waypoint wp = pathState.currentPath.waypoints.front();

    // Check if we've reached this waypoint
    float dx = wp.x - npcX;
    float dz = wp.z - npcZ;
    float distSq = dx * dx + dz * dz;

    if (distSq < WAYPOINT_REACH_DIST * WAYPOINT_REACH_DIST) {
        // Reached this waypoint, move to next
        pathState.currentPath.waypoints.erase(pathState.currentPath.waypoints.begin());

        if (pathState.currentPath.waypoints.empty()) {
            // Path complete
            stopMovement(registry, npc);
            return false;
        }

        // Get next waypoint
        wp = pathState.currentPath.waypoints.front();
        dx = wp.x - npcX;
        dz = wp.z - npcZ;
        distSq = dx * dx + dz * dz;
    }

    if (distSq < 0.01f) {
        stopMovement(registry, npc);
        return true;
    }

    // Move toward waypoint
    Velocity* vel = registry.try_get<Velocity>(npc);
    if (!vel) return false;

    float dist = std::sqrt(distSq);
    float speedFixed = speed * Constants::FLOAT_TO_FIXED;
    vel->dx = static_cast<Constants::Fixed>((dx / dist) * speedFixed);
    vel->dy = 0;
    vel->dz = static_cast<Constants::Fixed>((dz / dist) * speedFixed);

    return true;
}

void NPCAISystem::moveTowardWithPathfinding(Registry& registry, EntityID npc,
                                             const Position& target, float speed,
                                             uint32_t currentTimeMs) {
    if (!navGrid_) {
        // No grid — fall back to direct movement
        moveToward(registry, npc, target, speed);
        return;
    }

    // Check line of sight first — if clear, use direct movement
    Position* pos = registry.try_get<Position>(npc);
    if (!pos) return;

    if (hasLineOfSight(*pos, target)) {
        moveToward(registry, npc, target, speed);
        // Clear path cache since we're going direct
        clearPath(npc);
        return;
    }

    // No line of sight — use pathfinding
    float targetX, targetZ;
    posToFloat(target, targetX, targetZ);

    uint32_t npcId = static_cast<uint32_t>(npc);
    const Path& path = getOrCalcPath(npc, *pos, targetX, targetZ,
                                      currentTimeMs, npcId);

    auto& state = pathStates_[npcId];

    if (!path.valid || path.waypoints.empty()) {
        // No path found — try direct movement as fallback
        moveToward(registry, npc, target, speed);
        return;
    }

    // Follow the path
    if (!followPath(registry, npc, state, speed, currentTimeMs)) {
        // Path complete or failed — move directly
        moveToward(registry, npc, target, speed);
    }
}

void NPCAISystem::moveAwayWithPathfinding(Registry& registry, EntityID npc,
                                           const Position& threat,
                                           const Position& fleeGoal,
                                           float speed, uint32_t currentTimeMs) {
    if (!navGrid_) {
        moveAway(registry, npc, threat, speed);
        return;
    }

    // Flee toward the goal (usually spawn point) using pathfinding
    Position* pos = registry.try_get<Position>(npc);
    if (!pos) return;

    if (hasLineOfSight(*pos, fleeGoal)) {
        moveToward(registry, npc, fleeGoal, speed);
        clearPath(npc);
        return;
    }

    float goalX, goalZ;
    posToFloat(fleeGoal, goalX, goalZ);

    uint32_t npcId = static_cast<uint32_t>(npc);
    const Path& path = getOrCalcPath(npc, *pos, goalX, goalZ,
                                      currentTimeMs, 0);

    auto& state = pathStates_[npcId];

    if (!path.valid || path.waypoints.empty()) {
        moveAway(registry, npc, threat, speed);
        return;
    }

    if (!followPath(registry, npc, state, speed, currentTimeMs)) {
        moveAway(registry, npc, threat, speed);
    }
}

} // namespace DarkAges
