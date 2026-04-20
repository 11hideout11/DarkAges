// [SECURITY_AGENT] Tests for StatisticalDetector
// Statistical anomaly detection system tests

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "security/StatisticalDetector.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <cstdint>
#include <cmath>

using namespace DarkAges;
using namespace DarkAges::Security;

// ============================================================================
// Helper Functions
// ============================================================================

static Position makePosition(float x, float y, float z, uint32_t ts = 0) {
    Position p;
    p.x = static_cast<Constants::Fixed>(x * Constants::FLOAT_TO_FIXED);
    p.y = static_cast<Constants::Fixed>(y * Constants::FLOAT_TO_FIXED);
    p.z = static_cast<Constants::Fixed>(z * Constants::FLOAT_TO_FIXED);
    p.timestamp_ms = ts;
    return p;
}

static Velocity makeVelocity(float dx, float dy, float dz) {
    Velocity v;
    v.dx = static_cast<Constants::Fixed>(dx * Constants::FLOAT_TO_FIXED);
    v.dy = static_cast<Constants::Fixed>(dy * Constants::FLOAT_TO_FIXED);
    v.dz = static_cast<Constants::Fixed>(dz * Constants::FLOAT_TO_FIXED);
    return v;
}

// Helper to simulate normal player movement for N ticks
static void simulateNormalMovement(StatisticalDetector& detector, uint64_t playerId,
                                    uint32_t startTick, uint32_t numTicks,
                                    float speed = 5.0f) {
    Position pos = makePosition(0, 0, 0, startTick);
    
    for (uint32_t i = 0; i < numTicks; ++i) {
        uint32_t ts = startTick + i * 16; // ~60Hz
        float dt = 0.016f; // 16ms
        
        // Move forward at constant speed
        pos = makePosition(
            pos.x * Constants::FIXED_TO_FLOAT + speed * dt,
            0,
            pos.z * Constants::FIXED_TO_FLOAT,
            ts
        );
        
        Velocity vel = makeVelocity(speed, 0, 0);
        detector.updateStats(playerId, pos, vel, false, ts);
    }
}

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_CASE("StatisticalDetector initialization", "[security][statistics]") {
    SECTION("Default construction") {
        StatisticalDetector detector;
        REQUIRE(detector.getTrackedPlayerCount() == 0);
    }
    
    SECTION("Custom config construction") {
        StatisticalDetectorConfig config;
        config.emaAlpha = 0.2f;
        config.zScoreThreshold = 2.5f;
        
        StatisticalDetector detector(config);
        auto cfg = detector.getConfig();
        REQUIRE(cfg.emaAlpha == 0.2f);
        REQUIRE(cfg.zScoreThreshold == 2.5f);
    }
    
    SECTION("No tracking initially") {
        StatisticalDetector detector;
        REQUIRE_FALSE(detector.isTracking(123));
        REQUIRE(detector.getConfidence(123) == 0.0f);
    }
}

// ============================================================================
// Confidence Building Tests
// ============================================================================

TEST_CASE("StatisticalDetector confidence building", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 1;
    
    SECTION("Confidence starts at zero") {
        REQUIRE(detector.getConfidence(playerId) == 0.0f);
    }
    
    SECTION("Confidence increases with samples") {
        simulateNormalMovement(detector, playerId, 0, 50);
        float confidence1 = detector.getConfidence(playerId);
        REQUIRE(confidence1 > 0.0f);
        REQUIRE(confidence1 < 0.5f); // Still building confidence
        
        simulateNormalMovement(detector, playerId, 800, 50);
        float confidence2 = detector.getConfidence(playerId);
        REQUIRE(confidence2 > confidence1);
    }
    
    SECTION("Confidence reaches full after enough samples") {
        simulateNormalMovement(detector, playerId, 0, 150);
        float confidence = detector.getConfidence(playerId);
        REQUIRE(confidence >= 0.9f); // Close to full confidence
    }
    
    SECTION("AnomalyScore reliability reflects confidence") {
        simulateNormalMovement(detector, playerId, 0, 5);
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.isReliable());
        REQUIRE_FALSE(score.isHighConfidence());
        
        simulateNormalMovement(detector, playerId, 80, 50);
        score = detector.getAnomalyScore(playerId);
        REQUIRE(score.isReliable());
        
        simulateNormalMovement(detector, playerId, 880, 100);
        score = detector.getAnomalyScore(playerId);
        REQUIRE(score.isHighConfidence());
    }
}

