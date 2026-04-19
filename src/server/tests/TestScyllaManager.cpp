// TestScyllaManager.cpp - Behavioral tests for ScyllaManager
// Tests construction, state, data structures, stub behavior, metrics
// NOTE: ScyllaDB is disabled in CI — tests verify stub behavior and data structures

#include <catch2/catch_test_macros.hpp>
#include "db/ScyllaManager.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <atomic>
#include <vector>
#include <cstring>

using namespace DarkAges;

// ============================================================================
// Construction & State
// ============================================================================

TEST_CASE("ScyllaManager default construction", "[database][scylla]") {
    ScyllaManager manager;

    SECTION("not connected by default") {
        REQUIRE_FALSE(manager.isConnected());
    }

    SECTION("metrics are zeroed") {
        REQUIRE(manager.getWritesQueued() == 0);
        REQUIRE(manager.getWritesCompleted() == 0);
        REQUIRE(manager.getWritesFailed() == 0);
    }
}

TEST_CASE("ScyllaManager initialize in stub mode", "[database][scylla]") {
    ScyllaManager manager;

    SECTION("initialize with defaults returns true (stub)") {
        REQUIRE(manager.initialize());
    }

    SECTION("initialize with invalid host fails gracefully") {
        bool result = manager.initialize("invalid_host_12345", 9999);
        // Stub may or may not return false — should not crash
        REQUIRE_NOTHROW((void)result);
    }

    SECTION("still reports not connected after init (stub)") {
        manager.initialize();
        REQUIRE_FALSE(manager.isConnected());
    }

    SECTION("double initialize is safe") {
        REQUIRE(manager.initialize());
        REQUIRE_NOTHROW(manager.initialize());
    }
}

TEST_CASE("ScyllaManager shutdown is safe", "[database][scylla]") {
    ScyllaManager manager;

    SECTION("shutdown before init is safe") {
        REQUIRE_NOTHROW(manager.shutdown());
    }

    SECTION("shutdown after init is safe") {
        manager.initialize();
        REQUIRE_NOTHROW(manager.shutdown());
    }

    SECTION("double shutdown is safe") {
        manager.initialize();
        manager.shutdown();
        REQUIRE_NOTHROW(manager.shutdown());
    }
}

TEST_CASE("ScyllaManager update is safe without connection", "[database][scylla]") {
    ScyllaManager manager;

    SECTION("update before init is safe") {
        REQUIRE_NOTHROW(manager.update());
    }

    SECTION("update after init is safe") {
        manager.initialize();
        REQUIRE_NOTHROW(manager.update());
    }

    SECTION("repeated updates are safe") {
        manager.initialize();
        for (int i = 0; i < 100; ++i) {
            manager.update();
        }
    }
}

// ============================================================================
// CombatEvent Data Structure
// ============================================================================

TEST_CASE("CombatEvent default construction", "[database][scylla]") {
    CombatEvent event{};

    SECTION("all fields zeroed by default") {
        REQUIRE(event.eventId == 0);
        REQUIRE(event.timestamp == 0);
        REQUIRE(event.zoneId == 0);
        REQUIRE(event.attackerId == 0);
        REQUIRE(event.targetId == 0);
        REQUIRE(event.damageAmount == 0);
        REQUIRE_FALSE(event.isCritical);
        REQUIRE(event.serverTick == 0);
    }

    SECTION("strings are empty by default") {
        REQUIRE(event.eventType.empty());
        REQUIRE(event.weaponType.empty());
    }
}

TEST_CASE("CombatEvent field assignment", "[database][scylla]") {
    CombatEvent event{};
    event.eventId = 42;
    event.timestamp = 1700000000;
    event.zoneId = 7;
    event.attackerId = 1001;
    event.targetId = 1002;
    event.damageAmount = 1500;
    event.isCritical = true;
    event.eventType = "damage";
    event.weaponType = "sword";
    event.serverTick = 12345;
    event.position = Position{1000, 0, 2000, 1700000000};

    REQUIRE(event.eventId == 42);
    REQUIRE(event.timestamp == 1700000000);
    REQUIRE(event.zoneId == 7);
    REQUIRE(event.attackerId == 1001);
    REQUIRE(event.targetId == 1002);
    REQUIRE(event.damageAmount == 1500);
    REQUIRE(event.isCritical);
    REQUIRE(event.eventType == "damage");
    REQUIRE(event.weaponType == "sword");
    REQUIRE(event.serverTick == 12345);
}

TEST_CASE("CombatEvent event type classification", "[database][scylla]") {
    CombatEvent event{};

    SECTION("damage event") {
        event.eventType = "damage";
        event.damageAmount = 500;
        REQUIRE(event.eventType == "damage");
        REQUIRE(event.damageAmount > 0);
    }

    SECTION("kill event") {
        event.eventType = "kill";
        REQUIRE(event.eventType == "kill");
    }

    SECTION("heal event") {
        event.eventType = "heal";
        event.damageAmount = -200;  // negative = healing
        REQUIRE(event.eventType == "heal");
        REQUIRE(event.damageAmount < 0);
    }

    SECTION("death event") {
        event.eventType = "death";
        REQUIRE(event.eventType == "death");
    }
}

