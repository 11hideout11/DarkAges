// [COMBAT_AGENT] Projectile system implementation
// Handles projectile spawning, movement, collision, and lifetime management

#include "combat/ProjectileSystem.hpp"
#include "physics/SpatialHash.hpp"
#include "ecs/CoreTypes.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace DarkAges {

// ============================================================================
// ProjectileSystem Implementation
// ============================================================================

EntityID ProjectileSystem::spawnProjectile(Registry& registry,
                                            EntityID owner,
                                            const Position& spawnPos,
                                            const glm::vec3& direction,
                                            int16_t damage,
                                            float speed,
                                            uint32_t lifetimeMs,
                                            uint8_t pierceCount) {
    auto entity = registry.create();

    // Set position at spawn point
    Position pos = spawnPos;
    registry.emplace<Position>(entity, pos);

    // Set velocity from direction * speed
    Velocity vel{};
    float len = glm::length(direction);
    if (len > 0.001f) {
        glm::vec3 normDir = direction / len;
        vel.dx = static_cast<Constants::Fixed>(normDir.x * speed * Constants::FLOAT_TO_FIXED);
        vel.dy = static_cast<Constants::Fixed>(normDir.y * speed * Constants::FLOAT_TO_FIXED);
        vel.dz = static_cast<Constants::Fixed>(normDir.z * speed * Constants::FLOAT_TO_FIXED);
    }
    registry.emplace<Velocity>(entity, vel);

    // Set rotation from direction
    Rotation rot{};
    if (len > 0.001f) {
        rot.yaw = std::atan2(direction.x, direction.z);
    }
    registry.emplace<Rotation>(entity, rot);

    // Add projectile component
    Projectile proj;
    proj.owner = owner;
    proj.damage = damage;
    proj.speed = speed;
    proj.lifetimeMs = lifetimeMs;
    proj.spawnTimeMs = 0;  // Will be set by caller
    proj.lastUpdateTimeMs = 0;
    proj.pierceCount = pierceCount;
    proj.pierceRemaining = pierceCount;
    registry.emplace<Projectile>(entity, proj);

    // Add tags and collision layer
    registry.emplace<ProjectileTag>(entity);
    registry.emplace<CollisionLayer>(entity, CollisionLayer::makeProjectile(owner));

    return entity;
}

void ProjectileSystem::queueSpawn(ProjectileSpawnRequest request) {
    spawnQueue_.push_back(std::move(request));
}

void ProjectileSystem::processSpawnQueue(Registry& registry, uint32_t currentTimeMs) {
    for (const auto& req : spawnQueue_) {
        EntityID proj = spawnProjectile(registry, req.owner, req.spawnPos,
                                         req.direction, req.damage, req.speed,
                                         req.lifetimeMs, req.pierceCount);
        if (proj != entt::null) {
            if (auto* p = registry.try_get<Projectile>(proj)) {
                p->spawnTimeMs = currentTimeMs;
                p->lastUpdateTimeMs = currentTimeMs;
                p->isHoming = req.isHoming;
                p->homingTarget = req.homingTarget;
            }
        }
    }
    spawnQueue_.clear();
}

std::vector<ProjectileHitResult> ProjectileSystem::update(Registry& registry,
                                                            uint32_t currentTimeMs) {
    std::vector<ProjectileHitResult> allHits;

    // Process queued spawns first
    processSpawnQueue(registry, currentTimeMs);

    // Update all projectile entities
    auto view = registry.view<Projectile, Position, Velocity>();
    std::vector<EntityID> toDestroy;

    view.each([&](EntityID entity, Projectile& proj, Position& pos, Velocity& vel) {
        // Check lifetime
        if (proj.isExpired(currentTimeMs)) {
            toDestroy.push_back(entity);
            return;
        }

        // Skip if already hit and no pierce remaining
        if (proj.hitDetected && proj.pierceRemaining == 0) {
            toDestroy.push_back(entity);
            return;
        }

        // Move projectile
        moveProjectile(registry, entity, proj, currentTimeMs);

        // Check collision
        if (checkCollision(registry, entity, proj, currentTimeMs, allHits)) {
            if (proj.pierceRemaining > 0) {
                proj.pierceRemaining--;
                proj.hitDetected = true;
            } else {
                toDestroy.push_back(entity);
            }
        }

        proj.lastUpdateTimeMs = currentTimeMs;
    });

    // Destroy expired/hit projectiles
    for (EntityID entity : toDestroy) {
        registry.destroy(entity);
    }

    return allHits;
}

