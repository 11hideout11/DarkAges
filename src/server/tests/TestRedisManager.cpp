// TestRedisManager.cpp - Behavioral tests for RedisManager (stub mode)
// Tests construction, state, key conventions, callback behavior, metrics
// NOTE: Redis is disabled in CI — tests verify stub behavior, not real Redis

#include <catch2/catch_test_macros.hpp>
#include "db/RedisManager.hpp"
#include "db/PlayerSessionManager.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <string>
#include <atomic>
#include <vector>

using namespace DarkAges;

// ============================================================================
// Construction & State
// ============================================================================

TEST_CASE("RedisManager default construction", "[redis][redismanager]") {
    RedisManager manager;

    SECTION("not connected by default") {
        REQUIRE_FALSE(manager.isConnected());
    }

    SECTION("metrics are zeroed") {
        REQUIRE(manager.getCommandsSent() == 0);
        REQUIRE(manager.getCommandsCompleted() == 0);
        REQUIRE(manager.getCommandsFailed() == 0);
        REQUIRE(manager.getAverageLatencyMs() == 0.0f);
    }
}

TEST_CASE("RedisManager initialize in stub mode", "[redis][redismanager]") {
    RedisManager manager;

    SECTION("initialize returns true (stub always succeeds)") {
        REQUIRE(manager.initialize());
    }

    SECTION("initialize with custom host/port returns true") {
        REQUIRE(manager.initialize("10.0.0.1", 9999));
    }

    SECTION("still reports not connected after init (stub)") {
        manager.initialize();
        REQUIRE_FALSE(manager.isConnected());
    }

    SECTION("double initialize is safe") {
        REQUIRE(manager.initialize());
        REQUIRE(manager.initialize());
    }
}

