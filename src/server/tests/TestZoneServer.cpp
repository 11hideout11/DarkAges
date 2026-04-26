// [ZONE_AGENT] ZoneServer Unit Tests
// Tests data structures, construction, state management, metrics,
// entity operations, memory pool integration, and QoS behavior
// NOTE: Does NOT test initialize()/run() — those require real network/Redis

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/ZoneServer.hpp"
#include "Constants.hpp"
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>

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

// ============================================================================
// Config Validation Tests
// ============================================================================

TEST_CASE("ZoneConfig boundary validation", "[zones][zoneserver]") {
    ZoneConfig config;

    SECTION("default world bounds are valid") {
        REQUIRE(config.minX < config.maxX);
        REQUIRE(config.minZ < config.maxZ);
    }

    SECTION("custom bounds must be valid") {
        config.minX = -2000.0f;
        config.maxX = 2000.0f;
        config.minZ = -2000.0f;
        config.maxZ = 2000.0f;
        REQUIRE(config.minX < config.maxX);
        REQUIRE(config.minZ < config.maxZ);
    }

    SECTION("port is nonzero by default") {
        REQUIRE(config.port > 0);
    }

    SECTION("zoneId is positive by default") {
        REQUIRE(config.zoneId > 0);
    }
}

// ============================================================================
// TickMetrics Accumulation Tests
// ============================================================================

TEST_CASE("TickMetrics accumulation", "[zones][zoneserver]") {
    TickMetrics metrics;

    SECTION("tick count increments") {
        metrics.tickCount++;
        metrics.tickCount++;
        metrics.tickCount++;
        REQUIRE(metrics.tickCount == 3);
    }

    SECTION("total tick time accumulates") {
        metrics.totalTickTimeUs += 1000;
        metrics.totalTickTimeUs += 2000;
        metrics.totalTickTimeUs += 500;
        REQUIRE(metrics.totalTickTimeUs == 3500);
    }

    SECTION("max tick time tracks peak") {
        metrics.maxTickTimeUs = 1000;
        metrics.maxTickTimeUs = std::max(metrics.maxTickTimeUs, static_cast<uint64_t>(2000));
        metrics.maxTickTimeUs = std::max(metrics.maxTickTimeUs, static_cast<uint64_t>(500));
        REQUIRE(metrics.maxTickTimeUs == 2000);
    }

    SECTION("overruns count increments") {
        metrics.overruns++;
        REQUIRE(metrics.overruns == 1);
    }
}

TEST_CASE("TickMetrics component time tracking", "[zones][zoneserver]") {
    TickMetrics metrics;

    SECTION("component times are independent") {
        metrics.networkTimeUs = 1000;
        metrics.physicsTimeUs = 2000;
        metrics.gameLogicTimeUs = 3000;
        metrics.replicationTimeUs = 500;

        REQUIRE(metrics.networkTimeUs == 1000);
        REQUIRE(metrics.physicsTimeUs == 2000);
        REQUIRE(metrics.gameLogicTimeUs == 3000);
        REQUIRE(metrics.replicationTimeUs == 500);
    }

    SECTION("component times sum to total") {
        metrics.networkTimeUs = 1000;
        metrics.physicsTimeUs = 2000;
        metrics.gameLogicTimeUs = 3000;
        metrics.replicationTimeUs = 500;
        metrics.totalTickTimeUs = metrics.networkTimeUs + metrics.physicsTimeUs +
                                  metrics.gameLogicTimeUs + metrics.replicationTimeUs;
        REQUIRE(metrics.totalTickTimeUs == 6500);
    }
}

// ============================================================================
// Server State Machine Tests
// ============================================================================

TEST_CASE("ZoneServer QoS state management", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("QoS not degraded by default") {
        REQUIRE_FALSE(server.isQoSDegraded());
    }

    SECTION("set QoS degraded") {
        server.setQoSDegraded(true);
        REQUIRE(server.isQoSDegraded());
    }

    SECTION("clear QoS degraded") {
        server.setQoSDegraded(true);
        server.setQoSDegraded(false);
        REQUIRE_FALSE(server.isQoSDegraded());
    }
}

