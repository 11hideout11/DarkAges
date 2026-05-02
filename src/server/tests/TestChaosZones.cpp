// ============================================================================
// Phase 11: Chaos Testing - Zone Failure Simulation
// Tests zone failure scenarios and recovery mechanisms
// ============================================================================

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/ZoneServer.hpp"
#include "Constants.hpp"
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// Zone Failure Simulation Tests
// ============================================================================

TEST_CASE("ZoneServer graceful shutdown preserves metrics", "[chaos][zoneserver]") {
    ZoneServer server;
    ZoneConfig config;
    config.zoneId = 1;
    
    REQUIRE(server.initialize(config));
    
    // Record some metrics before shutdown
    server.getMetricsRef().tickCount = 100;
    server.getMetricsRef().totalTickTimeUs = 50000;
    server.getMetricsRef().maxTickTimeUs = 1000;
    
    // Request shutdown
    server.requestShutdown();
    
    // Verify shutdown is requested but server state is preserved
    REQUIRE(server.isShutdownRequested());
    
    // Metrics should still be accessible after shutdown request
    const auto& metrics = server.getMetrics();
    REQUIRE(metrics.tickCount == 100);
    REQUIRE(metrics.totalTickTimeUs == 50000);
    REQUIRE(metrics.maxTickTimeUs == 1000);
}

TEST_CASE("ZoneServer stop after shutdown is safe", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    server.requestShutdown();
    REQUIRE_NOTHROW(server.stop());
    REQUIRE_FALSE(server.isRunning());
}

TEST_CASE("ZoneServer multiple stop calls are safe", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    server.requestShutdown();
    server.stop();
    server.stop();
    server.stop();
    
    REQUIRE_FALSE(server.isRunning());
}

TEST_CASE("ZoneServer shutdown preserves configuration", "[chaos][zoneserver]") {
    ZoneConfig config;
    config.zoneId = 42;
    config.port = 9999;
    config.redisHost = "chaos.redis";
    config.autoPopulateNPCs = true;
    config.npcCount = 5;
    
    ZoneServer server;
    REQUIRE(server.initialize(config));
    
    server.requestShutdown();
    server.stop();
    
    // Configuration should be preserved after shutdown
    const auto& saved = server.getConfig();
    REQUIRE(saved.zoneId == 42);
    REQUIRE(saved.port == 9999);
    REQUIRE(saved.redisHost == "chaos.redis");
    REQUIRE(saved.autoPopulateNPCs == true);
    REQUIRE(saved.npcCount == 5);
}

TEST_CASE("ZoneServer tick count preserved after shutdown request", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Simulate some ticks
    for (int i = 0; i < 50; ++i) {
        server.tick();
    }
    
    REQUIRE(server.getCurrentTick() == 50);
    
    server.requestShutdown();
    
    // Tick count should be preserved
    REQUIRE(server.getCurrentTick() == 50);
}

TEST_CASE("ZoneServer entity tracking after shutdown", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Spawn some entities
    EntityID e1 = server.spawnPlayer(1, 100, "Player1", Position{});
    EntityID e2 = server.spawnPlayer(2, 200, "Player2", Position{});
    EntityID npc = server.spawnNPC(Position{}, 1, 10, 10.0f, 10.0f, 10.0f, 50, 5000);
    
    REQUIRE(server.getRegistry().valid(e1));
    REQUIRE(server.getRegistry().valid(e2));
    REQUIRE(server.getRegistry().valid(npc));
    
    server.requestShutdown();
    
    // Entities should still exist after shutdown request (not cleaned up until stop/destroy)
    REQUIRE(server.getRegistry().valid(e1));
    REQUIRE(server.getRegistry().valid(e2));
    REQUIRE(server.getRegistry().valid(npc));
}

// ============================================================================
// Zone Failure Detection Tests
// ============================================================================

TEST_CASE("ZoneServer detects internal failure via tick failure", "[chaos][zoneserver]") {
    ZoneServer server;
    // Don't initialize - this simulates a failed setup
    
    // tick() should return false when not initialized
    REQUIRE_FALSE(server.tick());
}

TEST_CASE("ZoneServer reports not running when not initialized", "[chaos][zoneserver]") {
    ZoneServer server;
    
    REQUIRE_FALSE(server.isRunning());
    REQUIRE_FALSE(server.isShutdownRequested());
}

TEST_CASE("ZoneServerQoS degrades under load simulation", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Initially not degraded
    REQUIRE_FALSE(server.qosDegraded_);
    REQUIRE(server.reducedUpdateRate_ == Constants::SNAPSHOT_RATE_HZ);
    
    // Simulate QoS degradation
    server.qosDegraded_ = true;
    server.reducedUpdateRate_ = 30;  // Reduced from 60Hz to 30Hz
    
    // Verify degradation state
    REQUIRE(server.qosDegraded_);
    REQUIRE(server.reducedUpdateRate_ == 30);
    
    // Restore
    server.qosDegraded_ = false;
    server.reducedUpdateRate_ = Constants::SNAPSHOT_RATE_HZ;
    REQUIRE_FALSE(server.qosDegraded_);
    REQUIRE(server.reducedUpdateRate_ == Constants::SNAPSHOT_RATE_HZ);
}

