// Tests for ProjectileSystem — spawning, movement, collision, lifetime, pierce

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/ProjectileSystem.hpp"
#include "physics/SpatialHash.hpp"
#include "ecs/CoreTypes.hpp"

using namespace DarkAges;

// ============================================================================
// Test Helpers
// ============================================================================

namespace {

Registry createTestRegistry() {
    return Registry{};
}

EntityID createTestPlayer(Registry& registry, float x = 0.0f, float z = 0.0f,
                          int16_t health = 10000, float radius = 0.5f) {
    auto entity = registry.create();
    Position pos;
    pos.x = static_cast<Constants::Fixed>(x * Constants::FLOAT_TO_FIXED);
    pos.z = static_cast<Constants::Fixed>(z * Constants::FLOAT_TO_FIXED);
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<CombatState>(entity);
    auto* combat = registry.try_get<CombatState>(entity);
    combat->health = health;
    combat->maxHealth = health;
    BoundingVolume bv;
    bv.radius = radius;
    registry.emplace<BoundingVolume>(entity, bv);
    registry.emplace<CollisionLayer>(entity, CollisionLayer::makePlayer());
    return entity;
}

} // namespace

// ============================================================================
// Spawn Tests
// ============================================================================

TEST_CASE("ProjectileSystem spawn", "[ProjectileSystem]") {
    auto registry = createTestRegistry();
    ProjectileSystem system;
    auto owner = createTestPlayer(registry);

    SECTION("spawn creates valid entity") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100);

        REQUIRE(proj != static_cast<EntityID>(entt::null));
        REQUIRE(registry.valid(proj));
    }

    SECTION("spawned entity has all required components") {
        Position spawnPos;
        spawnPos.x = 1000;  // 1.0f in fixed
        spawnPos.z = 2000;  // 2.0f in fixed
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100);

        REQUIRE(registry.all_of<Position>(proj));
        REQUIRE(registry.all_of<Velocity>(proj));
        REQUIRE(registry.all_of<Rotation>(proj));
        REQUIRE(registry.all_of<Projectile>(proj));
        REQUIRE(registry.all_of<ProjectileTag>(proj));
        REQUIRE(registry.all_of<CollisionLayer>(proj));
    }

    SECTION("projectile has correct owner and damage") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 250);

        auto* p = registry.try_get<Projectile>(proj);
        REQUIRE(p->owner == owner);
        REQUIRE(p->damage == 250);
    }

    SECTION("projectile velocity matches direction and speed") {
        Position spawnPos;
        glm::vec3 dir(1, 0, 0);  // Moving +X
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100, 20.0f);

        auto* vel = registry.try_get<Velocity>(proj);
        float vx = vel->dx * Constants::FIXED_TO_FLOAT;
        REQUIRE(vx == Catch::Approx(20.0f).margin(0.1f));
    }

    SECTION("projectile collision layer has owner reference") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100);

        auto* layer = registry.try_get<CollisionLayer>(proj);
        REQUIRE(layer->ownerEntity == owner);
        REQUIRE(layer->layer == CollisionLayerMask::PROJECTILE);
    }

    SECTION("zero direction gives zero velocity") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 0);  // No direction
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100);

        auto* vel = registry.try_get<Velocity>(proj);
        REQUIRE(vel->dx == 0);
        REQUIRE(vel->dz == 0);
    }
}

// ============================================================================
// Queue Spawn Tests
// ============================================================================

TEST_CASE("ProjectileSystem queued spawning", "[ProjectileSystem]") {
    auto registry = createTestRegistry();
    ProjectileSystem system;
    auto owner = createTestPlayer(registry);

    SECTION("queued spawns are created on update") {
        ProjectileSpawnRequest req;
        req.owner = owner;
        req.damage = 100;
        req.speed = 30.0f;
        req.direction = glm::vec3(0, 0, 1);

        system.queueSpawn(req);
        REQUIRE(system.getActiveCount(registry) == 0);

        system.update(registry, 0);
        REQUIRE(system.getActiveCount(registry) == 1);
    }

    SECTION("multiple queued spawns") {
        for (int i = 0; i < 5; ++i) {
            ProjectileSpawnRequest req;
            req.owner = owner;
            req.damage = 100;
            req.direction = glm::vec3(0, 0, 1);
            system.queueSpawn(req);
        }

        system.update(registry, 0);
        REQUIRE(system.getActiveCount(registry) == 5);
    }

    SECTION("queue is cleared after processing") {
        ProjectileSpawnRequest req;
        req.owner = owner;
        req.damage = 100;
        req.direction = glm::vec3(0, 0, 1);

        system.queueSpawn(req);
        system.update(registry, 0);
        system.update(registry, 16);  // Second tick should create nothing new
        REQUIRE(system.getActiveCount(registry) == 1);
    }
}

