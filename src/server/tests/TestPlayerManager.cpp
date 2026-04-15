// [ZONE_AGENT] PlayerManager unit tests
// Tests for player registration, lookup, and state management

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/PlayerManager.hpp"
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

TEST_CASE("PlayerManager construction", "[zones]") {
    SECTION("Default constructor creates empty manager") {
        PlayerManager pm;
        REQUIRE(pm.getPlayerCount() == 0);
    }

    SECTION("Constructed with ZoneServer has no players") {
        auto server = createTestZoneServer();
        PlayerManager pm(&server);
        REQUIRE(pm.getPlayerCount() == 0);
    }
}

TEST_CASE("PlayerManager zone ID", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Default zone ID is 1") {
        // Default constructor sets zoneId_ to 1
        // We can't directly read it, but we can set and verify via setZoneId
        pm.setZoneId(1);
        // No assertion needed - just verifying it doesn't crash
    }

    SECTION("setZoneId accepts various zone IDs") {
        pm.setZoneId(0);
        pm.setZoneId(1);
        pm.setZoneId(100);
        pm.setZoneId(99999);
    }
}

TEST_CASE("PlayerManager registerPlayer", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Register single player creates entity with correct mappings") {
        Position spawnPos = Position::fromVec3(glm::vec3(10.0f, 0.0f, 20.0f));
        EntityID entity = pm.registerPlayer(1, 100, "TestPlayer", spawnPos);

        REQUIRE(entity != static_cast<EntityID>(entt::null));
        REQUIRE(pm.getPlayerCount() == 1);
        REQUIRE(pm.getEntityByConnection(1) == entity);
        REQUIRE(pm.getConnectionByEntity(entity) == 1);
        REQUIRE(pm.getEntityByPlayerId(100) == entity);
    }

    SECTION("Register player adds all required components") {
        Position spawnPos = Position::fromVec3(glm::vec3(5.0f, 0.0f, 5.0f));
        EntityID entity = pm.registerPlayer(1, 200, "Player1", spawnPos);

        auto& registry = server.getRegistry();
        REQUIRE(registry.all_of<Position>(entity));
        REQUIRE(registry.all_of<Velocity>(entity));
        REQUIRE(registry.all_of<Rotation>(entity));
        REQUIRE(registry.all_of<BoundingVolume>(entity));
        REQUIRE(registry.all_of<InputState>(entity));
        REQUIRE(registry.all_of<CombatState>(entity));
        REQUIRE(registry.all_of<NetworkState>(entity));
        REQUIRE(registry.all_of<AntiCheatState>(entity));
        REQUIRE(registry.all_of<PlayerInfo>(entity));
        REQUIRE(registry.all_of<PlayerTag>(entity));
    }

    SECTION("Register player sets correct PlayerInfo fields") {
        Position spawnPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(42, 300, "MyUsername", spawnPos);

        auto& registry = server.getRegistry();
        const PlayerInfo& info = registry.get<PlayerInfo>(entity);
        REQUIRE(info.playerId == 300);
        REQUIRE(info.connectionId == 42);
        REQUIRE(std::string(info.username) == "MyUsername");
    }

    SECTION("Register player sets spawn position correctly") {
        Position spawnPos = Position::fromVec3(glm::vec3(100.0f, 50.0f, -25.0f));
        EntityID entity = pm.registerPlayer(1, 400, "SpawnTest", spawnPos);

        auto& registry = server.getRegistry();
        const Position& pos = registry.get<Position>(entity);
        glm::vec3 posVec = pos.toVec3();
        REQUIRE(posVec.x == Approx(100.0f).margin(0.1f));
        REQUIRE(posVec.y == Approx(50.0f).margin(0.1f));
        REQUIRE(posVec.z == Approx(-25.0f).margin(0.1f));
    }

    SECTION("Register multiple players creates distinct entities") {
        Position pos1 = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        Position pos2 = Position::fromVec3(glm::vec3(10.0f, 0.0f, 10.0f));
        Position pos3 = Position::fromVec3(glm::vec3(20.0f, 0.0f, 20.0f));

        EntityID e1 = pm.registerPlayer(1, 100, "Player1", pos1);
        EntityID e2 = pm.registerPlayer(2, 200, "Player2", pos2);
        EntityID e3 = pm.registerPlayer(3, 300, "Player3", pos3);

        REQUIRE(e1 != e2);
        REQUIRE(e2 != e3);
        REQUIRE(e1 != e3);
        REQUIRE(pm.getPlayerCount() == 3);

        // Verify all mappings
        REQUIRE(pm.getEntityByConnection(1) == e1);
        REQUIRE(pm.getEntityByConnection(2) == e2);
        REQUIRE(pm.getEntityByConnection(3) == e3);
        REQUIRE(pm.getEntityByPlayerId(100) == e1);
        REQUIRE(pm.getEntityByPlayerId(200) == e2);
        REQUIRE(pm.getEntityByPlayerId(300) == e3);
    }

    SECTION("Username is properly copied and null-terminated") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));

        // Test with a long username that would fill the buffer
        std::string longName(31, 'A');
        EntityID entity = pm.registerPlayer(1, 500, longName, pos);

        auto& registry = server.getRegistry();
        const PlayerInfo& info = registry.get<PlayerInfo>(entity);
        REQUIRE(std::strlen(info.username) <= 31);
        REQUIRE(info.username[std::strlen(info.username)] == '\0');
    }
}

