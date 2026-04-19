// AntiCheatLogger unit tests
// Tests construction, metrics, null handling, and event structure
// NOTE: When ScyllaDB is disabled, the stub implementation is used which does nothing

#include <catch2/catch_test_macros.hpp>
#include "db/AntiCheatLogger.hpp"
#include "db/ScyllaManager.hpp"
#include "ecs/CoreTypes.hpp"
#include <vector>
#include <string>

using namespace DarkAges;

// ============================================================================
// Construction and destruction
// ============================================================================

TEST_CASE("AntiCheatLogger construction", "[database][anticheat]") {
    SECTION("Logger can be constructed") {
        AntiCheatLogger logger;
        REQUIRE(logger.getWritesQueued() == 0);
        REQUIRE(logger.getWritesCompleted() == 0);
        REQUIRE(logger.getWritesFailed() == 0);
    }

    SECTION("Logger type is complete") {
        static_assert(sizeof(AntiCheatLogger) > 0, "AntiCheatLogger should be a complete type");
        REQUIRE(sizeof(AntiCheatLogger) > 0);
    }
}

// ============================================================================
// Metrics tracking
// ============================================================================

TEST_CASE("AntiCheatLogger metrics", "[database][anticheat]") {
    SECTION("Initial metrics are zero") {
        AntiCheatLogger logger;
        REQUIRE(logger.getWritesQueued() == 0);
        REQUIRE(logger.getWritesCompleted() == 0);
        REQUIRE(logger.getWritesFailed() == 0);
    }

    SECTION("Metrics getters are const") {
        const AntiCheatLogger logger;
        // Verify const access works
        uint64_t queued = logger.getWritesQueued();
        uint64_t completed = logger.getWritesCompleted();
        uint64_t failed = logger.getWritesFailed();
        REQUIRE(queued == 0);
        REQUIRE(completed == 0);
        REQUIRE(failed == 0);
    }
}

// ============================================================================
// Null session handling (stub-safe tests)
// ============================================================================

TEST_CASE("AntiCheatLogger null session handling", "[database][anticheat]") {
    AntiCheatLogger logger;

    SECTION("logAntiCheatEvent with null session doesn't crash") {
        AntiCheatEvent event;
        event.playerId = 42;
        event.zoneId = 1;
        event.cheatType = "speed_hack";
        event.severity = "suspicious";
        event.description = "Test event";
        event.confidence = 0.9f;
        event.timestamp = 1000;
        event.serverTick = 60;

        // Should not crash regardless of implementation (stub or real)
        logger.logAntiCheatEvent(nullptr, event, nullptr);
        REQUIRE(true);  // If we got here, no crash
    }

    SECTION("logAntiCheatEventsBatch with null session doesn't crash") {
        std::vector<AntiCheatEvent> events;
        AntiCheatEvent event;
        event.playerId = 1;
        events.push_back(event);

        // Should not crash regardless of implementation
        logger.logAntiCheatEventsBatch(nullptr, events, nullptr);
        REQUIRE(true);  // If we got here, no crash
    }

    SECTION("logAntiCheatEventsBatch with empty events doesn't crash") {
        std::vector<AntiCheatEvent> emptyEvents;
        AntiCheatLogger logger;

        // Should not crash regardless of implementation
        logger.logAntiCheatEventsBatch(nullptr, emptyEvents, nullptr);
        REQUIRE(true);  // If we got here, no crash
    }

    SECTION("logAntiCheatEvent without callback doesn't crash") {
        AntiCheatEvent event;
        event.playerId = 1;
        event.cheatType = "test";
        event.severity = "info";
        event.description = "test";

        // Should not crash regardless of implementation
        logger.logAntiCheatEvent(nullptr, event, nullptr);
        REQUIRE(true);  // If we got here, no crash
    }
}

// ============================================================================
// AntiCheatEvent structure validation
// ============================================================================

TEST_CASE("AntiCheatEvent structure", "[database][anticheat]") {
    SECTION("Event can be populated with all fields") {
        AntiCheatEvent event;
        event.eventId = 12345ULL;
        event.timestamp = 1000;
        event.zoneId = 42;
        event.playerId = 999;
        event.cheatType = "teleport";
        event.severity = "critical";
        event.description = "Player teleported 500m instantly";
        event.confidence = 0.95f;
        event.serverTick = 60000;
        event.position = Position(100, 0, 200, 0);

        REQUIRE(event.eventId == 12345ULL);
        REQUIRE(event.timestamp == 1000);
        REQUIRE(event.zoneId == 42);
        REQUIRE(event.playerId == 999);
        REQUIRE(event.cheatType == "teleport");
        REQUIRE(event.severity == "critical");
        REQUIRE(event.description == "Player teleported 500m instantly");
        REQUIRE(event.confidence == 0.95f);
        REQUIRE(event.serverTick == 60000);
    }

    SECTION("Event string fields are mutable") {
        AntiCheatEvent event;
        event.cheatType = "speed_hack";
        event.severity = "warning";
        event.description = "Moving too fast";

        REQUIRE(event.cheatType == "speed_hack");
        REQUIRE(event.severity == "warning");
        REQUIRE(event.description == "Moving too fast");
    }
}

// ============================================================================
// Batch operations
// ============================================================================

TEST_CASE("AntiCheatLogger batch operations", "[database][anticheat]") {
    SECTION("Multiple events can be batched") {
        std::vector<AntiCheatEvent> events;
        for (int i = 0; i < 5; i++) {
            AntiCheatEvent event;
            event.playerId = i;
            event.zoneId = 1;
            event.cheatType = "test";
            event.severity = "info";
            event.description = "Batch test event";
            event.confidence = 0.5f;
            event.timestamp = 1000 + i;
            event.serverTick = 60 + i;
            events.push_back(event);
        }

        REQUIRE(events.size() == 5);
        REQUIRE(events[0].playerId == 0);
        REQUIRE(events[4].playerId == 4);
    }
}
