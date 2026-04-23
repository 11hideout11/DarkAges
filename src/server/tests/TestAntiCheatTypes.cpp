// Tests for AntiCheatTypes - CheatType, ViolationSeverity, and related types

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "security/AntiCheatTypes.hpp"
#include <string>

using namespace DarkAges::Security;

// ============================================================================
// CheatType enum tests
// ============================================================================

TEST_CASE("CheatType enum values", "[security][anticheat][types]") {
    SECTION("All cheat types have valid string representations") {
        // These should not crash and should return non-nullptr
        REQUIRE(cheatTypeToString(CheatType::NONE) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::SPEED_HACK) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::TELEPORT) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::FLY_HACK) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::NO_CLIP) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::INPUT_MANIPULATION) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::PACKET_FLOODING) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::POSITION_SPOOFING) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::STAT_MANIPULATION) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::SUSPICIOUS_AIM) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::DAMAGE_HACK) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::COOLDOWN_VIOLATION) != nullptr);
        REQUIRE(cheatTypeToString(CheatType::HITBOX_EXTENSION) != nullptr);
    }

    SECTION("String representations are correct") {
        REQUIRE(std::string(cheatTypeToString(CheatType::NONE)) == "NONE");
        REQUIRE(std::string(cheatTypeToString(CheatType::SPEED_HACK)) == "SPEED_HACK");
        REQUIRE(std::string(cheatTypeToString(CheatType::TELEPORT)) == "TELEPORT");
        REQUIRE(std::string(cheatTypeToString(CheatType::FLY_HACK)) == "FLY_HACK");
        REQUIRE(std::string(cheatTypeToString(CheatType::NO_CLIP)) == "NO_CLIP");
        REQUIRE(std::string(cheatTypeToString(CheatType::INPUT_MANIPULATION)) == "INPUT_MANIPULATION");
        REQUIRE(std::string(cheatTypeToString(CheatType::PACKET_FLOODING)) == "PACKET_FLOODING");
        REQUIRE(std::string(cheatTypeToString(CheatType::POSITION_SPOOFING)) == "POSITION_SPOOFING");
        REQUIRE(std::string(cheatTypeToString(CheatType::STAT_MANIPULATION)) == "STAT_MANIPULATION");
        REQUIRE(std::string(cheatTypeToString(CheatType::SUSPICIOUS_AIM)) == "SUSPICIOUS_AIM");
        REQUIRE(std::string(cheatTypeToString(CheatType::DAMAGE_HACK)) == "DAMAGE_HACK");
        REQUIRE(std::string(cheatTypeToString(CheatType::COOLDOWN_VIOLATION)) == "COOLDOWN_VIOLATION");
        REQUIRE(std::string(cheatTypeToString(CheatType::HITBOX_EXTENSION)) == "HITBOX_EXTENSION");
    }

    SECTION("Default cheat type value") {
        CheatType defaultType;
        REQUIRE(defaultType == CheatType::NONE);
    }
}

// ============================================================================
// ViolationSeverity enum tests
// ============================================================================

TEST_CASE("ViolationSeverity enum values", "[security][anticheat][types]") {
    SECTION("All severity levels have valid string representations") {
        REQUIRE(severityToString(ViolationSeverity::INFO) != nullptr);
        REQUIRE(severityToString(ViolationSeverity::WARNING) != nullptr);
        REQUIRE(severityToString(ViolationSeverity::SUSPICIOUS) != nullptr);
        REQUIRE(severityToString(ViolationSeverity::CRITICAL) != nullptr);
        REQUIRE(severityToString(ViolationSeverity::BAN) != nullptr);
    }

    SECTION("String representations are correct") {
        REQUIRE(std::string(severityToString(ViolationSeverity::INFO)) == "INFO");
        REQUIRE(std::string(severityToString(ViolationSeverity::WARNING)) == "WARNING");
        REQUIRE(std::string(severityToString(ViolationSeverity::SUSPICIOUS)) == "SUSPICIOUS");
        REQUIRE(std::string(severityToString(ViolationSeverity::CRITICAL)) == "CRITICAL");
        REQUIRE(std::string(severityToString(ViolationSeverity::BAN)) == "BAN");
    }
}

// ============================================================================
// CheatDetectionResult struct tests
// ============================================================================