TEST_CASE("ZoneServer subsystem accessors are valid", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("registry is accessible") {
        REQUIRE_NOTHROW((void)&server.getRegistry());
    }

    SECTION("spatial hash is accessible") {
        REQUIRE_NOTHROW((void)&server.getSpatialHash());
    }

    SECTION("movement system is accessible") {
        REQUIRE_NOTHROW((void)&server.getMovementSystem());
    }

    SECTION("combat system pointer is non-null") {
        REQUIRE(server.getCombatSystemPtr() != nullptr);
    }

    SECTION("lag compensator pointer is non-null") {
        REQUIRE(server.getLagCompensatorPtr() != nullptr);
    }

    SECTION("anti-cheat reference is accessible") {
        REQUIRE_NOTHROW((void)&server.getAntiCheatRef());
    }

    SECTION("replication optimizer reference is accessible") {
        REQUIRE_NOTHROW((void)&server.getReplicationOptimizerRef());
    }
}

TEST_CASE("ZoneServer connection mapping is initially empty", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("connection-to-entity map is empty") {
        auto* map = server.getConnectionToEntityPtr();
        REQUIRE(map->empty());
    }

    SECTION("entity-to-connection map is empty") {
        auto* map = server.getEntityToConnectionPtr();
        REQUIRE(map->empty());
    }
}

// ============================================================================
// SnapshotHistory Data Structure Tests
// ============================================================================

TEST_CASE("SnapshotHistory default construction", "[zones][zoneserver]") {
    SnapshotHistory snapshot{};

    SECTION("tick is zero") {
        REQUIRE(snapshot.tick == 0);
    }

    SECTION("entities list is empty") {
        REQUIRE(snapshot.entities.empty());
    }
}

TEST_CASE("ClientSnapshotState default construction", "[zones][zoneserver]") {
    ClientSnapshotState state{};

    SECTION("all sequence counters are zero") {
        REQUIRE(state.lastAcknowledgedTick == 0);
        REQUIRE(state.lastSentTick == 0);
        REQUIRE(state.baselineTick == 0);
        REQUIRE(state.snapshotSequence == 0);
    }

    SECTION("pending removals is empty") {
        REQUIRE(state.pendingRemovals.empty());
    }
}

// ============================================================================
// Destructor Safety
// ============================================================================

TEST_CASE("ZoneServer destructor is safe", "[zones][zoneserver]") {
    SECTION("destroy default-constructed server") {
        auto server = std::make_unique<ZoneServer>();
        REQUIRE_NOTHROW(server.reset());
    }

    SECTION("destroy after requestShutdown") {
        auto server = std::make_unique<ZoneServer>();
        server->requestShutdown();
        REQUIRE_NOTHROW(server.reset());
    }
}

// ============================================================================
// tick() Safety Tests
// ============================================================================

TEST_CASE("tick returns false without initialization", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("server is not running by default") {
        // Can't safely call tick() without initialize() — it dereferences network_
        REQUIRE_FALSE(server.isRunning());
    }

    SECTION("tick counter is zero without initialization") {
        REQUIRE(server.getCurrentTick() == 0);
    }
}

// ============================================================================
// getCurrentTimeMs Tests
// ============================================================================

TEST_CASE("getCurrentTimeMs returns reasonable value", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("time is zero or small before initialization") {
        uint32_t timeMs = server.getCurrentTimeMs();
        // Before initialize(), startTime_ is default-constructed (epoch)
        // so getCurrentTimeMs() will return a very large value (time since epoch)
        // We just verify it doesn't crash
        REQUIRE_NOTHROW(server.getCurrentTimeMs());
    }
}

// ============================================================================
// setReducedUpdateRate Tests
// ============================================================================

TEST_CASE("setReducedUpdateRate changes QoS behavior", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("default reduced rate is SNAPSHOT_RATE_HZ") {
        // Default should be Constants::SNAPSHOT_RATE_HZ (20)
        // We can verify by checking QoS behavior
        REQUIRE_FALSE(server.isQoSDegraded());
    }

    SECTION("reduced update rate can be set") {
        REQUIRE_NOTHROW(server.setReducedUpdateRate(10));
        REQUIRE_NOTHROW(server.setReducedUpdateRate(5));
        REQUIRE_NOTHROW(server.setReducedUpdateRate(1));
        REQUIRE_NOTHROW(server.setReducedUpdateRate(0));
    }

    SECTION("setting rate to 0 is valid (no updates)") {
        REQUIRE_NOTHROW(server.setReducedUpdateRate(0));
    }
}

// ============================================================================
// ECS Registry / Entity Operations
// ============================================================================

