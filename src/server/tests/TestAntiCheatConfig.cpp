// AntiCheatConfig expanded tests
// Edge cases, boundary validation, config mutation, and singleton behavior

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "security/AntiCheatConfig.hpp"
#include <cmath>
#include <cstring>

using namespace DarkAges;
using namespace DarkAges::Security;
using Catch::Approx;

// ============================================================================
// Default values (existing tests)
// ============================================================================

TEST_CASE("AntiCheatConfig default values", "[security][config]") {
    AntiCheatConfig config;

    SECTION("Speed check parameters") {
        REQUIRE(config.speedTolerance == Approx(1.2f).margin(0.001f));
        REQUIRE(config.speedViolationThreshold == 3);
        REQUIRE(config.speedViolationWindowMs == 5000);
        REQUIRE(config.maxAirTimeMs == 500);
    }

    SECTION("Teleport detection parameters") {
        REQUIRE(config.maxTeleportDistance == Approx(100.0f).margin(0.001f));
        REQUIRE(config.instantBanOnTeleport == true);
        REQUIRE(config.teleportGracePeriodMs == 2000);
    }

    SECTION("Rate limiting parameters") {
        REQUIRE(config.maxInputsPerSecond == 60);
        REQUIRE(config.inputWindowMs == 1000);
        REQUIRE(config.inputBurstAllowance == 5);
    }

    SECTION("Fly/no-clip detection parameters") {
        REQUIRE(config.groundCheckDistance == Approx(1.0f).margin(0.001f));
        REQUIRE(config.maxVerticalSpeedNoJump == Approx(0.5f).margin(0.001f));
        REQUIRE(config.flyViolationThreshold == 3);
    }

    SECTION("Input validation parameters") {
        REQUIRE(config.maxYaw == Approx(6.283185f).margin(0.001f));
        REQUIRE(config.maxPitch == Approx(1.570796f).margin(0.001f));
        REQUIRE(config.minAttackIntervalMs == 500);
    }

    SECTION("Trust system parameters") {
        REQUIRE(config.initialTrustScore == 50);
        REQUIRE(config.minTrustForLenientChecks == 70);
        REQUIRE(config.suspiciousTrustThreshold == 30);
        REQUIRE(config.trustRecoveryPerMinute == 5);
    }

    SECTION("Position validation parameters") {
        REQUIRE(config.positionTolerance == Approx(0.5f).margin(0.001f));
        REQUIRE(config.maxLagCompensationDistance == Approx(10.0f).margin(0.001f));
        REQUIRE(config.positionMismatchThreshold == 3);
    }

    SECTION("Combat validation parameters") {
        REQUIRE(config.maxDamagePerHit == 5000);
        REQUIRE(config.maxMeleeRange == Approx(3.0f).margin(0.001f));
        REQUIRE(config.maxRangedRange == Approx(50.0f).margin(0.001f));
        REQUIRE(config.maxAimDeviation == Approx(30.0f).margin(0.001f));
        REQUIRE(config.damageCooldownMs == 100);
    }

    SECTION("Statistical analysis parameters") {
        REQUIRE(config.behaviorAnalysisWindowSec == 60);
        REQUIRE(config.aimbotAccuracyThreshold == Approx(95.0f).margin(0.001f));
        REQUIRE(config.minSamplesForAnalysis == 10);
    }

    SECTION("Response severity thresholds") {
        REQUIRE(config.infoThreshold == 1);
        REQUIRE(config.warningThreshold == 2);
        REQUIRE(config.suspiciousThreshold == 3);
        REQUIRE(config.criticalThreshold == 5);
        REQUIRE(config.banThreshold == 10);
    }

    SECTION("Ban/kick configuration") {
        REQUIRE(config.defaultBanDurationMinutes == 60);
        REQUIRE(config.banDurationMultiplier == Approx(2.0f).margin(0.001f));
        REQUIRE(config.maxBanDurationMinutes == 10080);
        REQUIRE(std::string(config.kickMessage) == "Kicked for violation of server rules");
        REQUIRE(std::string(config.banMessage) == "Banned for suspected cheating");
    }
}