TEST_CASE("RedisManager shutdown is safe", "[redis][redismanager]") {
    RedisManager manager;

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

TEST_CASE("RedisManager update is safe without connection", "[redis][redismanager]") {
    RedisManager manager;

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
// Key Naming Conventions (RedisKeys namespace)
// ============================================================================

TEST_CASE("RedisKeys playerSession format", "[redis][redismanager]") {
    auto key = RedisKeys::playerSession(42);
    REQUIRE(key == "player:42:session");
}

TEST_CASE("RedisKeys playerPosition format", "[redis][redismanager]") {
    auto key = RedisKeys::playerPosition(12345);
    REQUIRE(key == "player:12345:pos");
}

TEST_CASE("RedisKeys zonePlayers format", "[redis][redismanager]") {
    auto key = RedisKeys::zonePlayers(7);
    REQUIRE(key == "zone:7:players");
}

TEST_CASE("RedisKeys zoneEntities format", "[redis][redismanager]") {
    auto key = RedisKeys::zoneEntities(3);
    REQUIRE(key == "zone:3:entities");
}

TEST_CASE("RedisKeys entityState format", "[redis][redismanager]") {
    auto key = RedisKeys::entityState(999);
    REQUIRE(key == "entity:999:state");
}

TEST_CASE("RedisKeys are unique for different IDs", "[redis][redismanager]") {
    auto key1 = RedisKeys::playerSession(1);
    auto key2 = RedisKeys::playerSession(2);
    REQUIRE(key1 != key2);

    auto zone1 = RedisKeys::zonePlayers(1);
    auto zone2 = RedisKeys::zonePlayers(2);
    REQUIRE(zone1 != zone2);
}

// ============================================================================
// Stub Callback Behavior
// ============================================================================

TEST_CASE("RedisManager savePlayerSession callback in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    std::atomic<bool> result{false};

    PlayerSession session{};
    session.playerId = 1001;
    session.zoneId = 1;
    session.health = 100;

    manager.savePlayerSession(session, [&](bool success) {
        result = success;
        called = true;
    });

    REQUIRE(called);
    REQUIRE(result);
}

TEST_CASE("RedisManager loadPlayerSession callback in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    AsyncResult<PlayerSession> capturedResult;

    manager.loadPlayerSession(1001, [&](const AsyncResult<PlayerSession>& r) {
        capturedResult = r;
        called = true;
    });

    REQUIRE(called);
    // Stub returns default-constructed result
    REQUIRE(capturedResult.value.playerId == 0);
}

TEST_CASE("RedisManager removePlayerSession callback in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    std::atomic<bool> result{false};

    manager.removePlayerSession(1001, [&](bool success) {
        result = success;
        called = true;
    });

    REQUIRE(called);
    REQUIRE(result);
}

TEST_CASE("RedisManager addPlayerToZone callback in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    std::atomic<bool> result{false};

    manager.addPlayerToZone(1, 1001, [&](bool success) {
        result = success;
        called = true;
    });

    REQUIRE(called);
    REQUIRE(result);
}

TEST_CASE("RedisManager removePlayerFromZone callback in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    std::atomic<bool> result{false};

    manager.removePlayerFromZone(1, 1001, [&](bool success) {
        result = success;
        called = true;
    });

    REQUIRE(called);
    REQUIRE(result);
}

TEST_CASE("RedisManager getZonePlayers callback in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    std::atomic<bool> called{false};
    AsyncResult<std::vector<uint64_t>> capturedResult;

    manager.getZonePlayers(1, [&](const AsyncResult<std::vector<uint64_t>>& r) {
        capturedResult = r;
        called = true;
    });

    REQUIRE(called);
    REQUIRE(capturedResult.value.empty());
}

TEST_CASE("RedisManager stream operations fail gracefully in stub mode", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    SECTION("xadd returns error result") {
        std::atomic<bool> called{false};
        AsyncResult<std::string> capturedResult;

        std::unordered_map<std::string, std::string> fields{{"key", "value"}};
        manager.xadd("stream:1", "*", fields, [&](const AsyncResult<std::string>& r) {
            capturedResult = r;
            called = true;
        });

        REQUIRE(called);
        REQUIRE_FALSE(capturedResult.success);
        REQUIRE(capturedResult.error.find("Not connected") != std::string::npos);
    }

    SECTION("xread returns error result") {
        std::atomic<bool> called{false};
        AsyncResult<std::vector<StreamEntry>> capturedResult;

        manager.xread("stream:1", "0", [&](const AsyncResult<std::vector<StreamEntry>>& r) {
            capturedResult = r;
            called = true;
        });

        REQUIRE(called);
        REQUIRE_FALSE(capturedResult.success);
        REQUIRE(capturedResult.error.find("Not connected") != std::string::npos);
    }
}

// ============================================================================
// Fire-and-forget operations (no callback)
// ============================================================================

TEST_CASE("RedisManager operations without callbacks are safe", "[redis][redismanager]") {
    RedisManager manager;
    manager.initialize();

    SECTION("set without callback") {
        REQUIRE_NOTHROW(manager.set("key", "value", 60));
    }

    SECTION("get without callback") {
        REQUIRE_NOTHROW(manager.get("key", nullptr));
    }

    SECTION("del without callback") {
        REQUIRE_NOTHROW(manager.del("key"));
    }

    SECTION("savePlayerSession without callback") {
        PlayerSession session{};
        REQUIRE_NOTHROW(manager.savePlayerSession(session));
    }

    SECTION("publish without subscribers") {
        REQUIRE_NOTHROW(manager.publish("channel", "message"));
    }

    SECTION("pipelineSet without callback") {
        std::vector<std::pair<std::string, std::string>> kvs = {
            {"k1", "v1"}, {"k2", "v2"}
        };
        REQUIRE_NOTHROW(manager.pipelineSet(kvs, 60));
    }
}

// ============================================================================
// PlayerSession Data Structure
// ============================================================================

TEST_CASE("PlayerSession default construction", "[redis][redismanager]") {
    PlayerSession session{};

    SECTION("defaults are zeroed") {
        REQUIRE(session.playerId == 0);
        REQUIRE(session.zoneId == 0);
        REQUIRE(session.connectionId == 0);
        REQUIRE(session.health == 0);
        REQUIRE(session.lastActivity == 0);
    }

    SECTION("username is zeroed") {
        for (char c : session.username) {
            REQUIRE(c == 0);
        }
    }
}

TEST_CASE("PlayerSession field assignment", "[redis][redismanager]") {
    PlayerSession session{};
    session.playerId = 42;
    session.zoneId = 7;
    session.connectionId = 100;
    session.health = 7500;
    session.lastActivity = 1234567890;

    REQUIRE(session.playerId == 42);
    REQUIRE(session.zoneId == 7);
    REQUIRE(session.connectionId == 100);
    REQUIRE(session.health == 7500);
    REQUIRE(session.lastActivity == 1234567890);
}

// ============================================================================
// AsyncResult Template
// ============================================================================

TEST_CASE("AsyncResult default construction", "[redis][redismanager]") {
    SECTION("string result") {
        AsyncResult<std::string> result;
        REQUIRE_FALSE(result.success);
        REQUIRE(result.value.empty());
        REQUIRE(result.error.empty());
    }

    SECTION("vector result") {
        AsyncResult<std::vector<uint64_t>> result;
        REQUIRE_FALSE(result.success);
        REQUIRE(result.value.empty());
    }

    SECTION("result with error message") {
        AsyncResult<std::string> result;
        result.success = false;
        result.error = "Connection timeout";
        REQUIRE_FALSE(result.success);
        REQUIRE(result.error == "Connection timeout");
    }
}

// ============================================================================
// Destructor Safety
// ============================================================================

TEST_CASE("RedisManager destructor is safe", "[redis][redismanager]") {
    SECTION("destroy without init") {
        auto manager = std::make_unique<RedisManager>();
        REQUIRE_NOTHROW(manager.reset());
    }

    SECTION("destroy after init") {
        auto manager = std::make_unique<RedisManager>();
        manager->initialize();
        REQUIRE_NOTHROW(manager.reset());
    }

    SECTION("destroy with pending callbacks") {
        auto manager = std::make_unique<RedisManager>();
        manager->initialize();

        // Queue some operations
        manager->set("k1", "v1", 60);
        manager->set("k2", "v2", 60);
        manager->get("k3", nullptr);

        // Destroy should be safe even with pending ops
        REQUIRE_NOTHROW(manager.reset());
    }
}