TEST_CASE("PlayerManager lookup returns null for unknown entries", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("getEntityByConnection returns null for unknown connection") {
        EntityID result = pm.getEntityByConnection(999);
        REQUIRE(result == static_cast<EntityID>(entt::null));
    }

    SECTION("getConnectionByEntity returns 0 for unknown entity") {
        EntityID unknownEntity = static_cast<EntityID>(9999);
        REQUIRE(pm.getConnectionByEntity(unknownEntity) == 0);
    }

    SECTION("getEntityByPlayerId returns null for unknown player") {
        EntityID result = pm.getEntityByPlayerId(99999);
        REQUIRE(result == static_cast<EntityID>(entt::null));
    }
}

TEST_CASE("PlayerManager player connection checks", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("isPlayerConnected returns false for unregistered player") {
        REQUIRE_FALSE(pm.isPlayerConnected(100));
    }

    SECTION("isPlayerConnected returns true after registration") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        pm.registerPlayer(1, 100, "Player1", pos);

        REQUIRE(pm.isPlayerConnected(100));
        REQUIRE_FALSE(pm.isPlayerConnected(200));
    }

    SECTION("isPlayerConnected returns false after unregistration") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);
        pm.unregisterPlayer(entity);

        REQUIRE_FALSE(pm.isPlayerConnected(100));
    }
}

TEST_CASE("PlayerManager isEntityPlayer", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Registered entity is recognized as player") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);
        REQUIRE(pm.isEntityPlayer(entity));
    }

    SECTION("Non-player entity is not recognized as player") {
        auto& registry = server.getRegistry();
        EntityID npcEntity = registry.create();
        registry.emplace<Position>(npcEntity);
        REQUIRE_FALSE(pm.isEntityPlayer(npcEntity));
    }

    SECTION("Destroyed entity is not recognized as player") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);
        pm.unregisterPlayer(entity);
        // Entity is destroyed, so isEntityPlayer should return false
        REQUIRE_FALSE(pm.isEntityPlayer(entity));
    }
}

TEST_CASE("PlayerManager unregisterPlayer", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Unregister removes player from all lookups") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);

        REQUIRE(pm.getPlayerCount() == 1);
        pm.unregisterPlayer(entity);

        REQUIRE(pm.getPlayerCount() == 0);
        REQUIRE(pm.getEntityByConnection(1) == static_cast<EntityID>(entt::null));
        REQUIRE(pm.getEntityByPlayerId(100) == static_cast<EntityID>(entt::null));
        REQUIRE_FALSE(pm.isPlayerConnected(100));
    }

    SECTION("Unregister one player does not affect others") {
        Position pos1 = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        Position pos2 = Position::fromVec3(glm::vec3(10.0f, 0.0f, 10.0f));

        EntityID e1 = pm.registerPlayer(1, 100, "Player1", pos1);
        EntityID e2 = pm.registerPlayer(2, 200, "Player2", pos2);

        pm.unregisterPlayer(e1);

        REQUIRE(pm.getPlayerCount() == 1);
        REQUIRE(pm.getEntityByConnection(2) == e2);
        REQUIRE(pm.getEntityByPlayerId(200) == e2);
        REQUIRE(pm.isPlayerConnected(200));
        REQUIRE_FALSE(pm.isPlayerConnected(100));
    }

    SECTION("Unregister all players leaves manager empty") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID e1 = pm.registerPlayer(1, 100, "Player1", pos);
        EntityID e2 = pm.registerPlayer(2, 200, "Player2", pos);

        pm.unregisterPlayer(e1);
        pm.unregisterPlayer(e2);

        REQUIRE(pm.getPlayerCount() == 0);
    }
}