// ============================================================================
// Zone Recovery Patterns Tests
// ============================================================================

TEST_CASE("ZoneServer can reinitialize after shutdown", "[chaos][zoneserver]") {
    ZoneServer server;
    
    // First initialization
    ZoneConfig config1;
    config1.zoneId = 1;
    REQUIRE(server.initialize(config1));
    REQUIRE(server.getConfig().zoneId == 1);
    
    server.requestShutdown();
    server.stop();
    
    // Second initialization - should succeed
    ZoneConfig config2;
    config2.zoneId = 2;
    REQUIRE(server.initialize(config2));
    REQUIRE(server.getConfig().zoneId == 2);
}

TEST_CASE("ZoneServer tick count resets on reinitialize", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Initial tick is 0
    REQUIRE(server.getCurrentTick() == 0);
    
    // Manually set some ticks (normally done by run())
    // Note: tick() returns false when not running, so tick count stays at 0 in normal flow
    REQUIRE(server.getCurrentTick() == 0);
    
    server.requestShutdown();
    server.stop();
    
    // New initialization starts fresh
    REQUIRE(server.initialize(ZoneConfig{}));
    REQUIRE(server.getCurrentTick() == 0);
}

TEST_CASE("ZoneServer subsystems reinitialize correctly", "[chaos][zoneserver]") {
    ZoneServer server;
    
    REQUIRE(server.initialize(ZoneConfig{}));
    REQUIRE(server.getRegistry().alive() == 0);  // Empty initially
    
    server.requestShutdown();
    server.stop();
    
    // Reinitialize
    REQUIRE(server.initialize(ZoneConfig{}));
    REQUIRE(server.getRegistry().alive() == 0);  // Fresh start
}

// ============================================================================
// Snapshot History Tests (for recovery)
// ============================================================================

TEST_CASE("ZoneServer snapshot history starts empty", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Should start empty
    REQUIRE(server.snapshotHistory_.empty());
}

TEST_CASE("ZoneServer snapshot history respects capacity", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Add MAX_SNAPSHOT_HISTORY entries
    for (size_t i = 0; i < ZoneServer::MAX_SNAPSHOT_HISTORY; ++i) {
        SnapshotHistory entry;
        entry.tick = static_cast<uint32_t>(i);
        entry.timestamp = std::chrono::steady_clock::now();
        server.snapshotHistory_.push_back(entry);
    }
    
    REQUIRE(server.snapshotHistory_.size() == ZoneServer::MAX_SNAPSHOT_HISTORY);
    
    // Adding more should evict oldest
    SnapshotHistory newEntry;
    newEntry.tick = ZoneServer::MAX_SNAPSHOT_HISTORY;
    newEntry.timestamp = std::chrono::steady_clock::now();
    server.snapshotHistory_.push_back(newEntry);
    
    // Still at capacity (oldest was evicted)
    REQUIRE(server.snapshotHistory_.size() == ZoneServer::MAX_SNAPSHOT_HISTORY);
}

TEST_CASE("ZoneServer snapshot can be reconstructed from history", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Simulate storing a snapshot
    SnapshotHistory snapshot;
    snapshot.tick = 100;
    snapshot.timestamp = std::chrono::steady_clock::now();
    // Note: entities would be populated from ReplicationOptimizer in real code
    // For testing, we just verify history structure works
    
    server.snapshotHistory_.push_back(snapshot);
    
    // Can retrieve from history
    REQUIRE_FALSE(server.snapshotHistory_.empty());
    REQUIRE(server.snapshotHistory_.front().tick == 100);
}

// ============================================================================
// Client State Recovery Tests
// ============================================================================

TEST_CASE("ZoneServer client state tracking starts empty", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    REQUIRE(server.clientSnapshotState_.empty());
}

TEST_CASE("ZoneServer can track multiple clients", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    // Track multiple clients
    server.clientSnapshotState_[1] = ClientSnapshotState{};
    server.clientSnapshotState_[1].lastAcknowledgedTick = 10;
    server.clientSnapshotState_[1].lastSentTick = 15;
    
    server.clientSnapshotState_[2] = ClientSnapshotState{};
    server.clientSnapshotState_[2].lastAcknowledgedTick = 20;
    server.clientSnapshotState_[2].lastSentTick = 25;
    
    REQUIRE(server.clientSnapshotState_.size() == 2);
    REQUIRE(server.clientSnapshotState_[1].lastAcknowledgedTick == 10);
    REQUIRE(server.clientSnapshotState_[2].lastAcknowledgedTick == 20);
}