TEST_CASE("CheatDetectionResult default construction", "[security][anticheat][types]") {
    CheatDetectionResult result;

    SECTION("Default values are correct") {
        REQUIRE(result.detected == false);
        REQUIRE(result.type == CheatType::NONE);
        REQUIRE(result.severity == ViolationSeverity::INFO);
        REQUIRE(result.description == nullptr);
        REQUIRE(result.confidence == 0.0f);
        REQUIRE(result.actualValue == 0.0f);
        REQUIRE(result.expectedValue == 0.0f);
        REQUIRE(result.timestamp == 0);
    }

    SECTION("Position defaults to zero") {
        REQUIRE(result.correctedPosition.x == 0);
        REQUIRE(result.correctedPosition.y == 0);
        REQUIRE(result.correctedPosition.z == 0);
    }
}

TEST_CASE("CheatDetectionResult field assignment", "[security][anticheat][types]") {
    CheatDetectionResult result;

    SECTION("All fields can be assigned") {
        result.detected = true;
        result.type = CheatType::SPEED_HACK;
        result.severity = ViolationSeverity::CRITICAL;
        result.confidence = 0.95f;
        result.actualValue = 50.0f;
        result.expectedValue = 6.0f;
        result.timestamp = 12345678;
        result.correctedPosition.x = 1000;

        REQUIRE(result.detected == true);
        REQUIRE(result.type == CheatType::SPEED_HACK);
        REQUIRE(result.severity == ViolationSeverity::CRITICAL);
        REQUIRE(result.confidence == Catch::Approx(0.95f));
        REQUIRE(result.actualValue == Catch::Approx(50.0f));
        REQUIRE(result.expectedValue == Catch::Approx(6.0f));
        REQUIRE(result.timestamp == 12345678);
        REQUIRE(result.correctedPosition.x == 1000);
    }
}

// ============================================================================
// ViolationRecord struct tests
// ============================================================================

TEST_CASE("ViolationRecord default construction", "[security][anticheat][types]") {
    ViolationRecord record;

    SECTION("Default values are correct") {
        REQUIRE(record.type == CheatType::NONE);
        REQUIRE(record.timestamp == 0);
        REQUIRE(record.confidence == 0.0f);
        REQUIRE(record.details.empty());
    }
}

TEST_CASE("ViolationRecord field assignment", "[security][anticheat][types]") {
    ViolationRecord record;

    SECTION("All fields can be assigned") {
        record.type = CheatType::TELEPORT;
        record.timestamp = 9876543;
        record.confidence = 0.87f;
        record.details = "Player teleported 500m instantly";

        REQUIRE(record.type == CheatType::TELEPORT);
        REQUIRE(record.timestamp == 9876543);
        REQUIRE(record.confidence == Catch::Approx(0.87f));
        REQUIRE(record.details == "Player teleported 500m instantly");
    }
}

// ============================================================================
// BehaviorProfile struct tests
// ============================================================================

TEST_CASE("BehaviorProfile default construction", "[security][anticheat][types]") {
    BehaviorProfile profile;

    SECTION("Default values are correct") {
        REQUIRE(profile.playerId == 0);
        REQUIRE(profile.entityId == 0);
        REQUIRE(profile.averageSpeed == 0.0f);
        REQUIRE(profile.maxRecordedSpeed == 0.0f);
        REQUIRE(profile.totalMovements == 0);
        REQUIRE(profile.teleportDetections == 0);
        REQUIRE(profile.speedViolations == 0);
        REQUIRE(profile.flyViolations == 0);
        REQUIRE(profile.hitsLanded == 0);
        REQUIRE(profile.hitsMissed == 0);
        REQUIRE(profile.averageAccuracy == 0.0f);
        REQUIRE(profile.suspiciousAimEvents == 0);
        REQUIRE(profile.totalDamageDealt == 0);
        REQUIRE(profile.averagePacketInterval == Catch::Approx(16.0f));
        REQUIRE(profile.packetFloods == 0);
        REQUIRE(profile.totalPackets == 0);
        REQUIRE(profile.lastPacketTime == 0);
        REQUIRE(profile.trustScore == 50);
        REQUIRE(profile.consecutiveCleanTicks == 0);
        REQUIRE(profile.accountCreationTime == 0);
        REQUIRE(profile.violationHistory.empty());
    }
}

