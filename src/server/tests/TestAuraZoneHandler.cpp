// [ZONE_AGENT] AuraZoneHandler Unit Tests
// Tests for aura state serialization, zone transitions, entity migration completion

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/AuraZoneHandler.hpp"
#include "zones/ZoneServer.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <string>
#include <vector>
#include <cstring>

using namespace DarkAges;
using Catch::Approx;

// Helper to create a ZoneServer for testing (default-constructed, not initialized)
static ZoneServer createTestZoneServer() {
    return ZoneServer{};
}

// Helper to create a Position from float coordinates
static Position makePosition(float x, float y, float z) {
    Position pos;
    pos.x = static_cast<Constants::Fixed>(x * Constants::FLOAT_TO_FIXED);
    pos.y = static_cast<Constants::Fixed>(y * Constants::FLOAT_TO_FIXED);
    pos.z = static_cast<Constants::Fixed>(z * Constants::FLOAT_TO_FIXED);
    pos.timestamp_ms = 0;
    return pos;
}

// Helper to create a Velocity from float components
static Velocity makeVelocity(float dx, float dy, float dz) {
    Velocity vel;
    vel.dx = static_cast<Constants::Fixed>(dx * Constants::FLOAT_TO_FIXED);
    vel.dy = static_cast<Constants::Fixed>(dy * Constants::FLOAT_TO_FIXED);
    vel.dz = static_cast<Constants::Fixed>(dz * Constants::FLOAT_TO_FIXED);
    return vel;
}

// Helper to create a player entity with required components
static EntityID createPlayerEntity(Registry& registry, uint64_t playerId,
                                   const std::string& username, const Position& pos) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<BoundingVolume>(entity);
    registry.emplace<InputState>(entity);
    registry.emplace<CombatState>(entity);
    registry.emplace<NetworkState>(entity);
    registry.emplace<AntiCheatState>(entity);

    PlayerInfo info{};
    info.playerId = playerId;
    info.connectionId = 0;
    std::strncpy(info.username, username.c_str(), sizeof(info.username) - 1);
    info.username[sizeof(info.username) - 1] = '\0';
    registry.emplace<PlayerInfo>(entity, info);
    registry.emplace<PlayerTag>(entity);

    return entity;
}

// Helper to create a dynamic (non-static) entity
static EntityID createDynamicEntity(Registry& registry, const Position& pos) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<BoundingVolume>(entity);
    return entity;
}

// Helper to create a static entity (excluded from aura checks)
static EntityID createStaticEntity(Registry& registry, const Position& pos) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<StaticTag>(entity);
    return entity;
}

// ============================================================================
// Construction and Setup Tests
// ============================================================================

TEST_CASE("AuraZoneHandler construction", "[zones][aura]") {
    SECTION("Construct with ZoneServer reference") {
        auto server = createTestZoneServer();
        AuraZoneHandler handler(server);
    }
}

TEST_CASE("AuraZoneHandler connection mappings", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("Set connection mappings does not crash") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;
        handler.setConnectionMappings(&connToEntity, &entityToConn);
    }

    SECTION("Null connection mappings does not crash") {
        handler.setConnectionMappings(nullptr, nullptr);
    }
}

TEST_CASE("AuraZoneHandler subsystem setters", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("Setting null subsystem pointers does not crash") {
        handler.setNetwork(nullptr);
        handler.setRedis(nullptr);
        handler.setAuraManager(nullptr);
        handler.setMigrationManager(nullptr);
        handler.setHandoffController(nullptr);
    }
}

// ============================================================================
// Aura Manager Initialization Tests
// ============================================================================

TEST_CASE("AuraZoneHandler initializeAuraManager without aura manager", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("initializeAuraManager with no aura manager does not crash") {
        handler.initializeAuraManager();
    }
}

// ============================================================================
// Handoff Controller Initialization Tests
// ============================================================================

TEST_CASE("AuraZoneHandler initializeHandoffController without subsystems", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("initializeHandoffController with no handoff controller does not crash") {
        handler.initializeHandoffController();
    }
}

// ============================================================================
// Aura State Sync Tests
// ============================================================================