// ============================================================================
// Singleton behavior
// ============================================================================

TEST_CASE("AntiCheatConfig getAntiCheatConfig singleton", "[security][config]") {
    AntiCheatConfig& config = getAntiCheatConfig();

    SECTION("Singleton returns valid reference") {
        REQUIRE(&config != nullptr);
    }

    SECTION("Singleton has correct default values") {
        REQUIRE(config.speedTolerance == Approx(1.2f).margin(0.001f));
        REQUIRE(config.maxInputsPerSecond == 60);
        REQUIRE(config.defaultBanDurationMinutes == 60);
    }

    SECTION("Singleton is modifiable") {
        float original = config.speedTolerance;
        config.speedTolerance = 1.5f;
        REQUIRE(config.speedTolerance == Approx(1.5f).margin(0.001f));
        config.speedTolerance = original;  // Restore original
    }

    SECTION("Multiple calls return same instance") {
        AntiCheatConfig& config2 = getAntiCheatConfig();
        REQUIRE(&config == &config2);
    }
}

// ============================================================================
// Math validations
// ============================================================================

TEST_CASE("AntiCheatConfig math validations", "[security][config]") {
    AntiCheatConfig config;

    SECTION("maxYaw equals 2*PI") {
        float twoPi = 2.0f * 3.14159265358979323846f;
        REQUIRE(config.maxYaw == Approx(twoPi).margin(0.001f));
    }

    SECTION("maxPitch equals PI/2") {
        float halfPi = 3.14159265358979323846f / 2.0f;
        REQUIRE(config.maxPitch == Approx(halfPi).margin(0.001f));
    }

    SECTION("maxBanDurationMinutes equals 1 week") {
        // 10080 minutes = 7 days = 1 week
        REQUIRE(config.maxBanDurationMinutes == 7 * 24 * 60);
    }

    SECTION("speedTolerance allows 20% over max") {
        REQUIRE(config.speedTolerance == Approx(1.2f).margin(0.001f));
    }

    SECTION("maxMeleeRange is reasonable for melee") {
        REQUIRE(config.maxMeleeRange > 0.0f);
        REQUIRE(config.maxMeleeRange < 10.0f);
    }

    SECTION("maxRangedRange is greater than maxMeleeRange") {
        REQUIRE(config.maxRangedRange > config.maxMeleeRange);
    }
}

// ============================================================================
// Config mutation tests
// ============================================================================

TEST_CASE("AntiCheatConfig mutation", "[security][config]") {
    AntiCheatConfig config;

    SECTION("Speed tolerance can be modified") {
        config.speedTolerance = 1.5f;
        REQUIRE(config.speedTolerance == Approx(1.5f).margin(0.001f));

        config.speedTolerance = 2.0f;
        REQUIRE(config.speedTolerance == Approx(2.0f).margin(0.001f));
    }

    SECTION("Violation thresholds can be modified") {
        config.speedViolationThreshold = 5;
        REQUIRE(config.speedViolationThreshold == 5);

        config.flyViolationThreshold = 10;
        REQUIRE(config.flyViolationThreshold == 10);
    }

    SECTION("Boolean flags can be toggled") {
        config.instantBanOnTeleport = false;
        REQUIRE(config.instantBanOnTeleport == false);

        config.instantBanOnTeleport = true;
        REQUIRE(config.instantBanOnTeleport == true);
    }

    SECTION("Trust scores can be modified") {
        config.initialTrustScore = 100;
        REQUIRE(config.initialTrustScore == 100);

        config.suspiciousTrustThreshold = 50;
        REQUIRE(config.suspiciousTrustThreshold == 50);
    }

    SECTION("Ban durations can be modified") {
        config.defaultBanDurationMinutes = 120;
        REQUIRE(config.defaultBanDurationMinutes == 120);

        config.maxBanDurationMinutes = 0;  // Permanent
        REQUIRE(config.maxBanDurationMinutes == 0);
    }

    SECTION("String pointers can be changed") {
        const char* newKickMsg = "Custom kick message";
        config.kickMessage = newKickMsg;
        REQUIRE(std::string(config.kickMessage) == "Custom kick message");
    }
}