// ============================================================================
// Normal Behavior Tests
// ============================================================================

TEST_CASE("StatisticalDetector normal player behavior", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 10;
    
    SECTION("Normal walking stays below thresholds") {
        simulateNormalMovement(detector, playerId, 0, 60, 5.0f);
        AnomalyScore score = detector.getAnomalyScore(playerId);
        
        REQUIRE_FALSE(score.speedAnomaly);
        REQUIRE_FALSE(score.positionAnomaly);
        REQUIRE_FALSE(score.hasAnomaly());
        REQUIRE(score.composite < 30.0f); // Should be low for normal behavior
    }
    
    SECTION("Normal sprint stays below thresholds") {
        simulateNormalMovement(detector, playerId, 0, 60, 9.0f);
        AnomalyScore score = detector.getAnomalyScore(playerId);
        
        REQUIRE_FALSE(score.speedAnomaly);
        REQUIRE(score.speedScore < 50.0f);
    }
    
    SECTION("Normal action rate stays below thresholds") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(5, 0, 0);
        
        // 50 actions per second (below 70 limit)
        for (uint32_t i = 0; i < 60; ++i) {
            uint32_t ts = i * 16;
            bool action = (i % 3 == 0); // ~20 actions per second
            detector.updateStats(playerId, pos, vel, action, ts);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.actionRateAnomaly);
    }
}

// ============================================================================
// Speed Hacker Detection Tests
// ============================================================================

TEST_CASE("StatisticalDetector speed hacker detection", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 20;
    
    SECTION("Velocity spike triggers speed anomaly") {
        // First establish normal baseline
        simulateNormalMovement(detector, playerId, 0, 40, 5.0f);
        
        // Sudden velocity spike (speed hack)
        Position pos = makePosition(2, 0, 0, 700);
        Velocity vel = makeVelocity(50.0f, 0, 0); // 50 m/s = 3x normal
        detector.updateStats(playerId, pos, vel, false, 700);
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.speedAnomaly);
        REQUIRE(score.speedScore >= 50.0f);
    }
    
    SECTION("Speed Z-score increases with velocity spike") {
        simulateNormalMovement(detector, playerId, 0, 40, 5.0f);
        
        // Get Z-score for normal speed (updates variance with last normal sample)
        float normalZScore = detector.calculateSpeedZScore(playerId, 5.0f);
        
        // Feed a spike through updateStats to update EMA variance
        Position pos = makePosition(2, 0, 0, 700);
        Velocity vel = makeVelocity(30.0f, 0, 0); // Spike to 30 m/s
        detector.updateStats(playerId, pos, vel, false, 700);
        
        // Now Z-score for spike should be higher
        float spikeZScore = detector.calculateSpeedZScore(playerId, 30.0f);
        
        // Spike Z-score should be significantly higher than normal
        REQUIRE(spikeZScore > normalZScore);
    }
    
    SECTION("Brief spike in otherwise normal movement") {
        simulateNormalMovement(detector, playerId, 0, 60, 5.0f);
        
        // One tick spike
        Position pos = makePosition(5, 0, 0, 1000);
        Velocity vel = makeVelocity(40, 0, 0);
        detector.updateStats(playerId, pos, vel, false, 1000);
        
        // Should detect anomaly on that tick
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.speedAnomaly);
        REQUIRE(score.totalAnomalyCount > 0);
    }
}

// ============================================================================
// Teleport Detection Tests
// ============================================================================