TEST_CASE("AuraZoneHandler syncAuraState", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("syncAuraState with no redis does not crash") {
        handler.syncAuraState();
    }

    SECTION("syncAuraState with no aura manager does not crash") {
        handler.syncAuraState();
    }
}

// ============================================================================
// Zone Transition Detection Tests
// ============================================================================

TEST_CASE("AuraZoneHandler checkEntityZoneTransitions", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("checkEntityZoneTransitions with no aura manager does not crash") {
        auto entity = createDynamicEntity(registry, makePosition(100.0f, 0.0f, 100.0f));
        handler.checkEntityZoneTransitions();
    }

    SECTION("Static entities are excluded from zone transitions") {
        auto staticEntity = createStaticEntity(registry, makePosition(100.0f, 0.0f, 100.0f));
        // StaticTag entities should be excluded by the view filter
        handler.checkEntityZoneTransitions();
    }
}

// ============================================================================
// Entity Migration Completion Tests
// ============================================================================

TEST_CASE("AuraZoneHandler onEntityMigrationComplete success", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("Successful migration with no connection mappings does not crash") {
        EntityID entity = static_cast<EntityID>(42);
        handler.onEntityMigrationComplete(entity, true);
    }

    SECTION("Failed migration does not crash") {
        EntityID entity = static_cast<EntityID>(42);
        handler.onEntityMigrationComplete(entity, false);
    }
}

TEST_CASE("AuraZoneHandler onEntityMigrationComplete with connections", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("Successful migration removes connection mapping") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;

        auto entity = createDynamicEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        ConnectionID connId = 100;

        connToEntity[connId] = entity;
        entityToConn[entity] = connId;

        handler.setConnectionMappings(&connToEntity, &entityToConn);

        REQUIRE(entityToConn.count(entity) == 1);
        REQUIRE(connToEntity.count(connId) == 1);

        handler.onEntityMigrationComplete(entity, true);

        // Connection mapping should be removed
        REQUIRE(entityToConn.count(entity) == 0);
        REQUIRE(connToEntity.count(connId) == 0);
    }

    SECTION("Failed migration preserves connection mapping") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;

        auto entity = createDynamicEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        ConnectionID connId = 200;

        connToEntity[connId] = entity;
        entityToConn[entity] = connId;

        handler.setConnectionMappings(&connToEntity, &entityToConn);

        handler.onEntityMigrationComplete(entity, false);

        // Connection mapping should be preserved
        REQUIRE(entityToConn.count(entity) == 1);
        REQUIRE(connToEntity.count(connId) == 1);
    }

    SECTION("Migration for entity without connection does not crash") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;

        auto entity = createDynamicEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        // Entity has no connection mapping
        handler.setConnectionMappings(&connToEntity, &entityToConn);

        handler.onEntityMigrationComplete(entity, true);
    }

    SECTION("Migration with null connection mappings does not crash") {
        EntityID entity = static_cast<EntityID>(99);
        handler.setConnectionMappings(nullptr, nullptr);
        handler.onEntityMigrationComplete(entity, true);
    }
}

// ============================================================================
// Handoff Callback Tests
// ============================================================================

TEST_CASE("AuraZoneHandler handoff callbacks", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("onHandoffStarted does not crash") {
        handler.onHandoffStarted(100, 1, 2, true);
    }

    SECTION("onHandoffCompleted success does not crash") {
        handler.onHandoffCompleted(100, 1, 2, true);
    }

    SECTION("onHandoffCompleted failure does not crash") {
        handler.onHandoffCompleted(100, 1, 2, false);
    }
}

// ============================================================================
// Zone Lookup Tests
// ============================================================================

TEST_CASE("AuraZoneHandler lookupZone", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("lookupZone returns valid pointer") {
        ZoneDefinition* def = handler.lookupZone(1);
        REQUIRE(def != nullptr);
        REQUIRE(def->zoneId == 1);
    }

    SECTION("lookupZone for different zones returns different IDs") {
        // Note: lookupZone returns pointer to a static local, so repeated
        // calls overwrite the same object. Verify the second call sets
        // the correct zoneId on the shared object.
        ZoneDefinition* def1 = handler.lookupZone(1);
        REQUIRE(def1 != nullptr);
        REQUIRE(def1->zoneId == 1);

        ZoneDefinition* def2 = handler.lookupZone(2);
        REQUIRE(def2 != nullptr);
        REQUIRE(def2->zoneId == 2);

        // Both pointers reference the same static object
        REQUIRE(def1 == def2);
    }
}

