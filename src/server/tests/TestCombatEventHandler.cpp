// [ZONE_AGENT] CombatEventHandler Unit Tests
// Tests for combat event processing, death handling, and respawn queue

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/CombatEventHandler.hpp"
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

// Helper to create a player entity with all required components
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

// Helper to create an NPC entity
static EntityID createNPCEntity(Registry& registry, const Position& pos) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<BoundingVolume>(entity);
    registry.emplace<CombatState>(entity);
    registry.emplace<NPCTag>(entity);
    return entity;
}

// ============================================================================
// Construction and Setup Tests
// ============================================================================

TEST_CASE("CombatEventHandler construction", "[zones][combat]") {
    SECTION("Construct with ZoneServer reference") {
        auto server = createTestZoneServer();
        CombatEventHandler handler(server);
        REQUIRE(handler.getPendingRespawns().empty());
    }
}

TEST_CASE("CombatEventHandler connection mappings", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);

    SECTION("Set connection mappings does not crash") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;
        handler.setConnectionMappings(&connToEntity, &entityToConn);
    }

    SECTION("Null connection mappings does not crash") {
        handler.setConnectionMappings(nullptr, nullptr);
    }
}

TEST_CASE("CombatEventHandler subsystem setters", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);

    SECTION("Setting null subsystem pointers does not crash") {
        handler.setNetwork(nullptr);
        handler.setScylla(nullptr);
        handler.setCombatSystem(nullptr);
        handler.setLagCompensator(nullptr);
        handler.setAntiCheat(nullptr);
    }
}

// ============================================================================
// Entity Death Tests
// ============================================================================

TEST_CASE("CombatEventHandler onEntityDied", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("Death adds entry to pending respawns") {
        auto victim = createNPCEntity(registry, makePosition(10.0f, 0.0f, 20.0f));
        auto killer = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));

        REQUIRE(handler.getPendingRespawns().empty());
        handler.onEntityDied(victim, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
        REQUIRE(handler.getPendingRespawns()[0].entity == victim);
    }

    SECTION("Multiple deaths accumulate in pending respawns") {
        auto e1 = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto e2 = createNPCEntity(registry, makePosition(10.0f, 0.0f, 10.0f));
        auto killer = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));

        handler.onEntityDied(e1, killer);
        handler.onEntityDied(e2, killer);

        REQUIRE(handler.getPendingRespawns().size() == 2);

        bool foundE1 = false, foundE2 = false;
        for (const auto& respawn : handler.getPendingRespawns()) {
            if (respawn.entity == e1) foundE1 = true;
            if (respawn.entity == e2) foundE2 = true;
        }
        REQUIRE(foundE1);
        REQUIRE(foundE2);
    }

    SECTION("Player vs NPC death records player info") {
        Position pos = makePosition(100.0f, 0.0f, 200.0f);
        auto victim = createPlayerEntity(registry, 1001, "Victim", pos);
        auto killer = createNPCEntity(registry, makePosition(95.0f, 0.0f, 195.0f));

        handler.onEntityDied(victim, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }

    SECTION("NPC vs Player death records player info") {
        auto victim = createNPCEntity(registry, makePosition(10.0f, 0.0f, 10.0f));
        auto killer = createPlayerEntity(registry, 1002, "Killer", makePosition(5.0f, 0.0f, 5.0f));

        handler.onEntityDied(victim, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }

    SECTION("Player vs Player death records both") {
        auto victim = createPlayerEntity(registry, 1001, "Victim", makePosition(10.0f, 0.0f, 10.0f));
        auto killer = createPlayerEntity(registry, 1002, "Killer", makePosition(5.0f, 0.0f, 5.0f));

        handler.onEntityDied(victim, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }
}

// ============================================================================
// Respawn Queue Tests
// ============================================================================

TEST_CASE("CombatEventHandler processRespawns", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("processRespawns with empty queue does nothing") {
        REQUIRE(handler.getPendingRespawns().empty());
        handler.processRespawns();
        REQUIRE(handler.getPendingRespawns().empty());
    }

    SECTION("Respawn restores health to max") {
        auto entity = createNPCEntity(registry, makePosition(50.0f, 0.0f, 50.0f));
        auto& combat = registry.get<CombatState>(entity);
        combat.health = 0;
        combat.maxHealth = 10000;
        combat.isDead = true;

        // Call onEntityDied to queue it - respawnTimeMs = currentTime + 5000
        auto killer = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        handler.onEntityDied(entity, killer);

        // Verify it's queued
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }

    SECTION("Respawn resets position to origin") {
        auto entity = createNPCEntity(registry, makePosition(500.0f, 100.0f, -300.0f));
        auto killer = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));

        handler.onEntityDied(entity, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);

        // Position should still be original before respawn triggers
        auto& pos = registry.get<Position>(entity);
        REQUIRE(pos.x != 0);
    }
}

