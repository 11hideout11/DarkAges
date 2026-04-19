#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "security/AntiCheatConfig.hpp"

using namespace DarkAges;
using namespace DarkAges::Security;

TEST_CASE("AntiCheatConfig default values", "[security][config]") {
    AntiCheatConfig config;

    SECTION("Speed check parameters") {
        REQUIRE(config.speedTolerance == Catch::Approx(1.2f).margin(0.001f));
        REQUIRE(config.speedViolationThreshold == 3);
        REQUIRE(config.speedViolationWindowMs == 5000);
        REQUIRE(config.maxAirTimeMs == 500);
    }

    SECTION("Teleport detection parameters") {
        REQUIRE(config.maxTeleportDistance == Catch::Approx(100.0f).margin(0.001f));
        REQUIRE(config.instantBanOnTeleport == true);
        REQUIRE(config.teleportGracePeriodMs == 2000);
    }

    SECTION("Rate limiting parameters") {
        REQUIRE(config.maxInputsPerSecond == 60);
        REQUIRE(config.inputWindowMs == 1000);
        REQUIRE(config.inputBurstAllowance == 5);
    }

    SECTION("Fly/no-clip detection parameters") {
        REQUIRE(config.groundCheckDistance == Catch::Approx(1.0f).margin(0.001f));
        REQUIRE(config.maxVerticalSpeedNoJump == Catch::Approx(0.5f).margin(0.001f));
        REQUIRE(config.flyViolationThreshold == 3);
    }

    SECTION("Input validation parameters") {
        REQUIRE(config.maxYaw == Catch::Approx(6.283185f).margin(0.001f));
        REQUIRE(config.maxPitch == Catch::Approx(1.570796f).margin(0.001f));
        REQUIRE(config.minAttackIntervalMs == 500);
    }

    SECTION("Trust system parameters") {
        REQUIRE(config.initialTrustScore == 50);
        REQUIRE(config.minTrustForLenientChecks == 70);
        REQUIRE(config.suspiciousTrustThreshold == 30);
        REQUIRE(config.trustRecoveryPerMinute == 5);
    }

    SECTION("Position validation parameters") {
        REQUIRE(config.positionTolerance == Catch::Approx(0.5f).margin(0.001f));
        REQUIRE(config.maxLagCompensationDistance == Catch::Approx(10.0f).margin(0.001f));
        REQUIRE(config.positionMismatchThreshold == 3);
    }

    SECTION("Combat validation parameters") {
        REQUIRE(config.maxDamagePerHit == 5000);
        REQUIRE(config.maxMeleeRange == Catch::Approx(3.0f).margin(0.001f));
        REQUIRE(config.maxRangedRange == Catch::Approx(50.0f).margin(0.001f));
        REQUIRE(config.maxAimDeviation == Catch::Approx(30.0f).margin(0.001f));
        REQUIRE(config.damageCooldownMs == 100);
    }

    SECTION("Statistical analysis parameters") {
        REQUIRE(config.behaviorAnalysisWindowSec == 60);
        REQUIRE(config.aimbotAccuracyThreshold == Catch::Approx(95.0f).margin(0.001f));
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
        REQUIRE(config.banDurationMultiplier == Catch::Approx(2.0f).margin(0.001f));
        REQUIRE(config.maxBanDurationMinutes == 10080);
        REQUIRE(std::string(config.kickMessage) == "Kicked for violation of server rules");
        REQUIRE(std::string(config.banMessage) == "Banned for suspected cheating");
    }
}

TEST_CASE("AntiCheatConfig getAntiCheatConfig singleton", "[security][config]") {
    AntiCheatConfig& config = getAntiCheatConfig();

    // Verify the singleton returns a valid reference
    SECTION("Singleton returns valid reference") {
        REQUIRE(&config != nullptr);
    }

    SECTION("Singleton has correct default values") {
        REQUIRE(config.speedTolerance == Catch::Approx(1.2f).margin(0.001f));
        REQUIRE(config.maxInputsPerSecond == 60);
        REQUIRE(config.defaultBanDurationMinutes == 60);
    }

    SECTION("Singleton is modifiable") {
        float original = config.speedTolerance;
        config.speedTolerance = 1.5f;
        REQUIRE(config.speedTolerance == Catch::Approx(1.5f).margin(0.001f));
        config.speedTolerance = original;  // Restore original
    }
}

TEST_CASE("AntiCheatConfig math validations", "[security][config]") {
    AntiCheatConfig config;

    SECTION("maxYaw equals 2*PI") {
        float twoPi = 2.0f * 3.14159265358979323846f;
        REQUIRE(config.maxYaw == Catch::Approx(twoPi).margin(0.001f));
    }

    SECTION("maxPitch equals PI/2") {
        float halfPi = 3.14159265358979323846f / 2.0f;
        REQUIRE(config.maxPitch == Catch::Approx(halfPi).margin(0.001f));
    }

    SECTION("maxBanDurationMinutes equals 1 week") {
        // 10080 minutes = 7 days = 1 week
        REQUIRE(config.maxBanDurationMinutes == 7 * 24 * 60);
    }
}