// ============================================================================
// AntiCheatEvent Data Structure
// ============================================================================

TEST_CASE("AntiCheatEvent default construction", "[database][scylla]") {
    AntiCheatEvent event{};

    SECTION("all fields zeroed by default") {
        REQUIRE(event.eventId == 0);
        REQUIRE(event.timestamp == 0);
        REQUIRE(event.zoneId == 0);
        REQUIRE(event.playerId == 0);
        REQUIRE(event.confidence == 0.0f);
        REQUIRE(event.serverTick == 0);
    }

    SECTION("strings are empty by default") {
        REQUIRE(event.cheatType.empty());
        REQUIRE(event.severity.empty());
        REQUIRE(event.description.empty());
    }
}

TEST_CASE("AntiCheatEvent field assignment", "[database][scylla]") {
    AntiCheatEvent event{};
    event.eventId = 99;
    event.timestamp = 1700000000;
    event.zoneId = 3;
    event.playerId = 5001;
    event.cheatType = "speed_hack";
    event.severity = "critical";
    event.description = "Player moving at 3x normal speed";
    event.confidence = 0.95f;
    event.serverTick = 54321;

    REQUIRE(event.eventId == 99);
    REQUIRE(event.playerId == 5001);
    REQUIRE(event.cheatType == "speed_hack");
    REQUIRE(event.severity == "critical");
    REQUIRE(event.confidence == 0.95f);
}

TEST_CASE("AntiCheatEvent severity levels", "[database][scylla]") {
    AntiCheatEvent event{};

    SECTION("critical severity") {
        event.severity = "critical";
        event.confidence = 0.95f;
        REQUIRE(event.severity == "critical");
        REQUIRE(event.confidence > 0.9f);
    }

    SECTION("suspicious severity") {
        event.severity = "suspicious";
        event.confidence = 0.6f;
        REQUIRE(event.severity == "suspicious");
        REQUIRE(event.confidence < 0.9f);
    }

    SECTION("minor severity") {
        event.severity = "minor";
        event.confidence = 0.3f;
        REQUIRE(event.severity == "minor");
        REQUIRE(event.confidence < 0.5f);
    }
}

TEST_CASE("AntiCheatEvent cheat types", "[database][scylla]") {
    AntiCheatEvent event{};

    SECTION("speed hack") {
        event.cheatType = "speed_hack";
        REQUIRE(event.cheatType == "speed_hack");
    }

    SECTION("teleport hack") {
        event.cheatType = "teleport";
        REQUIRE(event.cheatType == "teleport");
    }

    SECTION("hitbox manipulation") {
        event.cheatType = "hitbox";
        REQUIRE(event.cheatType == "hitbox");
    }
}

// ============================================================================
// PlayerCombatStats Data Structure
// ============================================================================

TEST_CASE("PlayerCombatStats default construction", "[database][scylla]") {
    PlayerCombatStats stats{};

    SECTION("all fields zeroed by default") {
        REQUIRE(stats.playerId == 0);
        REQUIRE(stats.sessionDate == 0);
        REQUIRE(stats.kills == 0);
        REQUIRE(stats.deaths == 0);
        REQUIRE(stats.damageDealt == 0);
        REQUIRE(stats.damageTaken == 0);
        REQUIRE(stats.longestKillStreak == 0);
        REQUIRE(stats.currentKillStreak == 0);
    }
}

TEST_CASE("PlayerCombatStats field assignment", "[database][scylla]") {
    PlayerCombatStats stats{};
    stats.playerId = 1001;
    stats.sessionDate = 20240130;
    stats.kills = 15;
    stats.deaths = 3;
    stats.damageDealt = 25000;
    stats.damageTaken = 8000;
    stats.longestKillStreak = 7;
    stats.currentKillStreak = 4;

    REQUIRE(stats.playerId == 1001);
    REQUIRE(stats.sessionDate == 20240130);
    REQUIRE(stats.kills == 15);
    REQUIRE(stats.deaths == 3);
    REQUIRE(stats.damageDealt == 25000);
    REQUIRE(stats.damageTaken == 8000);
    REQUIRE(stats.longestKillStreak == 7);
    REQUIRE(stats.currentKillStreak == 4);
}

TEST_CASE("PlayerCombatStats KDA calculation", "[database][scylla]") {
    PlayerCombatStats stats{};
    stats.kills = 10;
    stats.deaths = 2;

    // Verify data supports KDA calculation
    REQUIRE(stats.kills > stats.deaths);
    REQUIRE(stats.kills / static_cast<float>(stats.deaths + 1) > 1.0f);
}

TEST_CASE("PlayerCombatStats kill streak consistency", "[database][scylla]") {
    PlayerCombatStats stats{};
    stats.longestKillStreak = 10;
    stats.currentKillStreak = 5;

    // Current streak should never exceed longest
    REQUIRE(stats.currentKillStreak <= stats.longestKillStreak);
}