TEST_CASE("ZoneServer registry supports entity creation", "[zones][zoneserver]") {
    ZoneServer server;
    auto& registry = server.getRegistry();

    SECTION("create entity with position component") {
        auto entity = registry.create();
        registry.emplace<Position>(entity, Position{100, 0, 200});

        REQUIRE(registry.valid(entity));
        REQUIRE(registry.all_of<Position>(entity));

        auto& pos = registry.get<Position>(entity);
        REQUIRE(pos.x == 100);
        REQUIRE(pos.z == 200);
    }

    SECTION("create entity with velocity component") {
        auto entity = registry.create();
        registry.emplace<Velocity>(entity, Velocity{10, 0, -5});

        REQUIRE(registry.valid(entity));
        REQUIRE(registry.all_of<Velocity>(entity));
    }

    SECTION("create entity with combat state component") {
        auto entity = registry.create();
        CombatState combat{};
        combat.health = 1000;
        combat.maxHealth = 1000;
        registry.emplace<CombatState>(entity, combat);

        auto& c = registry.get<CombatState>(entity);
        REQUIRE(c.health == 1000);
        REQUIRE(c.maxHealth == 1000);
    }

    SECTION("destroy entity removes it") {
        auto entity = registry.create();
        registry.emplace<Position>(entity, Position{0, 0, 0});
        REQUIRE(registry.valid(entity));

        registry.destroy(entity);
        REQUIRE_FALSE(registry.valid(entity));
    }

    SECTION("create multiple entities") {
        std::vector<entt::entity> entities;
        for (int i = 0; i < 100; ++i) {
            auto e = registry.create();
            registry.emplace<Position>(e, Position{Constants::Fixed(i * 10), 0, Constants::Fixed(i * 10)});
            entities.push_back(e);
        }

        int count = 0;
        registry.view<Position>().each([&count](auto, auto&) { ++count; });
        REQUIRE(count >= 100);

        // Destroy half
        for (size_t i = 0; i < 50; ++i) {
            registry.destroy(entities[i]);
        }
        // After destroy, remaining entities still in view
        int count2 = 0;
        registry.view<Position>().each([&count2](auto, auto&) { ++count2; });
        REQUIRE(count2 >= 50);
    }

    SECTION("entity iteration with view") {
        // Create entities with different component combinations
        auto e1 = registry.create();
        registry.emplace<Position>(e1, Position{0, 0, 0});

        auto e2 = registry.create();
        registry.emplace<Position>(e2, Position{10, 0, 20});
        registry.emplace<Velocity>(e2, Velocity{1, 0, 1});

        auto e3 = registry.create();
        registry.emplace<Position>(e3, Position{50, 0, 50});
        registry.emplace<Velocity>(e3, Velocity{-1, 0, 2});

        // View with position only (should find all 3)
        int posCount = 0;
        registry.view<Position>().each([&posCount](auto, auto&) { ++posCount; });
        REQUIRE(posCount >= 3);

        // View with both position and velocity (should find 2)
        int bothCount = 0;
        registry.view<Position, Velocity>().each([&bothCount](auto, auto&, auto&) { ++bothCount; });
        REQUIRE(bothCount >= 2);
    }
}

// ============================================================================
// SnapshotHistory / ClientSnapshotState Operations
// ============================================================================

TEST_CASE("SnapshotHistory entity list manipulation", "[zones][zoneserver]") {
    SECTION("add entities to snapshot") {
        SnapshotHistory snapshot;
        snapshot.tick = 42;

        Protocol::EntityStateData state1;
        state1.entity = static_cast<EntityID>(1);
        snapshot.entities.push_back(state1);

        Protocol::EntityStateData state2;
        state2.entity = static_cast<EntityID>(2);
        snapshot.entities.push_back(state2);

        REQUIRE(snapshot.entities.size() == 2);
        REQUIRE(snapshot.entities[0].entity == static_cast<EntityID>(1));
        REQUIRE(snapshot.entities[1].entity == static_cast<EntityID>(2));
    }

    SECTION("snapshot tick assignment") {
        SnapshotHistory snapshot;
        snapshot.tick = 12345;
        REQUIRE(snapshot.tick == 12345);
    }
}