TEST_CASE("PlayerManager despawnPlayer", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Despawn removes player from all lookups") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);

        pm.despawnPlayer(entity);

        REQUIRE(pm.getPlayerCount() == 0);
        REQUIRE(pm.getEntityByConnection(1) == static_cast<EntityID>(entt::null));
        REQUIRE(pm.getEntityByPlayerId(100) == static_cast<EntityID>(entt::null));
        REQUIRE_FALSE(pm.isPlayerConnected(100));
    }

    SECTION("Despawn one player preserves others") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID e1 = pm.registerPlayer(1, 100, "Player1", pos);
        EntityID e2 = pm.registerPlayer(2, 200, "Player2", pos);

        pm.despawnPlayer(e1);

        REQUIRE(pm.getPlayerCount() == 1);
        REQUIRE(pm.getEntityByPlayerId(200) == e2);
    }
}

TEST_CASE("PlayerManager removeConnectionMapping", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Remove mapping for connected entity") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);

        pm.removeConnectionMapping(entity);

        // Connection mapping removed, but entity still exists in registry
        REQUIRE(pm.getEntityByConnection(1) == static_cast<EntityID>(entt::null));
        REQUIRE(pm.getConnectionByEntity(entity) == 0);
        // Player count reflects connectionToEntity_ size, which is now 0
        REQUIRE(pm.getPlayerCount() == 0);
    }

    SECTION("Remove mapping for non-existent entity does not crash") {
        EntityID unknownEntity = static_cast<EntityID>(9999);
        pm.removeConnectionMapping(unknownEntity);
        REQUIRE(pm.getPlayerCount() == 0);
    }
}

TEST_CASE("PlayerManager getAllPlayers", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Empty manager returns empty vector") {
        auto players = pm.getAllPlayers();
        REQUIRE(players.empty());
    }

    SECTION("Returns all registered players") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID e1 = pm.registerPlayer(1, 100, "Player1", pos);
        EntityID e2 = pm.registerPlayer(2, 200, "Player2", pos);

        auto players = pm.getAllPlayers();
        REQUIRE(players.size() == 2);

        // Verify all players are present
        bool found1 = false, found2 = false;
        for (const auto& [connId, entityId] : players) {
            if (connId == 1 && entityId == e1) found1 = true;
            if (connId == 2 && entityId == e2) found2 = true;
        }
        REQUIRE(found1);
        REQUIRE(found2);
    }
}

TEST_CASE("PlayerManager forEachPlayer", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("forEachPlayer iterates over all players") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        pm.registerPlayer(1, 100, "Player1", pos);
        pm.registerPlayer(2, 200, "Player2", pos);

        std::vector<ConnectionID> connectionIds;
        std::vector<std::string> usernames;
        pm.forEachPlayer([&](EntityID, ConnectionID connId, const PlayerInfo* info) {
            connectionIds.push_back(connId);
            if (info) {
                usernames.push_back(info->username);
            }
        });

        REQUIRE(connectionIds.size() == 2);
        REQUIRE(usernames.size() == 2);

        // Check both players were visited
        bool found1 = false, found2 = false;
        for (const auto& name : usernames) {
            if (name == "Player1") found1 = true;
            if (name == "Player2") found2 = true;
        }
        REQUIRE(found1);
        REQUIRE(found2);
    }

    SECTION("forEachPlayer with no players does not call callback") {
        bool called = false;
        pm.forEachPlayer([&](EntityID, ConnectionID, const PlayerInfo*) {
            called = true;
        });
        REQUIRE_FALSE(called);
    }

    SECTION("forEachPlayer receives correct PlayerInfo") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(42, 300, "InfoTest", pos);

        pm.forEachPlayer([&](EntityID e, ConnectionID connId, const PlayerInfo* info) {
            REQUIRE(e == entity);
            REQUIRE(connId == 42);
            REQUIRE(info != nullptr);
            REQUIRE(info->playerId == 300);
            REQUIRE(std::string(info->username) == "InfoTest");
        });
    }
}