// ============================================================================
// Boundary validation tests
// ============================================================================

TEST_CASE("AntiCheatConfig boundary values", "[security][config]") {
    AntiCheatConfig config;

    SECTION("Zero speed tolerance is valid") {
        config.speedTolerance = 0.0f;
        REQUIRE(config.speedTolerance == Approx(0.0f).margin(0.001f));
    }

    SECTION("Very high speed tolerance is valid") {
        config.speedTolerance = 100.0f;
        REQUIRE(config.speedTolerance == Approx(100.0f).margin(0.001f));
    }

    SECTION("Trust score at boundaries") {
        config.initialTrustScore = 0;
        REQUIRE(config.initialTrustScore == 0);

        config.initialTrustScore = 100;
        REQUIRE(config.initialTrustScore == 100);
    }

    SECTION("Zero violation threshold means instant action") {
        config.speedViolationThreshold = 0;
        REQUIRE(config.speedViolationThreshold == 0);
    }

    SECTION("Zero ban duration means permanent ban") {
        config.maxBanDurationMinutes = 0;
        REQUIRE(config.maxBanDurationMinutes == 0);
    }

    SECTION("Negative damage per hit") {
        config.maxDamagePerHit = -100;
        REQUIRE(config.maxDamagePerHit == -100);
    }

    SECTION("Very small ranges") {
        config.maxMeleeRange = 0.1f;
        REQUIRE(config.maxMeleeRange == Approx(0.1f).margin(0.001f));

        config.maxRangedRange = 0.5f;
        REQUIRE(config.maxRangedRange == Approx(0.5f).margin(0.001f));
    }

    SECTION("Very large ranges") {
        config.maxMeleeRange = 1000.0f;
        REQUIRE(config.maxMeleeRange == Approx(1000.0f).margin(0.001f));
    }
}

// ============================================================================
// Relationship validation tests
// ============================================================================

TEST_CASE("AntiCheatConfig relationship validation", "[security][config]") {
    AntiCheatConfig config;

    SECTION("Warning threshold is less than suspicious threshold") {
        REQUIRE(config.warningThreshold < config.suspiciousThreshold);
    }

    SECTION("Suspicious threshold is less than critical threshold") {
        REQUIRE(config.suspiciousThreshold < config.criticalThreshold);
    }

    SECTION("Critical threshold is less than ban threshold") {
        REQUIRE(config.criticalThreshold < config.banThreshold);
    }

    SECTION("Min trust for lenient checks is greater than suspicious threshold") {
        REQUIRE(config.minTrustForLenientChecks > config.suspiciousTrustThreshold);
    }

    SECTION("Input window is reasonable for rate limiting") {
        REQUIRE(config.inputWindowMs > 0);
        REQUIRE(config.inputWindowMs <= 10000);
    }

    SECTION("Ban duration multiplier is at least 1.0") {
        REQUIRE(config.banDurationMultiplier >= 1.0f);
    }

    SECTION("Max damage is positive for normal gameplay") {
        REQUIRE(config.maxDamagePerHit > 0);
    }
}

// ============================================================================
// Config copy tests
// ============================================================================

TEST_CASE("AntiCheatConfig copy behavior", "[security][config]") {
    SECTION("Config can be copied") {
        AntiCheatConfig original;
        original.speedTolerance = 2.5f;
        original.maxInputsPerSecond = 120;
        original.instantBanOnTeleport = false;

        AntiCheatConfig copy = original;

        REQUIRE(copy.speedTolerance == Approx(2.5f).margin(0.001f));
        REQUIRE(copy.maxInputsPerSecond == 120);
        REQUIRE(copy.instantBanOnTeleport == false);
    }

    SECTION("Copy is independent of original") {
        AntiCheatConfig original;
        AntiCheatConfig copy = original;

        copy.speedTolerance = 999.0f;
        REQUIRE(original.speedTolerance == Approx(1.2f).margin(0.001f));
        REQUIRE(copy.speedTolerance == Approx(999.0f).margin(0.001f));
    }
}