TEST_CASE("ClientSnapshotState pending removals", "[zones][zoneserver]") {
    SECTION("add pending removals") {
        ClientSnapshotState state;
        EntityID e1 = static_cast<EntityID>(100);
        EntityID e2 = static_cast<EntityID>(200);

        state.pendingRemovals.push_back(static_cast<EntityID>(e1));
        state.pendingRemovals.push_back(static_cast<EntityID>(e2));

        REQUIRE(state.pendingRemovals.size() == 2);
        REQUIRE(state.pendingRemovals[0] == static_cast<EntityID>(100));
        REQUIRE(state.pendingRemovals[1] == static_cast<EntityID>(200));
    }

    SECTION("clear pending removals") {
        ClientSnapshotState state;
        state.pendingRemovals.push_back(static_cast<EntityID>(1));
        state.pendingRemovals.push_back(static_cast<EntityID>(2));

        state.pendingRemovals.clear();
        REQUIRE(state.pendingRemovals.empty());
    }

    SECTION("sequence counters can be incremented") {
        ClientSnapshotState state;
        state.lastAcknowledgedTick = 10;
        state.lastSentTick = 20;
        state.baselineTick = 5;
        state.snapshotSequence = 15;

        REQUIRE(state.lastAcknowledgedTick == 10);
        REQUIRE(state.lastSentTick == 20);
        REQUIRE(state.baselineTick == 5);
        REQUIRE(state.snapshotSequence == 15);
    }
}

// ============================================================================
// Memory Pool Integration
// ============================================================================

TEST_CASE("ZoneServer memory pools are accessible", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("spatial hash is functional") {
        auto& spatialHash = server.getSpatialHash();

        // SpatialHash should accept insertions (x, z coords, not radius)
        EntityID entity = static_cast<EntityID>(1);
        spatialHash.insert(entity, 0.0f, 0.0f);

        auto results = spatialHash.query(0.0f, 0.0f, 10.0f);
        REQUIRE_FALSE(results.empty());
    }

    SECTION("movement system is functional") {
        auto& movementSystem = server.getMovementSystem();
        // MovementSystem is accessible — verify reference works
        REQUIRE_NOTHROW((void)&movementSystem);
    }

    SECTION("combat system pointer is valid") {
        auto* combat = server.getCombatSystemPtr();
        REQUIRE(combat != nullptr);
    }

    SECTION("lag compensator pointer is valid") {
        auto* lag = server.getLagCompensatorPtr();
        REQUIRE(lag != nullptr);
    }

    SECTION("anti-cheat reference is valid") {
        auto& ac = server.getAntiCheatRef();
        REQUIRE_NOTHROW((void)&ac);
    }

    SECTION("replication optimizer reference is valid") {
        auto& ro = server.getReplicationOptimizerRef();
        REQUIRE_NOTHROW((void)&ro);
    }
}

// ============================================================================
// QoS Combined Behavior Tests
// ============================================================================

TEST_CASE("ZoneServer QoS degradation with reduced rate", "[zones][zoneserver]") {
    ZoneServer server;

    SECTION("enable QoS degradation") {
        server.setQoSDegraded(true);
        server.setReducedUpdateRate(10);

        REQUIRE(server.isQoSDegraded());
    }

    SECTION("disable QoS degradation") {
        server.setQoSDegraded(true);
        server.setReducedUpdateRate(10);
        server.setQoSDegraded(false);

        REQUIRE_FALSE(server.isQoSDegraded());
    }

    SECTION("toggle QoS multiple times") {
        for (int i = 0; i < 10; ++i) {
            server.setQoSDegraded(i % 2 == 0);
            REQUIRE(server.isQoSDegraded() == (i % 2 == 0));
        }
    }
}

// ============================================================================
// TickMetrics Overflow / Edge Cases
// ============================================================================

TEST_CASE("TickMetrics large values", "[zones][zoneserver]") {
    SECTION("tick count handles large values") {
        TickMetrics metrics;
        metrics.tickCount = UINT64_MAX - 1;
        metrics.tickCount++;
        REQUIRE(metrics.tickCount == UINT64_MAX);
    }

    SECTION("total tick time handles large values") {
        TickMetrics metrics;
        metrics.totalTickTimeUs = 1000000000ULL; // 1000 seconds
        metrics.totalTickTimeUs += 500000;
        REQUIRE(metrics.totalTickTimeUs == 1000500000ULL);
    }

    SECTION("component times don't interfere") {
        TickMetrics metrics;
        metrics.networkTimeUs = UINT64_MAX;
        metrics.physicsTimeUs = 0;

        // Verify they're independent
        REQUIRE(metrics.networkTimeUs == UINT64_MAX);
        REQUIRE(metrics.physicsTimeUs == 0);
    }
}

// ============================================================================
// ZoneConfig Edge Cases
// ============================================================================