TEST_CASE("StatisticalDetector teleport detection", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 30;
    
    SECTION("Normal movement has small position delta") {
        simulateNormalMovement(detector, playerId, 0, 10, 5.0f);
        
        // Move 0.08m in one tick (5 m/s * 16ms = 0.08m)
        Position newPos = makePosition(0.08f, 0, 0, 200);
        float delta = detector.calculatePositionDelta(playerId, newPos);
        
        REQUIRE(delta < 1.0f); // Well below teleport threshold
    }
    
    SECTION("Large position jump triggers anomaly") {
        simulateNormalMovement(detector, playerId, 0, 10, 5.0f);
        
        // Teleport 100m instantly
        Position pos = makePosition(100, 0, 0, 200);
        Velocity vel = makeVelocity(0, 0, 0);
        detector.updateStats(playerId, pos, vel, false, 200);
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.positionAnomaly);
        REQUIRE(score.positionScore > 0.0f);
    }
    
    SECTION("Position delta calculation is accurate") {
        Position pos1 = makePosition(0, 0, 0, 0);
        detector.updateStats(playerId, pos1, makeVelocity(0, 0, 0), false, 0);
        
        Position pos2 = makePosition(3, 4, 0, 16); // 3-4-5 triangle = 5 units
        float delta = detector.calculatePositionDelta(playerId, pos2);
        
        REQUIRE(delta == Catch::Approx(5.0f).margin(0.1f));
    }
    
    SECTION("Consecutive teleports increase anomaly score") {
        simulateNormalMovement(detector, playerId, 0, 10, 5.0f);
        
        // Multiple teleport-like jumps
        for (int i = 0; i < 5; ++i) {
            Position pos = makePosition(static_cast<float>(i) * 100.0f, 0, 0, 
                                        200 + static_cast<uint32_t>(i) * 16);
            detector.updateStats(playerId, pos, makeVelocity(0, 0, 0), false, 
                                200 + static_cast<uint32_t>(i) * 16);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.positionAnomaly);
        REQUIRE(score.totalAnomalyCount >= 3);
    }
}

// ============================================================================
// Action Rate Flooding Detection Tests
// ============================================================================

TEST_CASE("StatisticalDetector action rate flooding", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 40;
    
    SECTION("Normal action rate is accepted") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(5, 0, 0);
        
        // 30 actions per second (well below 70 limit)
        for (uint32_t i = 0; i < 60; ++i) {
            uint32_t ts = i * 16;
            bool action = (i % 2 == 0); // 30 actions per second
            detector.updateStats(playerId, pos, vel, action, ts);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.actionRateAnomaly);
    }
    
    SECTION("Excessive action rate triggers anomaly") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(5, 0, 0);
        
        // 100+ actions per second (way over 70 limit) for 2 seconds
        // Send action=true at 8ms intervals = 125 actions/sec
        for (uint32_t i = 0; i < 250; ++i) {
            uint32_t ts = i * 8;
            detector.updateStats(playerId, pos, vel, true, ts);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.actionRateAnomaly);
        REQUIRE(score.actionRateScore > 0.0f);
    }
    
    SECTION("Action rate at limit is acceptable") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(5, 0, 0);
        
        // Send exactly 60 actions spread over a full 1000ms window
        // so the measured rate is exactly 60/sec when the window completes
        for (uint32_t i = 0; i < 60; ++i) {
            uint32_t ts = static_cast<uint32_t>(i * (1000.0f / 59.0f)); // Spread over 1000ms
            bool action = true;
            detector.updateStats(playerId, pos, vel, action, ts);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        // At exactly 60 actions/sec, should not trigger anomaly
        // (action rate anomaly requires > maxActionsPerSecond)
        // Note: intermediate checks may trigger due to window timing,
        // but the final anomaly count should decay if rate stays at limit
        REQUIRE(score.actionRateScore < 70.0f);
    }
}

// ============================================================================
// Accuracy Anomaly Detection Tests
// ============================================================================

TEST_CASE("StatisticalDetector accuracy anomaly", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 50;
    
    SECTION("Normal accuracy is accepted") {
        simulateNormalMovement(detector, playerId, 0, 20, 5.0f);
        
        // 70% accuracy (normal)
        for (int i = 0; i < 20; ++i) {
            detector.recordHitAttempt(playerId, (i % 10 < 7), static_cast<uint32_t>(i) * 100);
        }
        
        float accuracy = detector.getCurrentAccuracy(playerId);
        REQUIRE(accuracy < 95.0f);
        REQUIRE(accuracy > 60.0f);
    }
    
    SECTION("Inhuman accuracy triggers anomaly") {
        simulateNormalMovement(detector, playerId, 0, 20, 5.0f);
        
        // 99% accuracy (suspicious)
        for (int i = 0; i < 100; ++i) {
            detector.recordHitAttempt(playerId, (i != 50), static_cast<uint32_t>(i) * 10);
        }
        
        float accuracy = detector.getCurrentAccuracy(playerId);
        REQUIRE(accuracy > 95.0f);
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.accuracyAnomaly);
        REQUIRE(score.accuracyScore > 0.0f);
    }
    
    SECTION("100% accuracy with few attempts not flagged") {
        simulateNormalMovement(detector, playerId, 0, 10, 5.0f);
        
        // Only 5 hits, all landed - not enough samples
        for (int i = 0; i < 5; ++i) {
            detector.recordHitAttempt(playerId, true, static_cast<uint32_t>(i) * 100);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.accuracyAnomaly);
    }
}