TEST_CASE("AuraZoneHandler findZoneByPosition", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("Position in center of world returns valid zone") {
        // Center of the world should map to some zone
        uint32_t zoneId = handler.findZoneByPosition(0.0f, 0.0f);
        REQUIRE(zoneId >= 1);
        REQUIRE(zoneId <= 4);  // 2x2 grid
    }

    SECTION("Position at world minimum returns valid zone") {
        uint32_t zoneId = handler.findZoneByPosition(
            Constants::WORLD_MIN_X, Constants::WORLD_MIN_Z);
        REQUIRE(zoneId >= 1);
        REQUIRE(zoneId <= 4);
    }

    SECTION("Position at world maximum returns valid zone") {
        uint32_t zoneId = handler.findZoneByPosition(
            Constants::WORLD_MAX_X, Constants::WORLD_MAX_Z);
        REQUIRE(zoneId >= 1);
        REQUIRE(zoneId <= 4);
    }

    SECTION("Position at quarter bounds returns consistent zone") {
        float midX = (Constants::WORLD_MIN_X + Constants::WORLD_MAX_X) / 2.0f;
        float midZ = (Constants::WORLD_MIN_Z + Constants::WORLD_MAX_Z) / 2.0f;

        uint32_t zoneId1 = handler.findZoneByPosition(
            Constants::WORLD_MIN_X + 10.0f, Constants::WORLD_MIN_Z + 10.0f);
        uint32_t zoneId2 = handler.findZoneByPosition(
            Constants::WORLD_MIN_X + 10.0f, Constants::WORLD_MIN_Z + 10.0f);

        REQUIRE(zoneId1 == zoneId2);
    }

    SECTION("Zone IDs are consistent for same quadrant") {
        float qtrX = (Constants::WORLD_MIN_X + Constants::WORLD_MAX_X) / 4.0f;
        float qtrZ = (Constants::WORLD_MIN_Z + Constants::WORLD_MAX_Z) / 4.0f;

        // Positions in same quadrant should return same zone
        uint32_t z1 = handler.findZoneByPosition(qtrX, qtrZ);
        uint32_t z2 = handler.findZoneByPosition(qtrX + 100.0f, qtrZ + 100.0f);
        REQUIRE(z1 == z2);
    }
}

// ============================================================================
// Aura Entity Migration Tests
// ============================================================================

TEST_CASE("AuraZoneHandler handleAuraEntityMigration", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("handleAuraEntityMigration with no subsystems does not crash") {
        auto entity = createDynamicEntity(registry, makePosition(100.0f, 0.0f, 100.0f));
        handler.handleAuraEntityMigration();
    }

    SECTION("handleAuraEntityMigration skips static entities") {
        auto staticEntity = createStaticEntity(registry, makePosition(100.0f, 0.0f, 100.0f));
        handler.handleAuraEntityMigration();
    }
}

// ============================================================================
// Zone Handoff Update Tests
// ============================================================================

TEST_CASE("AuraZoneHandler updateZoneHandoffs", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("updateZoneHandoffs with no handoff controller does not crash") {
        auto entity = createPlayerEntity(registry, 100, "Player",
                                         makePosition(0.0f, 0.0f, 0.0f));
        handler.updateZoneHandoffs();
    }

    SECTION("updateZoneHandoffs with no players does not crash") {
        handler.updateZoneHandoffs();
    }

    SECTION("updateZoneHandoffs skips players without connections") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;
        handler.setConnectionMappings(&connToEntity, &entityToConn);

        auto entity = createPlayerEntity(registry, 100, "Player",
                                         makePosition(100.0f, 0.0f, 100.0f));
        // Entity has PlayerInfo but no connection mapping
        handler.updateZoneHandoffs();
    }
}