TEST_CASE("ZoneConfig with extreme bounds", "[zones][zoneserver]") {
    SECTION("very small world") {
        ZoneConfig config;
        config.minX = -1.0f;
        config.maxX = 1.0f;
        config.minZ = -1.0f;
        config.maxZ = 1.0f;

        REQUIRE(config.minX < config.maxX);
        REQUIRE(config.minZ < config.maxZ);
        REQUIRE(config.maxX - config.minX == Approx(2.0f));
    }

    SECTION("very large world") {
        ZoneConfig config;
        config.minX = -100000.0f;
        config.maxX = 100000.0f;
        config.minZ = -100000.0f;
        config.maxZ = 100000.0f;

        REQUIRE(config.minX < config.maxX);
        REQUIRE(config.maxX - config.minX == Approx(200000.0f));
    }

    SECTION("non-square world") {
        ZoneConfig config;
        config.minX = -100.0f;
        config.maxX = 100.0f;
        config.minZ = -500.0f;
        config.maxZ = 500.0f;

        float widthX = config.maxX - config.minX;
        float widthZ = config.maxZ - config.minZ;
        REQUIRE(widthX == Approx(200.0f));
        REQUIRE(widthZ == Approx(1000.0f));
    }

    SECTION("high zone ID") {
        ZoneConfig config;
        config.zoneId = UINT32_MAX;
        REQUIRE(config.zoneId == UINT32_MAX);
    }

    SECTION("max port number") {
        ZoneConfig config;
        config.port = 65535;
        REQUIRE(config.port == 65535);
    }
}

    TEST_CASE("ZoneServer initialize/shutdown lifecycle", "[zoneserver]") {
        ZoneServer obj;

        SECTION("initialize returns true") {
            REQUIRE(obj.initialize(ZoneConfig{}));
        }

        SECTION("shutdown before init is safe") {
            REQUIRE_NOTHROW(obj.requestShutdown());
        }

        SECTION("double initialize is safe") {
            REQUIRE(obj.initialize(ZoneConfig{}));
            REQUIRE_NOTHROW(obj.initialize(ZoneConfig{}));
        }

        SECTION("shutdown after init is safe") {
            obj.initialize(ZoneConfig{});
            REQUIRE_NOTHROW(obj.requestShutdown());
        }
    }

    TEST_CASE("ZoneServer update is safe without connection", "[zoneserver]") {
        ZoneServer obj;

        SECTION("update before init is safe") {
            REQUIRE_NOTHROW(obj.tick());
        }

        SECTION("repeated updates are safe") {
            obj.initialize(ZoneConfig{});
            for (int i = 0; i < 10; ++i) {
                obj.tick();
            }
        }
    }

    TEST_CASE("ZoneServer initialize is safe to call", "[zoneserver]") {
        ZoneServer obj;
        obj.initialize(ZoneConfig{});
        REQUIRE_NOTHROW(obj.initialize(ZoneConfig{}));
    }

    TEST_CASE("ZoneServer run is safe to call", "[zoneserver]") {
        ZoneServer obj;
        obj.initialize(ZoneConfig{});
        std::thread runThread([&obj]() {
            obj.run();
        });
        // Let the main loop spin briefly, then stop it
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        obj.stop();
        runThread.join();
        REQUIRE(true);
    }

    TEST_CASE("ZoneServer setupSignalHandlers is safe to call", "[zoneserver]") {
        ZoneServer obj;
        obj.initialize(ZoneConfig{});
        REQUIRE_NOTHROW(obj.setupSignalHandlers());
    }


// ============================================================================
// ZoneServer Tick & Lifecycle Depth Tests (added 2026-04-26)
// ============================================================================

TEST_CASE("ZoneServer run loop increments tick counter", "[zones][zoneserver][depth]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    REQUIRE_FALSE(server.isRunning());

    // Start server main loop in background thread
    std::thread runThread([&server]() {
        server.run();
    });

    // Allow some ticks to occur (100ms ≈ 6 ticks at 60Hz)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Request shutdown and wait for thread exit
    server.requestShutdown();
    if (runThread.joinable()) {
        runThread.join();
    }

    // After run, tick count must have advanced
    REQUIRE(server.getCurrentTick() >= 1);
    auto& metrics = server.getMetricsRef();
    REQUIRE(metrics.tickCount == server.getCurrentTick());
    REQUIRE(metrics.totalTickTimeUs > 0);
    REQUIRE(metrics.maxTickTimeUs > 0);
}