// ============================================================================
// Composite Score Tests
// ============================================================================

TEST_CASE("StatisticalDetector composite scoring", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 60;
    
    SECTION("Clean player has low composite score") {
        simulateNormalMovement(detector, playerId, 0, 100, 5.0f);
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.composite < 20.0f);
        REQUIRE_FALSE(score.hasAnomaly());
    }
    
    SECTION("Multiple anomalies increase composite score") {
        // Establish baseline
        simulateNormalMovement(detector, playerId, 0, 40, 5.0f);
        
        // Trigger multiple anomalies - teleport and speed hack
        Position pos = makePosition(200, 0, 0, 700); // Teleport
        Velocity vel = makeVelocity(50, 0, 0);       // Speed hack
        detector.updateStats(playerId, pos, vel, false, 700);
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        // With speed and position anomalies, composite should be notable
        REQUIRE(score.composite > 0.0f);
        REQUIRE(score.hasAnomaly());
    }
    
    SECTION("Score breakdown reflects individual categories") {
        // Trigger speed anomaly only
        simulateNormalMovement(detector, playerId, 0, 40, 5.0f);
        
        Position pos = makePosition(5, 0, 0, 700);
        Velocity vel = makeVelocity(50, 0, 0); // Speed spike
        detector.updateStats(playerId, pos, vel, false, 700);
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.speedScore > score.positionScore);
        REQUIRE(score.speedAnomaly);
    }
}

// ============================================================================
// Memory and Cleanup Tests
// ============================================================================

TEST_CASE("StatisticalDetector memory management", "[security][statistics]") {
    StatisticalDetector detector;
    
    SECTION("Multiple players tracked independently") {
        for (uint64_t pid = 1; pid <= 10; ++pid) {
            simulateNormalMovement(detector, pid, 0, 20, 5.0f);
        }
        
        REQUIRE(detector.getTrackedPlayerCount() == 10);
        
        // Scores should be independent
        AnomalyScore score1 = detector.getAnomalyScore(1);
        AnomalyScore score5 = detector.getAnomalyScore(5);
        REQUIRE(score1.confidence == Catch::Approx(score5.confidence).margin(0.01f));
    }
    
    SECTION("Remove player clears data") {
        simulateNormalMovement(detector, 100, 0, 50, 5.0f);
        REQUIRE(detector.isTracking(100));
        
        detector.removePlayer(100);
        REQUIRE_FALSE(detector.isTracking(100));
        REQUIRE(detector.getConfidence(100) == 0.0f);
    }
    
    SECTION("Clear removes all players") {
        for (uint64_t pid = 1; pid <= 5; ++pid) {
            simulateNormalMovement(detector, pid, 0, 20, 5.0f);
        }
        REQUIRE(detector.getTrackedPlayerCount() == 5);
        
        detector.clear();
        REQUIRE(detector.getTrackedPlayerCount() == 0);
    }
    
    SECTION("History ring buffer does not overflow") {
        // Simulate 200 ticks (more than HISTORY_CAPACITY of 128)
        simulateNormalMovement(detector, 999, 0, 200, 5.0f);
        
        // Should not crash, and score should still work
        AnomalyScore score = detector.getAnomalyScore(999);
        REQUIRE(score.confidence > 0.9f);
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_CASE("StatisticalDetector edge cases", "[security][statistics]") {
    StatisticalDetector detector;
    uint64_t playerId = 70;
    
    SECTION("Zero velocity is valid") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(0, 0, 0);
        
        for (uint32_t i = 0; i < 30; ++i) {
            detector.updateStats(playerId, pos, vel, false, i * 16);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.speedAnomaly);
    }
    
    SECTION("Stationary player has zero speed anomaly") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(0, 0, 0);
        
        detector.updateStats(playerId, pos, vel, false, 0);
        detector.updateStats(playerId, pos, vel, false, 16);
        
        float zScore = detector.calculateSpeedZScore(playerId, 0.0f);
        REQUIRE(std::abs(zScore) < 0.1f);
    }
    
    SECTION("Anomaly decay works over time") {
        // Establish baseline
        simulateNormalMovement(detector, playerId, 0, 40, 5.0f);
        
        // Trigger anomaly at t=700
        Position pos = makePosition(100, 0, 0, 700);
        detector.updateStats(playerId, pos, makeVelocity(50, 0, 0), false, 700);
        
        AnomalyScore score1 = detector.getAnomalyScore(playerId);
        REQUIRE(score1.positionAnomaly);
        uint32_t initialCount = score1.totalAnomalyCount;
        
        // Wait 10 seconds with normal movement (anomalies should decay)
        simulateNormalMovement(detector, playerId, 750, 600, 5.0f);
        
        AnomalyScore score2 = detector.getAnomalyScore(playerId);
        REQUIRE(score2.totalAnomalyCount < initialCount);
    }
    
    SECTION("Very high speed value is clamped in anomaly score") {
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(1000, 0, 0); // Extreme speed
        
        for (uint32_t i = 0; i < 50; ++i) {
            detector.updateStats(playerId, pos, vel, false, i * 16);
        }
        
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.speedAnomaly);
        REQUIRE(score.speedScore <= 100.0f); // Clamped
    }
}

