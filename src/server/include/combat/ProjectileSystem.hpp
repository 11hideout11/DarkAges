#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <vector>

// [COMBAT_AGENT] Projectile system for DarkAges MMO
// Handles projectile spawning, movement, collision, and lifetime management

namespace DarkAges {

// ============================================================================
// Projectile Component
// ============================================================================

struct Projectile {
    EntityID owner{entt::null};           // Who fired this
    int16_t damage{0};                    // Base damage on hit
    float speed{30.0f};                   // Units per second
    uint32_t lifetimeMs{5000};            // Max lifetime before auto-destroy
    uint32_t spawnTimeMs{0};              // When this was spawned
    uint32_t lastUpdateTimeMs{0};         // Last movement update time
    bool isHoming{false};                 // Homes toward target
    EntityID homingTarget{entt::null};    // Target entity for homing
    float homingTurnRate{5.0f};           // Radians per second for homing
    bool hitDetected{false};              // Already hit something
    uint8_t pierceCount{0};              // How many targets can pass through (0 = stop on first hit)
    uint8_t pierceRemaining{0};          // Remaining pierces

    [[nodiscard]] bool isExpired(uint32_t currentTimeMs) const {
        return (currentTimeMs - spawnTimeMs) >= lifetimeMs;
    }
};

// ============================================================================
// Projectile Spawn Request (queued for next tick)
// ============================================================================

struct ProjectileSpawnRequest {
    EntityID owner{entt::null};
    Position spawnPos{};
    glm::vec3 direction{0.0f, 0.0f, 1.0f};
    int16_t damage{0};
    float speed{30.0f};
    uint32_t lifetimeMs{5000};
    uint8_t pierceCount{0};
    bool isHoming{false};
    EntityID homingTarget{entt::null};
};

// ============================================================================
// Hit Result for projectile collision
// ============================================================================

struct ProjectileHitResult {
    EntityID projectile{entt::null};
    EntityID target{entt::null};
    int16_t damage{0};
    Position hitPosition{};
};

// ============================================================================
// Spatial Hash Forward Declaration
// ============================================================================

class SpatialHash;

// ============================================================================
// Projectile System
// ============================================================================

class ProjectileSystem {
public:
    ProjectileSystem() = default;

    // Spawn a projectile entity with given parameters
    // Returns the entity ID of the spawned projectile
    EntityID spawnProjectile(Registry& registry,
                             EntityID owner,
                             const Position& spawnPos,
                             const glm::vec3& direction,
                             int16_t damage,
                             float speed = 30.0f,
                             uint32_t lifetimeMs = 5000,
                             uint8_t pierceCount = 0);

    // Queue a spawn request for next tick (thread-safe, deferred)
    void queueSpawn(ProjectileSpawnRequest request);

    // Update all projectiles - move, check collisions, expire
    // Returns list of hits that occurred this tick
    std::vector<ProjectileHitResult> update(Registry& registry,
                                             uint32_t currentTimeMs);

    // Set the spatial hash for collision detection
    void setSpatialHash(SpatialHash* hash) { spatialHash_ = hash; }

    // Set the combat system for applying damage
    void setDamageCallback(std::function<bool(Registry&, EntityID target, EntityID attacker,
                                               int16_t damage, uint32_t timeMs)> cb) {
        applyDamage_ = std::move(cb);
    }

    // Set the hit callback for notifications (e.g., send packets)
    void setHitCallback(std::function<void(const ProjectileHitResult&)> cb) {
        onHit_ = std::move(cb);
    }

    // Destroy a specific projectile
    void destroyProjectile(Registry& registry, EntityID projectile);

    // Destroy all projectiles owned by an entity
    void destroyProjectilesByOwner(Registry& registry, EntityID owner);

    // Get active projectile count
    [[nodiscard]] size_t getActiveCount(const Registry& registry) const;

private:
    // Move a single projectile based on its velocity
    void moveProjectile(Registry& registry, EntityID entity,
                        Projectile& proj, uint32_t currentTimeMs);

    // Check collision for a projectile against nearby entities
    // Returns true if a hit was detected
    bool checkCollision(Registry& registry, EntityID projectileEntity,
                        Projectile& proj, uint32_t currentTimeMs,
                        std::vector<ProjectileHitResult>& hits);

    // Process queued spawn requests
    void processSpawnQueue(Registry& registry, uint32_t currentTimeMs);

    SpatialHash* spatialHash_{nullptr};
    std::function<bool(Registry&, EntityID, EntityID, int16_t, uint32_t)> applyDamage_;
    std::function<void(const ProjectileHitResult&)> onHit_;

    std::vector<ProjectileSpawnRequest> spawnQueue_;
};

} // namespace DarkAges