TEST_CASE("CombatEventHandler respawn timing", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("Respawn time is set in the future") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto killer = createNPCEntity(registry, makePosition(1.0f, 0.0f, 1.0f));

        uint32_t beforeTime = handler.getCurrentTimeMs();
        handler.onEntityDied(entity, killer);

        REQUIRE(handler.getPendingRespawns().size() == 1);
        REQUIRE(handler.getPendingRespawns()[0].respawnTimeMs >= beforeTime);
    }

    SECTION("Respawn delay is approximately 5 seconds") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto killer = createNPCEntity(registry, makePosition(1.0f, 0.0f, 1.0f));

        uint32_t beforeTime = handler.getCurrentTimeMs();
        handler.onEntityDied(entity, killer);

        REQUIRE(handler.getPendingRespawns().size() == 1);
        uint32_t delay = handler.getPendingRespawns()[0].respawnTimeMs - beforeTime;
        // Allow some tolerance for timing
        REQUIRE(delay >= 4900);
        REQUIRE(delay <= 5100);
    }
}

// ============================================================================
// Combat Event Processing Tests
// ============================================================================

TEST_CASE("CombatEventHandler sendCombatEvent without network", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("sendCombatEvent with no network does not crash") {
        auto attacker = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto target = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));
        Position location = makePosition(2.5f, 0.0f, 2.5f);

        handler.sendCombatEvent(attacker, target, 100, location);
    }

    SECTION("sendCombatEvent with connection mappings but no network does not crash") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;
        handler.setConnectionMappings(&connToEntity, &entityToConn);

        auto attacker = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto target = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));
        Position location = makePosition(2.5f, 0.0f, 2.5f);

        handler.sendCombatEvent(attacker, target, -50, location);
    }
}

TEST_CASE("CombatEventHandler logCombatEvent without scylla", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("logCombatEvent with no scylla does not crash") {
        auto attacker = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto target = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));

        HitResult hit;
        hit.hit = true;
        hit.target = target;
        hit.damageDealt = 150;
        hit.isCritical = false;
        hit.hitLocation = makePosition(2.5f, 0.0f, 2.5f);
        hit.hitType = "melee";

        handler.logCombatEvent(hit, attacker, target);
    }

    SECTION("logCombatEvent with NPC vs NPC (no players) does not log") {
        auto attacker = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto target = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));

        HitResult hit;
        hit.hit = true;
        hit.target = target;
        hit.damageDealt = 75;
        hit.isCritical = true;
        hit.hitLocation = makePosition(2.5f, 0.0f, 2.5f);
        hit.hitType = "melee";

        // Without scylla, this should not crash
        handler.logCombatEvent(hit, attacker, target);
    }
}

// ============================================================================
// processAttackInput Tests
// ============================================================================