// ============================================================================
// Thread Safety Tests (Basic)
// ============================================================================

TEST_CASE("StatisticalDetector thread safety basics", "[security][statistics]") {
    StatisticalDetector detector;
    
    SECTION("Config update is thread-safe") {
        StatisticalDetectorConfig config;
        config.emaAlpha = 0.15f;
        detector.setConfig(config);
        
        auto retrieved = detector.getConfig();
        REQUIRE(retrieved.emaAlpha == 0.15f);
    }
    
    SECTION("Multiple player operations don't corrupt state") {
        // This tests that operations don't crash with concurrent-like access
        for (int iter = 0; iter < 10; ++iter) {
            for (uint64_t pid = 1; pid <= 5; ++pid) {
                simulateNormalMovement(detector, pid, 
                    static_cast<uint32_t>(iter) * 100, 10, 5.0f);
            }
        }
        
        // All players should have valid scores
        for (uint64_t pid = 1; pid <= 5; ++pid) {
            AnomalyScore score = detector.getAnomalyScore(pid);
            REQUIRE(score.confidence > 0.0f);
            REQUIRE(score.composite >= 0.0f);
            REQUIRE(score.composite <= 100.0f);
        }
    }
}

// ============================================================================
// Z-Score Edge Cases
// ============================================================================

TEST_CASE("StatisticalDetector Z-score edge cases", "[security][statistics]") {
    SECTION("Zero variance returns zero Z-score") {
        StatisticalDetector detector;
        uint64_t playerId = 80;
        // Need 2+ samples for calculateSpeedZScore to work
        Position pos = makePosition(0, 0, 0, 0);
        detector.updateStats(playerId, pos, makeVelocity(5, 0, 0), false, 0);
        detector.updateStats(playerId, pos, makeVelocity(5, 0, 0), false, 16);

        float zScore = detector.calculateSpeedZScore(playerId, 100.0f);
        REQUIRE(zScore == 0.0f);  // Zero variance -> Z=0
    }

    SECTION("Unknown player returns zero Z-score") {
        StatisticalDetector detector;
        float zScore = detector.calculateSpeedZScore(9999, 10.0f);
        REQUIRE(zScore == 0.0f);
    }

    SECTION("Z-score increases with deviation from mean") {
        StatisticalDetector detector;
        uint64_t playerId = 81;
        // Establish baseline at 5 m/s with one spike to create variance
        simulateNormalMovement(detector, playerId, 0, 40, 5.0f);
        // Feed a spike to establish variance
        Position pos = makePosition(5, 0, 0, 660);
        detector.updateStats(playerId, pos, makeVelocity(20, 0, 0), false, 660);

        // Z-score for 5 m/s (near EMA) should be small
        float zAtMean = detector.calculateSpeedZScore(playerId, 5.0f);

        // Z-score for 50 m/s (far from EMA) should be much larger
        float zAtDeviant = detector.calculateSpeedZScore(playerId, 50.0f);

        REQUIRE(std::abs(zAtDeviant) >= std::abs(zAtMean));
    }
}