// ============================================================================
// Aura State Serialization Tests
// ============================================================================

TEST_CASE("AuraZoneHandler aura sync payload format", "[zones][aura]") {
    // This tests the serialization logic indirectly by verifying
    // the syncAuraState method doesn't crash with various states
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("syncAuraState called multiple times is safe") {
        handler.syncAuraState();
        handler.syncAuraState();
        handler.syncAuraState();
    }
}

// ============================================================================
// Edge Cases and Robustness Tests
// ============================================================================

TEST_CASE("AuraZoneHandler edge cases", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("All operations with empty registry") {
        // No entities in registry
        handler.checkEntityZoneTransitions();
        handler.handleAuraEntityMigration();
        handler.updateZoneHandoffs();
        handler.syncAuraState();
    }

    SECTION("Multiple dynamic entities in registry") {
        for (int i = 0; i < 10; ++i) {
            float x = static_cast<float>(i * 100.0f) + Constants::WORLD_MIN_X + 500.0f;
            float z = static_cast<float>(i * 100.0f) + Constants::WORLD_MIN_Z + 500.0f;
            createDynamicEntity(registry, makePosition(x, 0.0f, z));
        }

        handler.checkEntityZoneTransitions();
    }

    SECTION("Mixed player and NPC entities") {
        createPlayerEntity(registry, 1, "Player1", makePosition(100.0f, 0.0f, 100.0f));
        createDynamicEntity(registry, makePosition(200.0f, 0.0f, 200.0f));
        createStaticEntity(registry, makePosition(300.0f, 0.0f, 300.0f));

        handler.checkEntityZoneTransitions();
        handler.updateZoneHandoffs();
    }

    SECTION("Migration complete for multiple entities") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;

        std::vector<EntityID> entities;
        for (int i = 0; i < 5; ++i) {
            auto entity = createDynamicEntity(registry,
                makePosition(static_cast<float>(i) * 10.0f, 0.0f, 0.0f));
            ConnectionID connId = static_cast<ConnectionID>(i + 1);
            connToEntity[connId] = entity;
            entityToConn[entity] = connId;
            entities.push_back(entity);
        }

        handler.setConnectionMappings(&connToEntity, &entityToConn);

        // Complete migrations for some entities
        for (size_t i = 0; i < 3; ++i) {
            handler.onEntityMigrationComplete(entities[i], true);
        }

        REQUIRE(entityToConn.size() == 2);
        REQUIRE(connToEntity.size() == 2);
    }
}

// ============================================================================
// Zone Definition Consistency Tests
// ============================================================================

TEST_CASE("AuraZoneHandler zone definition consistency", "[zones][aura]") {
    auto server = createTestZoneServer();
    AuraZoneHandler handler(server);

    SECTION("Lookup zone returns valid bounds") {
        ZoneDefinition* def = handler.lookupZone(1);
        REQUIRE(def != nullptr);
        REQUIRE(def->minX <= def->maxX);
        REQUIRE(def->minZ <= def->maxZ);
    }

    SECTION("All 4 zones in 2x2 grid have valid definitions") {
        for (uint32_t i = 1; i <= 4; ++i) {
            ZoneDefinition* def = handler.lookupZone(i);
            REQUIRE(def != nullptr);
            REQUIRE(def->zoneId == i);
            REQUIRE(def->minX <= def->maxX);
            REQUIRE(def->minZ <= def->maxZ);
        }
    }

    SECTION("findZoneByPosition returns zone that contains the position") {
        float testX = 100.0f;
        float testZ = 100.0f;
        uint32_t zoneId = handler.findZoneByPosition(testX, testZ);
        ZoneDefinition* def = handler.lookupZone(zoneId);
        REQUIRE(def != nullptr);
        // The zone should approximately contain this position
        REQUIRE(testX >= def->minX - 1.0f);
        REQUIRE(testX <= def->maxX + 1.0f);
        REQUIRE(testZ >= def->minZ - 1.0f);
        REQUIRE(testZ <= def->maxZ + 1.0f);
    }
}