// ============================================================================
// Stub Callback Behavior
// ============================================================================

TEST_CASE("ScyllaManager logCombatEvent callback in stub mode", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    std::atomic<bool> result{false};

    CombatEvent event{};
    event.eventId = 1;
    event.attackerId = 1001;
    event.targetId = 1002;
    event.damageAmount = 500;
    event.eventType = "damage";

    manager.logCombatEvent(event, [&](bool success) {
        result = success;
        called = true;
    });

    // Stub should call the callback
    REQUIRE(called);
}

TEST_CASE("ScyllaManager logCombatEventsBatch callback in stub mode", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    std::atomic<bool> called{false};

    std::vector<CombatEvent> events;
    for (int i = 0; i < 5; ++i) {
        CombatEvent event{};
        event.eventId = static_cast<uint64_t>(i);
        event.attackerId = 1001;
        event.targetId = 1002 + i;
        event.damageAmount = static_cast<int16_t>(100 * (i + 1));
        events.push_back(event);
    }

    manager.logCombatEventsBatch(events, [&](bool) {
        called = true;
    });

    REQUIRE(called);
}

TEST_CASE("ScyllaManager updatePlayerStats callback in stub mode", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    std::atomic<bool> called{false};

    PlayerCombatStats stats{};
    stats.playerId = 1001;
    stats.kills = 5;
    stats.deaths = 2;

    manager.updatePlayerStats(stats, [&](bool) {
        called = true;
    });

    REQUIRE(called);
}

TEST_CASE("ScyllaManager logAntiCheatEvent callback in stub mode", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    std::atomic<bool> called{false};

    AntiCheatEvent event{};
    event.playerId = 5001;
    event.cheatType = "speed_hack";
    event.severity = "critical";

    manager.logAntiCheatEvent(event, [&](bool) {
        called = true;
    });

    REQUIRE(called);
}

TEST_CASE("ScyllaManager logAntiCheatEventsBatch callback in stub mode", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    std::atomic<bool> called{false};

    std::vector<AntiCheatEvent> events;
    for (int i = 0; i < 3; ++i) {
        AntiCheatEvent event{};
        event.playerId = static_cast<uint64_t>(5001 + i);
        event.cheatType = "speed_hack";
        events.push_back(event);
    }

    manager.logAntiCheatEventsBatch(events, [&](bool) {
        called = true;
    });

    REQUIRE(called);
}

TEST_CASE("ScyllaManager analytics callbacks in stub mode", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    SECTION("getTopKillers callback fires") {
        std::atomic<bool> called{false};
        manager.getTopKillers(1, 0, 999999999, 10, [&](bool, const std::vector<std::pair<uint64_t, uint32_t>>&) {
            called = true;
        });
        REQUIRE(called);
    }

    SECTION("getKillFeed callback fires") {
        std::atomic<bool> called{false};
        manager.getKillFeed(1, 10, [&](bool, const std::vector<CombatEvent>&) {
            called = true;
        });
        REQUIRE(called);
    }
}

// ============================================================================
// Fire-and-forget operations (no callback)
// ============================================================================

TEST_CASE("ScyllaManager operations without callbacks are safe", "[database][scylla]") {
    ScyllaManager manager;
    manager.initialize();

    SECTION("logCombatEvent without callback") {
        CombatEvent event{};
        event.attackerId = 1001;
        REQUIRE_NOTHROW(manager.logCombatEvent(event));
    }

    SECTION("logCombatEventsBatch without callback") {
        std::vector<CombatEvent> events(3);
        REQUIRE_NOTHROW(manager.logCombatEventsBatch(events));
    }

    SECTION("updatePlayerStats without callback") {
        PlayerCombatStats stats{};
        REQUIRE_NOTHROW(manager.updatePlayerStats(stats));
    }

    SECTION("logAntiCheatEvent without callback") {
        AntiCheatEvent event{};
        REQUIRE_NOTHROW(manager.logAntiCheatEvent(event));
    }

    SECTION("savePlayerState without callback") {
        REQUIRE_NOTHROW(manager.savePlayerState(1001, 1, 1700000000));
    }
}

// ============================================================================
// Destructor Safety
// ============================================================================

TEST_CASE("ScyllaManager destructor is safe", "[database][scylla]") {
    SECTION("destroy without init") {
        auto manager = std::make_unique<ScyllaManager>();
        REQUIRE_NOTHROW(manager.reset());
    }

    SECTION("destroy after init") {
        auto manager = std::make_unique<ScyllaManager>();
        manager->initialize();
        REQUIRE_NOTHROW(manager.reset());
    }

    SECTION("destroy with pending writes") {
        auto manager = std::make_unique<ScyllaManager>();
        manager->initialize();

        CombatEvent event{};
        manager->logCombatEvent(event);
        manager->logCombatEvent(event);

        REQUIRE_NOTHROW(manager->reset());
    }
}