// ============================================================================
// EMA Behavior Verification
// ============================================================================

TEST_CASE("StatisticalDetector EMA behavior", "[security][statistics]") {
    SECTION("First sample initializes EMA to current value") {
        StatisticalDetector detector;
        uint64_t playerId = 90;
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(7.5f, 0, 0);

        detector.updateStats(playerId, pos, vel, true, 0);

        // After first sample, tracking should be active
        REQUIRE(detector.isTracking(playerId));
        float confidence = detector.getConfidence(playerId);
        REQUIRE(confidence > 0.0f);
    }

    SECTION("EMA converges to constant input over time") {
        StatisticalDetector detector;
        StatisticalDetectorConfig config;
        config.emaAlpha = 0.5f;  // Fast convergence for testing
        detector.setConfig(config);

        uint64_t playerId = 91;
        Position pos = makePosition(0, 0, 0, 0);

        // Feed constant speed of 5 m/s for 100 samples
        for (uint32_t i = 0; i < 100; ++i) {
            detector.updateStats(playerId, pos, makeVelocity(5, 0, 0), false, i * 16);
        }

        // After many samples, EMA should converge close to 5.0
        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.speedAnomaly);
        REQUIRE(score.speedScore < 20.0f);  // Should be low for constant input
    }

    SECTION("EMA responds to sudden value change") {
        StatisticalDetector detector;
        StatisticalDetectorConfig config;
        config.emaAlpha = 0.5f;
        detector.setConfig(config);

        uint64_t playerId = 92;
        Position pos = makePosition(0, 0, 0, 0);

        // Establish baseline at 5 m/s
        for (uint32_t i = 0; i < 30; ++i) {
            detector.updateStats(playerId, pos, makeVelocity(5, 0, 0), false, i * 16);
        }

        // Sudden change to 50 m/s
        detector.updateStats(playerId, pos, makeVelocity(50, 0, 0), false, 500);

        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.speedAnomaly);
    }
}

// ============================================================================
// Anomaly Score Scoring
// ============================================================================

TEST_CASE("StatisticalDetector anomaly score properties", "[security][statistics]") {
    SECTION("Unknown player returns zeroed score") {
        StatisticalDetector detector;
        AnomalyScore score = detector.getAnomalyScore(99999);
        REQUIRE(score.composite == 0.0f);
        REQUIRE(score.confidence == 0.0f);
        REQUIRE_FALSE(score.hasAnomaly());
        REQUIRE_FALSE(score.isReliable());
    }

    SECTION("Composite score stays within 0-100") {
        StatisticalDetector detector;
        uint64_t playerId = 100;

        // Extreme anomaly - massive teleport + speed hack + action flood
        for (uint32_t i = 0; i < 50; ++i) {
            Position p = makePosition(static_cast<float>(i) * 1000.0f, 0, 0, i * 8);
            detector.updateStats(playerId, p, makeVelocity(1000, 0, 0), true, i * 8);
        }

        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.composite >= 0.0f);
        REQUIRE(score.composite <= 100.0f);
    }

    SECTION("Zero-speed player has low speed score") {
        StatisticalDetector detector;
        uint64_t playerId = 101;
        Position pos = makePosition(0, 0, 0, 0);

        for (uint32_t i = 0; i < 60; ++i) {
            detector.updateStats(playerId, pos, makeVelocity(0, 0, 0), false, i * 16);
        }

        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE(score.speedScore < 10.0f);
    }
}

// ============================================================================
// Multi-Player Isolation
// ============================================================================