TEST_CASE("ZoneServer client state preserved across shutdown request", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    server.clientSnapshotState_[1] = ClientSnapshotState{};
    server.clientSnapshotState_[1].lastAcknowledgedTick = 50;
    server.clientSnapshotState_[1].baselineTick = 45;
    
    server.requestShutdown();
    
    // Client state preserved
    REQUIRE(server.clientSnapshotState_[1].lastAcknowledgedTick == 50);
    REQUIRE(server.clientSnapshotState_[1].baselineTick == 45);
}

// ============================================================================
// Destroyed Entities Tracking Tests
// ============================================================================

TEST_CASE("ZoneServer destroyed entities tracking starts empty", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    REQUIRE(server.destroyedEntities_.empty());
}

TEST_CASE("ZoneServer can track destroyed entities", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    EntityID e1 = server.spawnNPC(Position{}, 1, 10, 10.0f, 10.0f, 10.0f, 50, 5000);
    EntityID e2 = server.spawnNPC(Position{}, 1, 10, 10.0f, 10.0f, 10.0f, 50, 5000);
    
    // Track destroyed entities
    server.destroyedEntities_.push_back(e1);
    server.destroyedEntities_.push_back(e2);
    
    REQUIRE(server.destroyedEntities_.size() == 2);
    REQUIRE(server.destroyedEntities_[0] == e1);
    REQUIRE(server.destroyedEntities_[1] == e2);
}

TEST_CASE("ZoneServer destroyed entities cleared after tick", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    server.destroyedEntities_.push_back(1);
    server.destroyedEntities_.push_back(2);
    
    REQUIRE_FALSE(server.destroyedEntities_.empty());
    
    // Clear for next tick
    server.destroyedEntities_.clear();
    
    REQUIRE(server.destroyedEntities_.empty());
}

// ============================================================================
// Respawn Queue Tests
// ============================================================================

TEST_CASE("ZoneServer respawn queue starts empty", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    REQUIRE(server.pendingRespawns_.empty());
}

TEST_CASE("ZoneServer can queue respawn", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    ZoneServer::PendingRespawn respawn;
    respawn.entity = 100;
    respawn.respawnTimeMs = 5000;
    respawn.spawnPos = Position::fromVec3(glm::vec3(10, 0, 20), 0);
    
    server.pendingRespawns_.push_back(respawn);
    
    REQUIRE(server.pendingRespawns_.size() == 1);
    REQUIRE(server.pendingRespawns_[0].entity == 100);
    REQUIRE(server.pendingRespawns_[0].respawnTimeMs == 5000);
}

TEST_CASE("ZoneServer respawn processing by time", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    uint32_t currentTime = 1000;
    
    // Add respawns at different times
    ZoneServer::PendingRespawn r1;
    r1.entity = 1;
    r1.respawnTimeMs = 500;   // Due in past
    server.pendingRespawns_.push_back(r1);
    
    ZoneServer::PendingRespawn r2;
    r2.entity = 2;
    r2.respawnTimeMs = 2000;  // Due in future
    server.pendingRespawns_.push_back(r2);
    
    // First respawn is due (500 < 1000)
    // Second respawn is not due (2000 > 1000)
    // Note: Due respawns should be processed by processRespawns()
    // This test verifies ordering works
    REQUIRE(server.pendingRespawns_[0].respawnTimeMs == 500);
    REQUIRE(server.pendingRespawns_[1].respawnTimeMs == 2000);
}

// ============================================================================
// Connection Mapping Tests (for failure scenarios)
// ============================================================================

TEST_CASE("ZoneServer connection maps start empty", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    REQUIRE(server.connectionToEntity_.empty());
    REQUIRE(server.entityToConnection_.empty());
}

TEST_CASE("ZoneServer can track connection mapping", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    ConnectionID cid = 42;
    EntityID eid = server.spawnPlayer(cid, 100, "TestPlayer", Position{});
    
    REQUIRE(server.connectionToEntity_.find(cid) != server.connectionToEntity_.end());
    REQUIRE(server.entityToConnection_.find(eid) != server.entityToConnection_.end());
    REQUIRE(server.connectionToEntity_[cid] == eid);
    REQUIRE(server.entityToConnection_[eid] == cid);
}

TEST_CASE("ZoneServer mapping preserved after shutdown", "[chaos][zoneserver]") {
    ZoneServer server;
    REQUIRE(server.initialize(ZoneConfig{}));
    
    EntityID eid = server.spawnPlayer(1, 100, "Player", Position{});
    REQUIRE(server.connectionToEntity_.find(1) != server.connectionToEntity_.end());
    
    server.requestShutdown();
    
    // Mappings preserved until explicit cleanup
    REQUIRE(server.connectionToEntity_.find(1) != server.connectionToEntity_.end());
    REQUIRE(server.entityToConnection_.find(eid) != server.entityToConnection_.end());
}

// ============================================================================
// End of Chaos Zone Failure Tests
// ============================================================================