// ============================================================================
// Lifetime Tests
// ============================================================================

TEST_CASE("ProjectileSystem lifetime", "[ProjectileSystem]") {
    auto registry = createTestRegistry();
    ProjectileSystem system;
    auto owner = createTestPlayer(registry);

    SECTION("projectile is destroyed after lifetime expires") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100, 30.0f, 5000);

        auto* p = registry.try_get<Projectile>(proj);
        p->spawnTimeMs = 0;
        p->lastUpdateTimeMs = 0;

        // Still alive at 4999ms
        system.update(registry, 4999);
        REQUIRE(system.getActiveCount(registry) == 1);

        // Destroyed at 5000ms
        system.update(registry, 5000);
        REQUIRE(system.getActiveCount(registry) == 0);
    }

    SECTION("projectile survives within lifetime") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100, 30.0f, 10000);

        auto* p = registry.try_get<Projectile>(proj);
        p->spawnTimeMs = 0;
        p->lastUpdateTimeMs = 0;

        system.update(registry, 5000);
        REQUIRE(system.getActiveCount(registry) == 1);
    }
}

// ============================================================================
// Destroy Tests
// ============================================================================

TEST_CASE("ProjectileSystem destroy", "[ProjectileSystem]") {
    auto registry = createTestRegistry();
    ProjectileSystem system;
    auto owner = createTestPlayer(registry);

    SECTION("destroy specific projectile") {
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100);

        system.destroyProjectile(registry, proj);
        REQUIRE_FALSE(registry.valid(proj));
    }

    SECTION("destroy projectiles by owner") {
        auto owner2 = createTestPlayer(registry, 10.0f, 10.0f);
        Position spawnPos;
        glm::vec3 dir(0, 0, 1);

        system.spawnProjectile(registry, owner, spawnPos, dir, 100);
        system.spawnProjectile(registry, owner, spawnPos, dir, 100);
        system.spawnProjectile(registry, owner2, spawnPos, dir, 100);

        REQUIRE(system.getActiveCount(registry) == 3);

        system.destroyProjectilesByOwner(registry, owner);
        REQUIRE(system.getActiveCount(registry) == 1);
    }

    SECTION("destroy nonexistent projectile is safe") {
        system.destroyProjectile(registry, EntityID(999));
        // Should not crash
    }
}

// ============================================================================
// Movement Tests
// ============================================================================

TEST_CASE("ProjectileSystem movement", "[ProjectileSystem]") {
    auto registry = createTestRegistry();
    ProjectileSystem system;
    auto owner = createTestPlayer(registry);

    SECTION("projectile moves in correct direction") {
        // Spawn projectile far from any entities to avoid immediate collision
        Position spawnPos;
        spawnPos.x = static_cast<Constants::Fixed>(100.0f * Constants::FLOAT_TO_FIXED);
        spawnPos.z = static_cast<Constants::Fixed>(100.0f * Constants::FLOAT_TO_FIXED);
        glm::vec3 dir(0, 0, 1);  // +Z
        EntityID proj = system.spawnProjectile(registry, owner, spawnPos, dir, 100, 30.0f, 10000);

        auto* p = registry.try_get<Projectile>(proj);
        p->spawnTimeMs = 0;
        p->lastUpdateTimeMs = 0;

        // Simulate movement for ~1 second (60 ticks of 16.67ms)
        for (uint32_t t = 16; t <= 1000; t += 16) {
            system.update(registry, t);
        }

        if (registry.valid(proj)) {
            auto* pos = registry.try_get<Position>(proj);
            float z = pos->z * Constants::FIXED_TO_FLOAT;
            // Should have moved approximately 30 units in +Z from 100
            REQUIRE(z > 100.0f);
        }
    }
}

// ============================================================================
// Projectile Structure Tests
// ============================================================================

TEST_CASE("Projectile isExpired", "[ProjectileSystem]") {
    SECTION("not expired before lifetime") {
        Projectile p;
        p.spawnTimeMs = 1000;
        p.lifetimeMs = 5000;
        REQUIRE_FALSE(p.isExpired(5999));
    }

    SECTION("expired at lifetime") {
        Projectile p;
        p.spawnTimeMs = 1000;
        p.lifetimeMs = 5000;
        REQUIRE(p.isExpired(6000));
    }

    SECTION("expired after lifetime") {
        Projectile p;
        p.spawnTimeMs = 1000;
        p.lifetimeMs = 5000;
        REQUIRE(p.isExpired(7000));
    }
}