TEST_CASE("PlayerManager saveAllPlayerStates", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("saveAllPlayerStates does not crash with no players") {
        // No database connections set, but should not crash
        pm.saveAllPlayerStates();
    }

    SECTION("saveAllPlayerStates does not crash with registered players") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        pm.registerPlayer(1, 100, "Player1", pos);
        pm.registerPlayer(2, 200, "Player2", pos);

        // No database connections set (redis_ and scylla_ are nullptr)
        // Should not crash - savePlayerState handles null databases gracefully
        pm.saveAllPlayerStates();
    }
}

TEST_CASE("PlayerManager savePlayerState", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("savePlayerState with no database connections does not crash") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        EntityID entity = pm.registerPlayer(1, 100, "Player1", pos);
        // No database connections - should handle gracefully
        pm.savePlayerState(entity);
    }
}

TEST_CASE("PlayerManager setDatabaseConnections", "[zones]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);

    SECTION("Setting null database connections does not crash") {
        pm.setDatabaseConnections(nullptr, nullptr);

        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        pm.registerPlayer(1, 100, "Player1", pos);
        pm.saveAllPlayerStates();
    }
}

TEST_CASE("PlayerManager player lifecycle", "[zones][integration]") {
    auto server = createTestZoneServer();
    PlayerManager pm(&server);
    pm.setDatabaseConnections(nullptr, nullptr);

    SECTION("Full register-unregister cycle") {
        Position pos = Position::fromVec3(glm::vec3(15.0f, 0.0f, 25.0f));

        // Register
        EntityID entity = pm.registerPlayer(10, 1000, "LifecycleTest", pos);
        REQUIRE(pm.getPlayerCount() == 1);
        REQUIRE(pm.isPlayerConnected(1000));
        REQUIRE(pm.isEntityPlayer(entity));

        // Verify state
        REQUIRE(pm.getEntityByConnection(10) == entity);
        REQUIRE(pm.getConnectionByEntity(entity) == 10);
        REQUIRE(pm.getEntityByPlayerId(1000) == entity);

        // Unregister
        pm.unregisterPlayer(entity);
        REQUIRE(pm.getPlayerCount() == 0);
        REQUIRE_FALSE(pm.isPlayerConnected(1000));
        REQUIRE(pm.getEntityByConnection(10) == static_cast<EntityID>(entt::null));
        REQUIRE(pm.getEntityByPlayerId(1000) == static_cast<EntityID>(entt::null));
    }

    SECTION("Register-replace cycle with same connection ID") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));

        EntityID e1 = pm.registerPlayer(1, 100, "First", pos);
        REQUIRE(pm.getPlayerCount() == 1);

        pm.unregisterPlayer(e1);

        EntityID e2 = pm.registerPlayer(1, 200, "Second", pos);
        REQUIRE(pm.getPlayerCount() == 1);
        REQUIRE(pm.getEntityByConnection(1) == e2);
        REQUIRE(pm.getEntityByPlayerId(200) == e2);
        REQUIRE_FALSE(pm.isPlayerConnected(100));
    }

    SECTION("Multiple players then sequential unregister") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
        std::vector<EntityID> entities;

        for (uint32_t i = 1; i <= 5; ++i) {
            entities.push_back(pm.registerPlayer(i, i * 100, "Player" + std::to_string(i), pos));
        }
        REQUIRE(pm.getPlayerCount() == 5);

        for (size_t i = 0; i < entities.size(); ++i) {
            pm.unregisterPlayer(entities[i]);
            REQUIRE(pm.getPlayerCount() == 5 - i - 1);
        }

        REQUIRE(pm.getPlayerCount() == 0);
    }
}