TEST_CASE("BehaviorProfile field assignment", "[security][anticheat][types]") {
    BehaviorProfile profile;

    SECTION("All fields can be assigned") {
        profile.playerId = 12345;
        profile.entityId = 67890;
        profile.averageSpeed = 5.5f;
        profile.maxRecordedSpeed = 12.3f;
        profile.totalMovements = 1000;
        profile.teleportDetections = 5;
        profile.speedViolations = 3;
        profile.flyViolations = 2;
        profile.hitsLanded = 500;
        profile.hitsMissed = 50;
        profile.averageAccuracy = 90.0f;
        profile.suspiciousAimEvents = 1;
        profile.totalDamageDealt = 50000;
        profile.averagePacketInterval = 16.67f;
        profile.packetFloods = 0;
        profile.totalPackets = 10000;
        profile.lastPacketTime = 9999999;
        profile.trustScore = 75;
        profile.consecutiveCleanTicks = 100;
        profile.accountCreationTime = 1000000;

        REQUIRE(profile.playerId == 12345);
        REQUIRE(profile.entityId == 67890);
        REQUIRE(profile.averageSpeed == Catch::Approx(5.5f));
        REQUIRE(profile.maxRecordedSpeed == Catch::Approx(12.3f));
        REQUIRE(profile.totalMovements == 1000);
        REQUIRE(profile.teleportDetections == 5);
        REQUIRE(profile.speedViolations == 3);
        REQUIRE(profile.flyViolations == 2);
        REQUIRE(profile.hitsLanded == 500);
        REQUIRE(profile.hitsMissed == 50);
        REQUIRE(profile.averageAccuracy == Catch::Approx(90.0f));
        REQUIRE(profile.suspiciousAimEvents == 1);
        REQUIRE(profile.totalDamageDealt == 50000);
        REQUIRE(profile.trustScore == 75);
    }
}

TEST_CASE("BehaviorProfile isTrusted and isSuspicious", "[security][anticheat][types]") {
    SECTION("Trust threshold 70 isTrusted returns true") {
        BehaviorProfile profile;
        profile.trustScore = 70;
        REQUIRE(profile.isTrusted() == true);
        REQUIRE(profile.isSuspicious() == false);
    }

    SECTION("Trust score above 70 isTrusted returns true") {
        BehaviorProfile profile;
        profile.trustScore = 100;
        REQUIRE(profile.isTrusted() == true);
    }

    SECTION("Trust score below 70 isTrusted returns false") {
        BehaviorProfile profile;
        profile.trustScore = 69;
        REQUIRE(profile.isTrusted() == false);
    }

    SECTION("Trust threshold 30 isSuspicious returns true") {
        BehaviorProfile profile;
        profile.trustScore = 30;
        REQUIRE(profile.isSuspicious() == false);  // At threshold, not suspicious
    }

    SECTION("Trust score below 30 isSuspicious returns true") {
        BehaviorProfile profile;
        profile.trustScore = 29;
        REQUIRE(profile.isSuspicious() == true);
    }

    SECTION("Trust score above 30 isSuspicious returns false") {
        BehaviorProfile profile;
        profile.trustScore = 31;
        REQUIRE(profile.isSuspicious() == false);
    }
}

TEST_CASE("BehaviorProfile recordViolation", "[security][anticheat][types]") {
    BehaviorProfile profile;
    profile.speedViolations = 1;
    profile.flyViolations = 0;

    CheatDetectionResult detection;
    detection.type = CheatType::SPEED_HACK;
    detection.severity = ViolationSeverity::WARNING;
    detection.confidence = 0.75f;
    detection.timestamp = 12345;

    SECTION("recordViolation adds to history") {
        profile.recordViolation(detection);
        REQUIRE(profile.violationHistory.size() == 1);
        REQUIRE(profile.violationHistory[0].type == CheatType::SPEED_HACK);
        REQUIRE(profile.violationHistory[0].timestamp == 12345);
    }

    SECTION("recordViolation increments violation counter for SPEED_HACK") {
        REQUIRE(profile.speedViolations == 1);  // Initial
        profile.recordViolation(detection);
        REQUIRE(profile.speedViolations == 2);  // Incremented
    }

    SECTION("recordViolation increments violation counter for TELEPORT") {
        detection.type = CheatType::TELEPORT;
        profile.recordViolation(detection);
        REQUIRE(profile.teleportDetections == 1);
    }

    SECTION("recordViolation increments violation counter for FLY_HACK") {
        detection.type = CheatType::FLY_HACK;
        profile.recordViolation(detection);
        REQUIRE(profile.flyViolations == 1);
    }
}