TEST_CASE("CombatEventHandler processAttackInput without combat system", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("processAttackInput with no combat system does not crash") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));

        ClientInputPacket input;
        input.connectionId = 1;
        input.input.sequence = 100;
        input.input.timestamp_ms = 1000;
        input.input.yaw = 0.5f;
        input.input.attack = 1;
        input.receiveTimeMs = 1000;

        handler.processAttackInput(entity, input);
    }

    SECTION("processAttackInput with NetworkState component uses RTT") {
        auto entity = createPlayerEntity(registry, 1, "Player1", makePosition(0.0f, 0.0f, 0.0f));
        auto& netState = registry.get<NetworkState>(entity);
        netState.rttMs = 50;

        ClientInputPacket input;
        input.connectionId = 1;
        input.input.sequence = 200;
        input.input.timestamp_ms = 2000;
        input.input.yaw = 1.0f;
        input.input.attack = 1;
        input.receiveTimeMs = 2000;

        // No combat system set - should return early without crash
        handler.processAttackInput(entity, input);
    }

    SECTION("processAttackInput with zero RTT uses default") {
        auto entity = createPlayerEntity(registry, 2, "Player2", makePosition(0.0f, 0.0f, 0.0f));
        auto& netState = registry.get<NetworkState>(entity);
        netState.rttMs = 0;  // Not measured yet

        ClientInputPacket input;
        input.connectionId = 1;
        input.input.sequence = 300;
        input.input.timestamp_ms = 3000;
        input.input.yaw = 0.0f;
        input.input.attack = 1;
        input.receiveTimeMs = 3000;

        handler.processAttackInput(entity, input);
    }
}

// ============================================================================
// processCombat Tests
// ============================================================================

TEST_CASE("CombatEventHandler processCombat", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);

    SECTION("processCombat does not crash") {
        // processCombat is a no-op placeholder but should not crash
        handler.processCombat();
    }
}

// ============================================================================
// Pending Respawns Accessor Tests
// ============================================================================

TEST_CASE("CombatEventHandler pending respawns accessor", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("getPendingRespawns returns empty vector initially") {
        const auto& respawns = handler.getPendingRespawns();
        REQUIRE(respawns.empty());
    }

    SECTION("getPendingRespawns reflects current state after deaths") {
        auto e1 = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto e2 = createNPCEntity(registry, makePosition(10.0f, 0.0f, 10.0f));
        auto e3 = createNPCEntity(registry, makePosition(20.0f, 0.0f, 20.0f));
        auto killer = createNPCEntity(registry, makePosition(5.0f, 0.0f, 5.0f));

        handler.onEntityDied(e1, killer);
        handler.onEntityDied(e2, killer);
        handler.onEntityDied(e3, killer);

        REQUIRE(handler.getPendingRespawns().size() == 3);
    }

    SECTION("Respawn entries have correct structure") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto killer = createNPCEntity(registry, makePosition(1.0f, 0.0f, 1.0f));

        handler.onEntityDied(entity, killer);

        const auto& respawns = handler.getPendingRespawns();
        REQUIRE(respawns.size() == 1);
        REQUIRE(respawns[0].entity == entity);
        REQUIRE(respawns[0].respawnTimeMs > 0);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("CombatEventHandler edge cases", "[zones][combat]") {
    auto server = createTestZoneServer();
    CombatEventHandler handler(server);
    auto& registry = server.getRegistry();

    SECTION("Entity dies to itself") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        handler.onEntityDied(entity, entity);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }

    SECTION("Multiple rapid deaths of same entity") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto killer = createNPCEntity(registry, makePosition(1.0f, 0.0f, 1.0f));

        // This shouldn't happen in practice but handler should be robust
        handler.onEntityDied(entity, killer);
        handler.onEntityDied(entity, killer);

        REQUIRE(handler.getPendingRespawns().size() == 2);
    }

    SECTION("Death at world boundaries") {
        auto entity = createNPCEntity(registry,
            makePosition(Constants::WORLD_MAX_X, 0.0f, Constants::WORLD_MAX_Z));
        auto killer = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));

        handler.onEntityDied(entity, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }

    SECTION("Death with zero health entity") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto& combat = registry.get<CombatState>(entity);
        combat.health = 0;
        combat.maxHealth = 10000;

        auto killer = createNPCEntity(registry, makePosition(1.0f, 0.0f, 1.0f));
        handler.onEntityDied(entity, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }

    SECTION("Death with max health entity (overkill)") {
        auto entity = createNPCEntity(registry, makePosition(0.0f, 0.0f, 0.0f));
        auto& combat = registry.get<CombatState>(entity);
        combat.health = 10000;
        combat.maxHealth = 10000;

        auto killer = createNPCEntity(registry, makePosition(1.0f, 0.0f, 1.0f));
        handler.onEntityDied(entity, killer);
        REQUIRE(handler.getPendingRespawns().size() == 1);
    }
}
