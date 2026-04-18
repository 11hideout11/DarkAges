// [ZONE_AGENT] ZoneServer Unit Tests
// Tests data structures, construction, state management, and metrics
// NOTE: Does NOT test initialize()/run() — those require real network/Redis

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/ZoneServer.hpp"
#include "Constants.hpp"
#include <cstdint>
#include <string>

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// ZoneConfig Tests
// ============================================================================

TEST_CASE("ZoneConfig defaults", "[zones][zoneserver]") {
    ZoneConfig config;

    SECTION("zoneId defaults to 1") {
        REQUIRE(config.zoneId == 1);
    }

    SECTION("port defaults to DEFAULT_SERVER_PORT") {
        REQUIRE(config.port == Constants::DEFAULT_SERVER_PORT);
    }

    SECTION("world bounds match constants") {
        REQUIRE(config.minX == Approx(Constants::WORLD_MIN_X));
        REQUIRE(config.maxX == Approx(Constants::WORLD_MAX_X));
        REQUIRE(config.minZ == Approx(Constants::WORLD_MIN_Z));
        REQUIRE(config.maxZ == Approx(Constants::WORLD_MAX_Z));
    }

    SECTION("aura buffer matches AURA_BUFFER_METERS constant") {
        REQUIRE(config.auraBuffer == Approx(Constants::AURA_BUFFER_METERS));
    }

    SECTION("redis defaults") {
        REQUIRE(config.redisHost == "localhost");
        REQUIRE(config.redisPort == Constants::REDIS_DEFAULT_PORT);
    }

    SECTION("scylla defaults") {
        REQUIRE(config.scyllaHost == "localhost");
        REQUIRE(config.scyllaPort == Constants::SCYLLA_DEFAULT_PORT);
    }

    SECTION("world bounds are valid") {
        REQUIRE(config.minX < config.maxX);
        REQUIRE(config.minZ < config.maxZ);
    }

    SECTION("custom config") {
        ZoneConfig custom;
        custom.zoneId = 42;
        custom.port = 8888;
        custom.minX = -100.0f;
        custom.maxX = 100.0f;
        custom.auraBuffer = 25.0f;
        custom.redisHost = "redis.local";
        custom.redisPort = 16379;
        custom.scyllaHost = "scylla.local";
        custom.scyllaPort = 19042;

        REQUIRE(custom.zoneId == 42);
        REQUIRE(custom.port == 8888);
        REQUIRE(custom.minX == Approx(-100.0f));
        REQUIRE(custom.maxX == Approx(100.0f));
        REQUIRE(custom.auraBuffer == Approx(25.0f));
        REQUIRE(custom.redisHost == "redis.local");
        REQUIRE(custom.redisPort == 16379);
        REQUIRE(custom.scyllaHost == "scylla.local");
        REQUIRE(custom.scyllaPort == 19042);
    }
}

// ============================================================================
// TickMetrics Tests
// ============================================================================

TEST_CASE("TickMetrics default construction", "[zones][zoneserver]") {
    TickMetrics metrics;

    SECTION("all fields zeroed by default") {
        REQUIRE(metrics.tickCount == 0);
        REQUIRE(metrics.totalTickTimeUs == 0);
        REQUIRE(metrics.maxTickTimeUs == 0);
        REQUIRE(metrics.overruns == 0);
        REQUIRE(metrics.networkTimeUs == 0);
        REQUIRE(metrics.physicsTimeUs == 0);
        REQUIRE(metrics.gameLogicTimeUs == 0);
        REQUIRE(metrics.replicationTimeUs == 0);
    }
}

TEST_CASE("TickMetrics reset behavior", "[zones][zoneserver]") {
    TickMetrics metrics;

    // Mutate all fields
    metrics.tickCount = 100;
    metrics.totalTickTimeUs = 500000;
    metrics.maxTickTimeUs = 8000;
    metrics.overruns = 3;
    metrics.networkTimeUs = 100000;
    metrics.physicsTimeUs = 200000;
    metrics.gameLogicTimeUs = 150000;
    metrics.replicationTimeUs = 50000;

    // Verify mutation took effect
    REQUIRE(metrics.tickCount == 100);
    REQUIRE(metrics.overruns == 3);

    // Reset and verify all fields are zeroed
    metrics.reset();

    SECTION("reset zeroes all counters") {
        REQUIRE(metrics.tickCount == 0);
        REQUIRE(metrics.totalTickTimeUs == 0);
        REQUIRE(metrics.maxTickTimeUs == 0);
        REQUIRE(metrics.overruns == 0);
        REQUIRE(metrics.networkTimeUs == 0);
        REQUIRE(metrics.physicsTimeUs == 0);
        REQUIRE(metrics.gameLogicTimeUs == 0);
        REQUIRE(metrics.replicationTimeUs == 0);
    }
}