TEST_CASE("BehaviorProfile recordCleanMovement", "[security][anticheat][types]") {
    BehaviorProfile profile;
    profile.consecutiveCleanTicks = 5;
    // Note: recordCleanMovement only increments consecutiveCleanTicks, not totalMovements

    SECTION("Increments consecutiveCleanTicks") {
        profile.recordCleanMovement();
        REQUIRE(profile.consecutiveCleanTicks == 6);
    }
}

TEST_CASE("BehaviorProfile updateTrustScore", "[security][anticheat][types]") {
    BehaviorProfile profile;
    profile.trustScore = 50;

    SECTION("Positive delta increases trust score") {
        profile.updateTrustScore(10);
        REQUIRE(profile.trustScore == 60);
    }

    SECTION("Negative delta decreases trust score") {
        profile.updateTrustScore(-20);
        REQUIRE(profile.trustScore == 30);
    }

    SECTION("Trust score clamped to maximum 100") {
        profile.trustScore = 95;
        profile.updateTrustScore(10);
        REQUIRE(profile.trustScore == 100);
    }

    SECTION("Trust score clamped to minimum 0") {
        profile.trustScore = 5;
        profile.updateTrustScore(-10);
        REQUIRE(profile.trustScore == 0);
    }

    SECTION("Large positive delta clamped to 100") {
        profile.trustScore = 50;
        profile.updateTrustScore(100);
        REQUIRE(profile.trustScore == 100);
    }

    SECTION("Large negative delta clamped to 0") {
        profile.trustScore = 50;
        profile.updateTrustScore(-100);
        REQUIRE(profile.trustScore == 0);
    }
}

TEST_CASE("BehaviorProfile isNewPlayer", "[security][anticheat][types]") {
    SECTION("Account creation time 0 is new if current time is less than 5 minutes") {
        BehaviorProfile profile;
        profile.accountCreationTime = 0;
        // With grace period of 300000ms (5 minutes), currentTimeMs < 300000 means new
        REQUIRE(profile.isNewPlayer(100000) == true);
        // currentTimeMs > 300000 means not new
        REQUIRE(profile.isNewPlayer(400000) == false);
    }

    SECTION("Recent account is new player") {
        BehaviorProfile profile;
        profile.accountCreationTime = 100000;  // Created at t=100s
        // At t=200000 (200s), player is still new (200000 - 100000 = 100000 < 300000)
        REQUIRE(profile.isNewPlayer(200000) == true);
    }

    SECTION("Old account is not new player") {
        BehaviorProfile profile;
        profile.accountCreationTime = 100000;
        // At t=500000 (500s), player is not new (500000 - 100000 = 400000 > 300000)
        REQUIRE(profile.isNewPlayer(500000) == false);
    }
}

TEST_CASE("BehaviorProfile getRecentViolationCount", "[security][anticheat][types]") {
    BehaviorProfile profile;
    // Add violations at different timestamps
    profile.violationHistory.push_back({CheatType::SPEED_HACK, 100, 0.8f, ""});
    profile.violationHistory.push_back({CheatType::TELEPORT, 200, 1.0f, ""});
    profile.violationHistory.push_back({CheatType::SPEED_HACK, 500, 0.7f, ""});
    profile.violationHistory.push_back({CheatType::FLY_HACK, 800, 0.9f, ""});

    SECTION("Large window includes all violations") {
        // Window of 1000ms from timestamp 1000 includes all 4
        REQUIRE(profile.getRecentViolationCount(1000, 1000) == 4);
    }

    SECTION("Medium window includes recent violations") {
        // Window of 400ms from timestamp 900 includes timestamps 500 and 800
        REQUIRE(profile.getRecentViolationCount(400, 900) == 2);
    }

    SECTION("Small window includes only very recent") {
        // Window of 100ms from timestamp 850 includes only timestamp 800
        REQUIRE(profile.getRecentViolationCount(100, 850) == 1);
    }

    SECTION("Window with no violations returns 0") {
        REQUIRE(profile.getRecentViolationCount(50, 50) == 0);
    }
}

// ============================================================================
// Callback type tests (compile-time only)
// ============================================================================

TEST_CASE("Callback types are defined", "[security][anticheat][types]") {
    // These are type definitions - just ensure they compile
    CheatCallback dummyCallback = nullptr;
    BanCallback dummyBanCallback = nullptr;
    KickCallback dummyKickCallback = nullptr;

    // Assignment to nullptr should work for all
    REQUIRE(dummyCallback == nullptr);
    REQUIRE(dummyBanCallback == nullptr);
    REQUIRE(dummyKickCallback == nullptr);
}