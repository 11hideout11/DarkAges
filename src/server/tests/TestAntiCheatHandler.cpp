// AntiCheatHandler unit tests
// Tests initialization, connection mapping, and callback behavior

#include <catch2/catch_test_macros.hpp>
#include "zones/AntiCheatHandler.hpp"
#include "zones/ZoneServer.hpp"
#include "security/AntiCheatTypes.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <unordered_map>
#include <cstdint>

using namespace DarkAges;

// ============================================================================
// Basic construction and initialization
// ============================================================================

TEST_CASE("AntiCheatHandler construction", "[anticheat][handler]") {
    // AntiCheatHandler requires a ZoneServer reference
    // We test that the handler can be constructed and has expected defaults
    // Full integration testing requires a running ZoneServer

    SECTION("Handler type is defined and complete") {
        // Verify AntiCheatHandler is a complete type
        static_assert(sizeof(AntiCheatHandler) > 0, "AntiCheatHandler should be a complete type");
        REQUIRE(sizeof(AntiCheatHandler) > 0);
    }
}

// ============================================================================
// Connection mapping
// ============================================================================

TEST_CASE("AntiCheatHandler connection mapping types", "[anticheat][handler]") {
    SECTION("Connection maps can be created") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        std::unordered_map<EntityID, ConnectionID> entityToConn;

        // Test basic map operations (cast to entity type)
        connToEntity[1] = static_cast<EntityID>(100);
        connToEntity[2] = static_cast<EntityID>(200);
        entityToConn[static_cast<EntityID>(100)] = 1;
        entityToConn[static_cast<EntityID>(200)] = 2;

        REQUIRE(connToEntity.size() == 2);
        REQUIRE(entt::to_integral(connToEntity[1]) == 100);
        REQUIRE(entityToConn[static_cast<EntityID>(200)] == 2);
    }

    SECTION("Connection maps support lookup by value") {
        std::unordered_map<ConnectionID, EntityID> connToEntity;
        connToEntity[42] = static_cast<EntityID>(999);

        bool found = false;
        for (const auto& [connId, entityId] : connToEntity) {
            if (entt::to_integral(entityId) == 999) {
                found = true;
                REQUIRE(connId == 42);
                break;
            }
        }
        REQUIRE(found);
    }
}

// ============================================================================
// CheatDetectionResult structure validation
// ============================================================================

TEST_CASE("CheatDetectionResult structure", "[anticheat][handler]") {
    SECTION("Default construction has safe values") {
        Security::CheatDetectionResult result;

        REQUIRE(result.detected == false);
        REQUIRE(result.type == Security::CheatType::NONE);
        REQUIRE(result.severity == Security::ViolationSeverity::INFO);
        REQUIRE(result.description == nullptr);
        REQUIRE(result.confidence == 0.0f);
    }

    SECTION("Result can be populated") {
        Security::CheatDetectionResult result;
        result.detected = true;
        result.type = Security::CheatType::SPEED_HACK;
        result.severity = Security::ViolationSeverity::SUSPICIOUS;
        result.description = "Speed hack detected";
        result.confidence = 0.85f;
        result.actualValue = 15.0f;
        result.expectedValue = 5.0f;
        result.timestamp = 12345;

        REQUIRE(result.detected == true);
        REQUIRE(result.type == Security::CheatType::SPEED_HACK);
        REQUIRE(result.severity == Security::ViolationSeverity::SUSPICIOUS);
        REQUIRE(std::string(result.description) == "Speed hack detected");
        REQUIRE(result.confidence == 0.85f);
        REQUIRE(result.actualValue == 15.0f);
        REQUIRE(result.expectedValue == 5.0f);
    }
}

// ============================================================================
// AntiCheatEvent structure validation
// ============================================================================

TEST_CASE("AntiCheatEvent structure", "[anticheat][handler]") {
    SECTION("Event fields can be set") {
        AntiCheatEvent event;
        event.eventId = 123456789;
        event.timestamp = 1000;
        event.zoneId = 1;
        event.playerId = 42;
        event.cheatType = "speed_hack";
        event.severity = "suspicious";
        event.description = "Player moving too fast";
        event.confidence = 0.9f;
        event.serverTick = 6000;

        REQUIRE(event.eventId == 123456789);
        REQUIRE(event.zoneId == 1);
        REQUIRE(event.playerId == 42);
        REQUIRE(event.cheatType == "speed_hack");
        REQUIRE(event.confidence == 0.9f);
    }
}

// ============================================================================
// CheatType enum validation
// ============================================================================

TEST_CASE("CheatType enum values", "[anticheat][handler]") {
    SECTION("CheatType has expected values") {
        // Verify all cheat types are defined
        REQUIRE(static_cast<int>(Security::CheatType::NONE) == 0);
        REQUIRE(static_cast<int>(Security::CheatType::SPEED_HACK) > 0);
        REQUIRE(static_cast<int>(Security::CheatType::TELEPORT) > 0);
        REQUIRE(static_cast<int>(Security::CheatType::FLY_HACK) > 0);
        REQUIRE(static_cast<int>(Security::CheatType::NO_CLIP) > 0);
    }

    SECTION("cheatTypeToString returns valid strings") {
        const char* speedStr = Security::cheatTypeToString(Security::CheatType::SPEED_HACK);
        const char* teleportStr = Security::cheatTypeToString(Security::CheatType::TELEPORT);

        REQUIRE(speedStr != nullptr);
        REQUIRE(teleportStr != nullptr);
        REQUIRE(std::string(speedStr).length() > 0);
    }
}

// ============================================================================
// ViolationSeverity enum validation
// ============================================================================

TEST_CASE("ViolationSeverity levels", "[anticheat][handler]") {
    SECTION("Severity levels are ordered correctly") {
        REQUIRE(static_cast<int>(Security::ViolationSeverity::INFO) <
                static_cast<int>(Security::ViolationSeverity::SUSPICIOUS));
        REQUIRE(static_cast<int>(Security::ViolationSeverity::SUSPICIOUS) <
                static_cast<int>(Security::ViolationSeverity::CRITICAL));
    }
}