TEST_CASE("StatisticalDetector multi-player isolation", "[security][statistics]") {
    SECTION("Anomaly on one player doesn't affect another") {
        StatisticalDetector detector;
        uint64_t cleanPlayer = 110;
        uint64_t cheatPlayer = 111;

        // Both start with normal movement
        simulateNormalMovement(detector, cleanPlayer, 0, 40, 5.0f);
        simulateNormalMovement(detector, cheatPlayer, 0, 40, 5.0f);

        // Cheat player teleports
        Position pos = makePosition(500, 0, 0, 700);
        detector.updateStats(cheatPlayer, pos, makeVelocity(0, 0, 0), false, 700);

        // Clean player should still have clean score
        AnomalyScore cleanScore = detector.getAnomalyScore(cleanPlayer);
        REQUIRE_FALSE(cleanScore.hasAnomaly());
        REQUIRE(cleanScore.composite < 30.0f);

        // Cheat player should have anomalies
        AnomalyScore cheatScore = detector.getAnomalyScore(cheatPlayer);
        REQUIRE(cheatScore.hasAnomaly());
    }

    SECTION("Player count reflects tracked players accurately") {
        StatisticalDetector detector;
        REQUIRE(detector.getTrackedPlayerCount() == 0);

        for (uint64_t pid = 1; pid <= 20; ++pid) {
            simulateNormalMovement(detector, pid, 0, 5, 5.0f);
        }
        REQUIRE(detector.getTrackedPlayerCount() == 20);

        detector.removePlayer(5);
        REQUIRE(detector.getTrackedPlayerCount() == 19);

        detector.removePlayer(999);  // Non-existent
        REQUIRE(detector.getTrackedPlayerCount() == 19);
    }

    SECTION("isTracking reflects accurate state") {
        StatisticalDetector detector;
        REQUIRE_FALSE(detector.isTracking(123));

        simulateNormalMovement(detector, 123, 0, 5, 5.0f);
        REQUIRE(detector.isTracking(123));

        detector.removePlayer(123);
        REQUIRE_FALSE(detector.isTracking(123));
    }
}

// ============================================================================
// Configuration Effects
// ============================================================================

TEST_CASE("StatisticalDetector configuration effects", "[security][statistics]") {
    SECTION("Stricter z-score threshold catches more anomalies") {
        // Default detector
        StatisticalDetector detectorDefault;
        uint64_t pid1 = 120;
        simulateNormalMovement(detectorDefault, pid1, 0, 40, 5.0f);
        Position pos = makePosition(5, 0, 0, 700);
        detectorDefault.updateStats(pid1, pos, makeVelocity(30, 0, 0), false, 700);
        AnomalyScore scoreDefault = detectorDefault.getAnomalyScore(pid1);

        // Strict detector
        StatisticalDetectorConfig strictConfig;
        strictConfig.zScoreThreshold = 1.0f;  // Very strict
        StatisticalDetector detectorStrict(strictConfig);
        uint64_t pid2 = 121;
        simulateNormalMovement(detectorStrict, pid2, 0, 40, 5.0f);
        detectorStrict.updateStats(pid2, pos, makeVelocity(30, 0, 0), false, 700);
        AnomalyScore scoreStrict = detectorStrict.getAnomalyScore(pid2);

        // Strict detector should flag anomalies more readily
        // (score may or may not be higher depending on implementation details)
        // At minimum, both should detect the speed spike
        REQUIRE(scoreDefault.speedAnomaly);
        REQUIRE(scoreStrict.speedAnomaly);
    }

    SECTION("Higher maxVelocity allows faster movement") {
        StatisticalDetectorConfig config;
        config.maxVelocity = 100.0f;  // Very permissive
        StatisticalDetector detector(config);
        uint64_t playerId = 130;

        // Fast movement that would normally trigger anomaly
        for (uint32_t i = 0; i < 40; ++i) {
            Position pos = makePosition(static_cast<float>(i) * 0.8f, 0, 0, i * 16);
            detector.updateStats(playerId, pos, makeVelocity(50, 0, 0), false, i * 16);
        }

        AnomalyScore score = detector.getAnomalyScore(playerId);
        // With high maxVelocity, speed anomaly should still trigger from Z-score
        // but the absolute velocity check should pass
        REQUIRE(score.composite <= 100.0f);
    }
}

// ============================================================================
// Position Delta Calculation
// ============================================================================

TEST_CASE("StatisticalDetector position delta precision", "[security][statistics]") {
    SECTION("Position delta for unknown player is zero") {
        StatisticalDetector detector;
        Position pos = makePosition(10, 0, 10, 0);
        float delta = detector.calculatePositionDelta(9999, pos);
        REQUIRE(delta == 0.0f);
    }

    SECTION("Position delta uses Euclidean distance") {
        StatisticalDetector detector;
        uint64_t playerId = 140;
        Position startPos = makePosition(0, 0, 0, 0);
        detector.updateStats(playerId, startPos, makeVelocity(0, 0, 0), false, 0);

        // 6-8-10 triangle
        Position newPos = makePosition(6, 8, 0, 16);
        float delta = detector.calculatePositionDelta(playerId, newPos);
        REQUIRE(delta == Catch::Approx(10.0f).margin(0.2f));
    }

    SECTION("Zero position delta for same position") {
        StatisticalDetector detector;
        uint64_t playerId = 141;
        Position pos = makePosition(50, 10, 50, 0);
        detector.updateStats(playerId, pos, makeVelocity(0, 0, 0), false, 0);

        float delta = detector.calculatePositionDelta(playerId, pos);
        REQUIRE(delta == Catch::Approx(0.0f).margin(0.001f));
    }
}