void ProjectileSystem::moveProjectile(Registry& registry, EntityID entity,
                                        Projectile& proj, uint32_t currentTimeMs) {
    Position* pos = registry.try_get<Position>(entity);
    Velocity* vel = registry.try_get<Velocity>(entity);
    if (!pos || !vel) return;

    // Homing: adjust velocity toward target
    if (proj.isHoming && proj.homingTarget != entt::null) {
        const Position* targetPos = registry.try_get<Position>(proj.homingTarget);
        if (targetPos && registry.valid(proj.homingTarget)) {
            float dx = (targetPos->x - pos->x) * Constants::FIXED_TO_FLOAT;
            float dz = (targetPos->z - pos->z) * Constants::FIXED_TO_FLOAT;
            float dist = std::sqrt(dx * dx + dz * dz);

            if (dist > 0.1f) {
                glm::vec3 toTarget(dx / dist, 0.0f, dz / dist);
                glm::vec3 currentDir(
                    vel->dx * Constants::FIXED_TO_FLOAT,
                    0.0f,
                    vel->dz * Constants::FIXED_TO_FLOAT
                );
                float currentSpeed = glm::length(currentDir);
                if (currentSpeed > 0.001f) {
                    currentDir /= currentSpeed;

                    // Slerp toward target direction
                    float dot = glm::clamp(glm::dot(currentDir, toTarget), -1.0f, 1.0f);
                    float angle = std::acos(dot);

                    if (angle > 0.001f) {
                        float dtSec = (currentTimeMs - proj.lastUpdateTimeMs) / 1000.0f;
                        float maxTurn = proj.homingTurnRate * dtSec;
                        float t = std::min(maxTurn / angle, 1.0f);

                        glm::vec3 newDir = glm::normalize(
                            currentDir * (1.0f - t) + toTarget * t
                        );

                        vel->dx = static_cast<Constants::Fixed>(newDir.x * currentSpeed * Constants::FLOAT_TO_FIXED);
                        vel->dz = static_cast<Constants::Fixed>(newDir.z * currentSpeed * Constants::FLOAT_TO_FIXED);
                    }
                }
            }
        }
    }
}

bool ProjectileSystem::checkCollision(Registry& registry, EntityID projectileEntity,
                                        Projectile& proj, uint32_t currentTimeMs,
                                        std::vector<ProjectileHitResult>& hits) {
    Position* projPos = registry.try_get<Position>(projectileEntity);
    if (!projPos) return false;

    // Query spatial hash for nearby entities
    constexpr float COLLISION_RADIUS = 1.0f;  // Projectile collision radius

    if (spatialHash_) {
        auto nearby = spatialHash_->query(*projPos, COLLISION_RADIUS);

        for (EntityID candidate : nearby) {
            if (candidate == projectileEntity) continue;
            if (candidate == proj.owner) continue;  // Don't hit self

            // Skip non-collidable entities
            const CollisionLayer* candidateLayer = registry.try_get<CollisionLayer>(candidate);
            if (candidateLayer) {
                const CollisionLayer* projLayer = registry.try_get<CollisionLayer>(projectileEntity);
                if (projLayer && !((candidateLayer->layer & projLayer->collidesWith) != 0)) {
                    continue;  // Layer mask doesn't match
                }
            }

            // Skip dead entities
            const CombatState* combat = registry.try_get<CombatState>(candidate);
            if (combat && combat->isDead) continue;

            // Check actual distance (cylinder collision)
            const Position* candidatePos = registry.try_get<Position>(candidate);
            if (!candidatePos) continue;

            float dx = (candidatePos->x - projPos->x) * Constants::FIXED_TO_FLOAT;
            float dz = (candidatePos->z - projPos->z) * Constants::FIXED_TO_FLOAT;
            float distSq = dx * dx + dz * dz;

            const BoundingVolume* bv = registry.try_get<BoundingVolume>(candidate);
            float hitRadius = bv ? (bv->radius + COLLISION_RADIUS) : COLLISION_RADIUS;

            if (distSq <= hitRadius * hitRadius) {
                // Hit detected!
                ProjectileHitResult hitResult;
                hitResult.projectile = projectileEntity;
                hitResult.target = candidate;
                hitResult.damage = proj.damage;
                hitResult.hitPosition = *projPos;

                // Apply damage if callback is set
                if (applyDamage_) {
                    applyDamage_(registry, candidate, proj.owner, proj.damage, currentTimeMs);
                }

                // Notify hit callback
                if (onHit_) {
                    onHit_(hitResult);
                }

                hits.push_back(hitResult);
                return true;
            }
        }
    }

    return false;
}

void ProjectileSystem::destroyProjectile(Registry& registry, EntityID projectile) {
    if (registry.valid(projectile) && registry.all_of<ProjectileTag>(projectile)) {
        registry.destroy(projectile);
    }
}

void ProjectileSystem::destroyProjectilesByOwner(Registry& registry, EntityID owner) {
    auto view = registry.view<Projectile, ProjectileTag>();
    std::vector<EntityID> toDestroy;

    view.each([&](EntityID entity, const Projectile& proj) {
        if (proj.owner == owner) {
            toDestroy.push_back(entity);
        }
    });

    for (EntityID entity : toDestroy) {
        registry.destroy(entity);
    }
}

size_t ProjectileSystem::getActiveCount(const Registry& registry) const {
    size_t count = 0;
    auto view = registry.view<Projectile>();
    view.each([&](const Projectile&) { count++; });
    return count;
}

} // namespace DarkAges