TEST_CASE("TickMetrics reset is idempotent", "[zones][zoneserver]") {
    TickMetrics metrics;
    metrics.reset();
    metrics.reset();

    REQUIRE(metrics.tickCount == 0);
    REQUIRE(metrics.totalTickTimeUs == 0);
}

// ============================================================================
// ZoneServer Construction Tests
// ============================================================================

TEST_CASE("ZoneServer default construction", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("server is not running by default") {
        REQUIRE_FALSE(server.isRunning());
    }

    SECTION("tick counter is zero") {
        REQUIRE(server.getCurrentTick() == 0);
    }

    SECTION("shutdown is not requested") {
        REQUIRE_FALSE(server.isShutdownRequested());
    }

    SECTION("metrics are zeroed") {
        const auto& metrics = server.getMetrics();
        REQUIRE(metrics.tickCount == 0);
        REQUIRE(metrics.totalTickTimeUs == 0);
        REQUIRE(metrics.maxTickTimeUs == 0);
        REQUIRE(metrics.overruns == 0);
        REQUIRE(metrics.networkTimeUs == 0);
        REQUIRE(metrics.physicsTimeUs == 0);
        REQUIRE(metrics.gameLogicTimeUs == 0);
        REQUIRE(metrics.replicationTimeUs == 0);
    }

    SECTION("config has default values") {
        const auto& config = server.getConfig();
        REQUIRE(config.zoneId == 1);
        REQUIRE(config.port == Constants::DEFAULT_SERVER_PORT);
        REQUIRE(config.minX == Approx(Constants::WORLD_MIN_X));
        REQUIRE(config.maxX == Approx(Constants::WORLD_MAX_X));
        REQUIRE(config.auraBuffer == Approx(Constants::AURA_BUFFER_METERS));
    }
}

// ============================================================================
// requestShutdown Tests
// ============================================================================

TEST_CASE("requestShutdown sets shutdown flag", "[zones][zoneserver]") {
    ZoneServer server;

    REQUIRE_FALSE(server.isShutdownRequested());

    server.requestShutdown();

    SECTION("shutdown flag is set") {
        REQUIRE(server.isShutdownRequested());
    }

    SECTION("double request is safe") {
        server.requestShutdown();
        REQUIRE(server.isShutdownRequested());
    }
}

// ============================================================================
// stop() Safety Tests
// ============================================================================

TEST_CASE("stop is safe without initialize", "[zones][zoneserver]") {
    ZoneServer server;

    // Should not crash or assert — just a safety check
    REQUIRE_NOTHROW(server.stop());
}

TEST_CASE("stop after requestShutdown is safe", "[zones][zoneserver]") {
    ZoneServer server;
    server.requestShutdown();

    REQUIRE_NOTHROW(server.stop());
}

// ============================================================================
// Metrics Accessor Tests
// ============================================================================

TEST_CASE("getMetricsRef returns mutable reference", "[zones][zoneserver]") {
    ZoneServer server;

    TickMetrics& metrics = server.getMetricsRef();

    // Verify mutation through the reference
    metrics.tickCount = 42;
    metrics.totalTickTimeUs = 123456;
    metrics.maxTickTimeUs = 9999;
    metrics.overruns = 7;
    metrics.networkTimeUs = 11111;
    metrics.physicsTimeUs = 22222;
    metrics.gameLogicTimeUs = 33333;
    metrics.replicationTimeUs = 44444;

    SECTION("mutable reference mutates internal state") {
        const auto& constMetrics = server.getMetrics();
        REQUIRE(constMetrics.tickCount == 42);
        REQUIRE(constMetrics.totalTickTimeUs == 123456);
        REQUIRE(constMetrics.maxTickTimeUs == 9999);
        REQUIRE(constMetrics.overruns == 7);
        REQUIRE(constMetrics.networkTimeUs == 11111);
        REQUIRE(constMetrics.physicsTimeUs == 22222);
        REQUIRE(constMetrics.gameLogicTimeUs == 33333);
        REQUIRE(constMetrics.replicationTimeUs == 44444);
    }

    SECTION("reset through reference zeroes metrics") {
        metrics.reset();
        const auto& constMetrics = server.getMetrics();
        REQUIRE(constMetrics.tickCount == 0);
        REQUIRE(constMetrics.totalTickTimeUs == 0);
    }
}

TEST_CASE("getMetrics returns const reference", "[zones][zoneserver]") {
    ZoneServer server;

    // Get mutable ref and set values
    server.getMetricsRef().tickCount = 99;

    // Verify const accessor returns same state
    const auto& metrics = server.getMetrics();
    REQUIRE(metrics.tickCount == 99);
}