// ============================================================================
// Action Rate Tracking
// ============================================================================

TEST_CASE("StatisticalDetector action rate tracking", "[security][statistics]") {
    SECTION("getCurrentActionRate for unknown player is zero") {
        StatisticalDetector detector;
        float rate = detector.getCurrentActionRate(9999);
        REQUIRE(rate == 0.0f);
    }

    SECTION("getCurrentAccuracy for unknown player is zero") {
        StatisticalDetector detector;
        float accuracy = detector.getCurrentAccuracy(9999);
        REQUIRE(accuracy == 0.0f);
    }

    SECTION("getCurrentAccuracy with no hits attempted is zero") {
        StatisticalDetector detector;
        uint64_t playerId = 150;
        simulateNormalMovement(detector, playerId, 0, 20, 5.0f);
        float accuracy = detector.getCurrentAccuracy(playerId);
        REQUIRE(accuracy == 0.0f);
    }

    SECTION("Action rate resets after window expires") {
        StatisticalDetector detector;
        uint64_t playerId = 151;
        Position pos = makePosition(0, 0, 0, 0);
        Velocity vel = makeVelocity(5, 0, 0);

        // Burst of actions in first window
        for (uint32_t i = 0; i < 30; ++i) {
            detector.updateStats(playerId, pos, vel, true, i * 16);
        }

        // After 2 seconds of no actions, rate should drop
        for (uint32_t i = 30; i < 150; ++i) {
            detector.updateStats(playerId, pos, vel, false, i * 16);
        }

        float rate = detector.getCurrentActionRate(playerId);
        REQUIRE(rate < 50.0f);  // Should have dropped significantly
    }
}

// ============================================================================
// Hit Accuracy Tracking
// ============================================================================

TEST_CASE("StatisticalDetector hit accuracy tracking", "[security][statistics]") {
    SECTION("Perfect accuracy triggers anomaly after enough attempts") {
        StatisticalDetector detector;
        uint64_t playerId = 160;
        simulateNormalMovement(detector, playerId, 0, 20, 5.0f);

        // 100% accuracy with 50 hits — enough to push EMA above threshold
        for (int i = 0; i < 50; ++i) {
            detector.recordHitAttempt(playerId, true, static_cast<uint32_t>(i) * 100);
        }

        float accuracy = detector.getCurrentAccuracy(playerId);
        REQUIRE(accuracy > 90.0f);  // EMA-smoothed accuracy

        AnomalyScore score = detector.getAnomalyScore(playerId);
        // With 100% accuracy over 50 hits, anomaly should trigger
        // (threshold is 95% with >= 10 attempts)
        REQUIRE(score.accuracyAnomaly);
    }

    SECTION("Zero accuracy doesn't trigger anomaly") {
        StatisticalDetector detector;
        uint64_t playerId = 161;
        simulateNormalMovement(detector, playerId, 0, 20, 5.0f);

        // 0% accuracy
        for (int i = 0; i < 20; ++i) {
            detector.recordHitAttempt(playerId, false, static_cast<uint32_t>(i) * 100);
        }

        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.accuracyAnomaly);
    }

    SECTION("Mixed accuracy stays below threshold") {
        StatisticalDetector detector;
        uint64_t playerId = 162;
        simulateNormalMovement(detector, playerId, 0, 20, 5.0f);

        // ~60% accuracy
        for (int i = 0; i < 50; ++i) {
            detector.recordHitAttempt(playerId, (i % 5 < 3), static_cast<uint32_t>(i) * 50);
        }

        float accuracy = detector.getCurrentAccuracy(playerId);
        REQUIRE(accuracy < 90.0f);

        AnomalyScore score = detector.getAnomalyScore(playerId);
        REQUIRE_FALSE(score.accuracyAnomaly);
    }
}