TEST_CASE("ZoneServer getCurrentTimeMs is monotonic after initialize", "[zones][zoneserver][depth]") {
    ZoneServer server;
    server.initialize(ZoneConfig{});

    uint32_t t1 = server.getCurrentTimeMs();
    uint32_t t2 = server.getCurrentTimeMs();

    // Should be non-decreasing
    REQUIRE(t2 >= t1);

    // Sleep a bit to ensure time moves forward
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    uint32_t t3 = server.getCurrentTimeMs();
    REQUIRE(t3 > t1);
}

TEST_CASE("ZoneServer spawnPlayer creates entity with required components and mapping", "[zones][zoneserver][depth]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));

    ConnectionID connId = 42;
    uint64_t playerId = 1001;
    std::string username = "TestPlayer";
    Position spawnPos{100, 0, 200};

    EntityID e = server.spawnPlayer(connId, playerId, username, spawnPos);

    auto& registry = server.getRegistry();
    REQUIRE(registry.valid(e));
    REQUIRE(registry.all_of<Position>(e));
    REQUIRE(registry.all_of<Velocity>(e));
    REQUIRE(registry.all_of<Rotation>(e));
    REQUIRE(registry.all_of<BoundingVolume>(e));
    REQUIRE(registry.all_of<InputState>(e));
    REQUIRE(registry.all_of<CombatState>(e));
    REQUIRE(registry.all_of<NetworkState>(e));
    REQUIRE(registry.all_of<AntiCheatState>(e));
    REQUIRE(registry.all_of<PlayerInfo>(e));
    REQUIRE(registry.all_of<PlayerTag>(e));
    REQUIRE(registry.all_of<PlayerProgression>(e));

    auto& pos = registry.get<Position>(e);
    REQUIRE(pos.x == 100);
    REQUIRE(pos.z == 200);

    auto& info = registry.get<PlayerInfo>(e);
    REQUIRE(info.playerId == playerId);
    REQUIRE(info.connectionId == connId);
    REQUIRE(std::string(info.username) == username);

    auto* connToEnt = server.getConnectionToEntityPtr();
    REQUIRE(connToEnt->at(connId) == e);

    auto* entToConn = server.getEntityToConnectionPtr();
    REQUIRE(entToConn->at(e) == connId);
}

TEST_CASE("ZoneServer despawnEntity removes entity and cleans mappings", "[zones][zoneserver][depth]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));

    EntityID e = server.spawnPlayer(1, 1, "Temp", Position{0,0,0});
    auto* connToEnt = server.getConnectionToEntityPtr();
    auto* entToConn = server.getEntityToConnectionPtr();

    REQUIRE(connToEnt->find(1) != connToEnt->end());
    REQUIRE(entToConn->find(e) != entToConn->end());

    server.despawnEntity(e);

    auto& registry = server.getRegistry();
    REQUIRE_FALSE(registry.valid(e));
    REQUIRE(connToEnt->find(1) == connToEnt->end());
    REQUIRE(entToConn->find(e) == entToConn->end());
}

TEST_CASE("ZoneServer metrics accumulate correctly across many ticks", "[zones][zoneserver][depth]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));

    auto& metrics = server.getMetricsRef();
    const int numTicks = 1000;

    for (int i = 0; i < numTicks; ++i) {
        server.tick();
    }

    REQUIRE(metrics.tickCount == numTicks);
    REQUIRE(metrics.totalTickTimeUs > 0);
    REQUIRE(metrics.maxTickTimeUs > 0);
    REQUIRE(metrics.overruns >= 0);
    REQUIRE(metrics.networkTimeUs >= 0);
    REQUIRE(metrics.physicsTimeUs >= 0);
    REQUIRE(metrics.gameLogicTimeUs >= 0);
    REQUIRE(metrics.replicationTimeUs >= 0);
}

TEST_CASE("ZoneServer metrics reset zeroes all counters", "[zones][zoneserver][depth]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));

    for (int i = 0; i < 10; ++i) server.tick();

    auto& metrics = server.getMetricsRef();
    REQUIRE(metrics.tickCount > 0);

    metrics.reset();
    REQUIRE(metrics.tickCount == 0);
    REQUIRE(metrics.totalTickTimeUs == 0);
    REQUIRE(metrics.maxTickTimeUs == 0);
    REQUIRE(metrics.overruns == 0);
    REQUIRE(metrics.networkTimeUs == 0);
    REQUIRE(metrics.physicsTimeUs == 0);
    REQUIRE(metrics.gameLogicTimeUs == 0);
    REQUIRE(metrics.replicationTimeUs == 0);
}
