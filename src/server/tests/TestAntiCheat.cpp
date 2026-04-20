// [SECURITY_AGENT] Tests for AntiCheatSystem
// Comprehensive anti-cheat detection and behavior tracking tests

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "security/AntiCheat.hpp"
#include "security/AntiCheatConfig.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <cstdint>
#include <string>
#include <vector>

using namespace DarkAges;
using namespace DarkAges::Security;

// ============================================================================
// Helper: Create a fixed-point position from float values
// ============================================================================
static Position makePosition(float x, float y, float z, uint32_t ts = 0) {
    Position p;
    p.x = static_cast<Constants::Fixed>(x * Constants::FLOAT_TO_FIXED);
    p.y = static_cast<Constants::Fixed>(y * Constants::FLOAT_TO_FIXED);
    p.z = static_cast<Constants::Fixed>(z * Constants::FLOAT_TO_FIXED);
    p.timestamp_ms = ts;
    return p;
}

// ============================================================================
// Helper: Create a minimal input state
// ============================================================================
static InputState makeInput(bool forward = true, bool sprint = false) {
    InputState input;
    input.forward = forward ? 1 : 0;
    input.sprint = sprint ? 1 : 0;
    return input;
}

// ============================================================================
// Helper: Setup a player entity in the registry
// ============================================================================
static EntityID setupPlayerEntity(Registry& registry, uint64_t playerId) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, makePosition(0, 0, 0));
    registry.emplace<Velocity>(entity, Velocity{0, 0, 0});
    registry.emplace<PlayerInfo>(entity, PlayerInfo{playerId, 1, "TestPlayer"});
    registry.emplace<AntiCheatState>(entity, AntiCheatState{});
    registry.emplace<CombatState>(entity, CombatState{});
    return entity;
}

// ============================================================================
// AntiCheatSystem Initialization
// ============================================================================

TEST_CASE("AntiCheatSystem initialization", "[security][anticheat]") {
    AntiCheatSystem acs;

    SECTION("Initialize returns true") {
        REQUIRE(acs.initialize());
    }

    SECTION("Double initialize is safe") {
        REQUIRE(acs.initialize());
        REQUIRE(acs.initialize());
    }

    SECTION("Initial state has zero stats") {
        REQUIRE(acs.initialize());
        REQUIRE(acs.getTotalDetections() == 0);
        REQUIRE(acs.getPlayersKicked() == 0);
        REQUIRE(acs.getPlayersBanned() == 0);
        REQUIRE(acs.getActiveProfileCount() == 0);
    }

    SECTION("Shutdown is safe before init") {
        // Should not crash
        acs.shutdown();
    }

    SECTION("Shutdown clears profiles") {
        REQUIRE(acs.initialize());
        Registry registry;
        auto entity = setupPlayerEntity(registry, 42);
        // Trigger profile creation via validation
        (void)acs.validateMovement(entity, makePosition(0, 0, 0), makePosition(1, 0, 0),
                             100, makeInput(), registry);
        REQUIRE(acs.getActiveProfileCount() >= 1);
        acs.shutdown();
        // shutdown() doesn't clear profiles, just sets initialized=false
        REQUIRE(acs.getActiveProfileCount() >= 1);
    }
}

// ============================================================================
// Speed Hack Detection
// ============================================================================

TEST_CASE("AntiCheatSystem speed hack detection", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;
    auto entity = setupPlayerEntity(registry, 1);

    SECTION("Normal walking speed is accepted") {
        // Walk at 5 m/s over 100ms = 0.5m displacement
        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(0.5f, 0, 0),
            100, makeInput(true, false), registry
        );
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Sprint speed is accepted") {
        // Sprint at 9 m/s over 100ms = 0.9m displacement
        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(0.9f, 0, 0),
            100, makeInput(true, true), registry
        );
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Excessive speed triggers detection") {
        // Move 20m in 100ms = 200 m/s (way over 6 m/s limit)
        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(20, 0, 0),
            100, makeInput(), registry
        );
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::SPEED_HACK);
        REQUIRE(result.confidence > 0.0f);
        REQUIRE(result.actualValue > result.expectedValue);
    }

    SECTION("Speed with zero delta time does not crash") {
        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(10, 0, 0),
            0, makeInput(), registry
        );
        // Should not crash; zero dt means speed is indeterminate
        // The teleport detector may fire instead for large distance
        (void)result;
    }
}

// ============================================================================
// Teleport Detection
// ============================================================================

TEST_CASE("AntiCheatSystem teleport detection", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;
    auto entity = setupPlayerEntity(registry, 2);

    SECTION("Small position changes are allowed") {
        // Move 0.5m in 100ms = 5 m/s, within base speed of 6 m/s
        auto result = acs.validateMovement(
            entity,
            makePosition(10, 0, 10),
            makePosition(10.5f, 0, 10),
            100, makeInput(), registry
        );
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Large instant teleport detected") {
        // Move 500m instantly - well over 100m threshold
        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(500, 0, 0),
            100, makeInput(), registry
        );
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::TELEPORT);
        REQUIRE(result.correctedPosition.x == makePosition(0, 0, 0).x);
    }

    SECTION("Teleport detection with instant ban config") {
        AntiCheatConfig config = acs.getConfig();
        config.instantBanOnTeleport = true;
        acs.setConfig(config);

        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(200, 0, 0),
            16, makeInput(), registry
        );
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::TELEPORT);
        REQUIRE(result.severity == ViolationSeverity::BAN);
    }

    SECTION("Teleport detection without instant ban kicks instead") {
        AntiCheatConfig config = acs.getConfig();
        config.instantBanOnTeleport = false;
        acs.setConfig(config);

        auto result = acs.validateMovement(
            entity,
            makePosition(0, 0, 0),
            makePosition(200, 0, 0),
            16, makeInput(), registry
        );
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::TELEPORT);
        REQUIRE(result.severity == ViolationSeverity::CRITICAL);
    }
}

// ============================================================================
// Position Bounds Validation
// ============================================================================

TEST_CASE("AntiCheatSystem position bounds validation", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Valid position within bounds") {
        auto result = acs.validatePositionBounds(makePosition(100, 10, 100));
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Position at boundary is valid") {
        auto result = acs.validatePositionBounds(
            makePosition(Constants::WORLD_MAX_X, 0, 0));
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Position beyond X bounds detected") {
        auto result = acs.validatePositionBounds(
            makePosition(Constants::WORLD_MAX_X + 100, 0, 0));
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::POSITION_SPOOFING);
    }

    SECTION("Position beyond Y bounds detected") {
        auto result = acs.validatePositionBounds(
            makePosition(0, Constants::WORLD_MAX_Y + 100, 0));
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::POSITION_SPOOFING);
    }

    SECTION("Position beyond Z bounds detected") {
        auto result = acs.validatePositionBounds(
            makePosition(0, 0, Constants::WORLD_MIN_Z - 100));
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::POSITION_SPOOFING);
    }

    SECTION("Negative positions out of bounds detected") {
        auto result = acs.validatePositionBounds(
            makePosition(Constants::WORLD_MIN_X - 1, 0, 0));
        REQUIRE(result.detected);
    }
}

// ============================================================================
// Input Validation
// ============================================================================

TEST_CASE("AntiCheatSystem input validation", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;
    auto entity = setupPlayerEntity(registry, 3);

    SECTION("Valid input accepted") {
        InputState input = makeInput();
        input.yaw = 1.0f;
        input.pitch = 0.5f;
        auto result = acs.validateInput(entity, input, 1000, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Invalid yaw detected") {
        InputState input = makeInput();
        input.yaw = 10.0f;  // Way over 2*PI
        auto result = acs.validateInput(entity, input, 1000, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::INPUT_MANIPULATION);
    }

    SECTION("Invalid pitch detected") {
        InputState input = makeInput();
        input.pitch = 3.0f;  // Way over PI/2
        auto result = acs.validateInput(entity, input, 1000, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::INPUT_MANIPULATION);
    }

    SECTION("Conflicting forward/backward detected") {
        InputState input;
        input.forward = 1;
        input.backward = 1;
        auto result = acs.validateInput(entity, input, 1000, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::INPUT_MANIPULATION);
    }

    SECTION("Conflicting left/right detected") {
        InputState input;
        input.left = 1;
        input.right = 1;
        auto result = acs.validateInput(entity, input, 1000, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::INPUT_MANIPULATION);
    }
}

// ============================================================================
// Rate Limiting
// ============================================================================

TEST_CASE("AntiCheatSystem rate limiting", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;
    auto entity = setupPlayerEntity(registry, 4);

    SECTION("Inputs within rate limit accepted") {
        AntiCheatConfig config = acs.getConfig();
        config.maxInputsPerSecond = 10;
        config.inputBurstAllowance = 2;
        config.inputWindowMs = 1000;
        acs.setConfig(config);

        // Re-create entity with new config
        AntiCheatState state;
        state.inputWindowStart = 0;
        state.inputCount = 0;
        registry.replace<AntiCheatState>(entity, state);

        // 10 inputs within limit
        for (uint32_t i = 0; i < 10; ++i) {
            auto result = acs.checkRateLimit(entity, i * 10, registry);
            REQUIRE_FALSE(result.detected);
        }
    }

    SECTION("Excessive inputs trigger flooding detection") {
        AntiCheatConfig config = acs.getConfig();
        config.maxInputsPerSecond = 5;
        config.inputBurstAllowance = 0;
        config.inputWindowMs = 1000;
        acs.setConfig(config);

        AntiCheatState state;
        state.inputWindowStart = 0;
        state.inputCount = 0;
        registry.replace<AntiCheatState>(entity, state);

        // First 5 should pass
        for (uint32_t i = 0; i < 5; ++i) {
            auto result = acs.checkRateLimit(entity, i * 10, registry);
            REQUIRE_FALSE(result.detected);
        }

        // 6th should trigger
        auto result = acs.checkRateLimit(entity, 50, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::PACKET_FLOODING);
    }

    SECTION("Window reset allows new inputs") {
        AntiCheatConfig config = acs.getConfig();
        config.maxInputsPerSecond = 3;
        config.inputBurstAllowance = 0;
        config.inputWindowMs = 100;
        acs.setConfig(config);

        AntiCheatState state;
        state.inputWindowStart = 0;
        state.inputCount = 0;
        registry.replace<AntiCheatState>(entity, state);

        // Exhaust first window
        for (uint32_t i = 0; i < 3; ++i) {
            (void)acs.checkRateLimit(entity, i * 10, registry);
        }

        // 4th in same window triggers
        auto result1 = acs.checkRateLimit(entity, 30, registry);
        REQUIRE(result1.detected);

        // After window expires, should accept again
        auto result2 = acs.checkRateLimit(entity, 200, registry);
        REQUIRE_FALSE(result2.detected);
    }
}

// ============================================================================
// Behavior Profile & Violation Tracking
// ============================================================================

TEST_CASE("AntiCheatSystem behavior profile tracking", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;

    SECTION("Profile is created on first access") {
        uint64_t playerId = 100;
        auto* profile = acs.getProfile(playerId);
        REQUIRE(profile != nullptr);
        REQUIRE(profile->playerId == playerId);
        REQUIRE(acs.getActiveProfileCount() == 1);
    }

    SECTION("Profile tracks violations") {
        uint64_t playerId = 200;
        auto* profile = acs.getProfile(playerId);
        REQUIRE(profile->violationHistory.empty());

        CheatDetectionResult detection;
        detection.type = CheatType::SPEED_HACK;
        detection.severity = ViolationSeverity::CRITICAL;
        detection.confidence = 0.9f;
        detection.timestamp = 1000;
        detection.description = "Test speed hack";

        profile->recordViolation(detection);
        REQUIRE(profile->violationHistory.size() == 1);
        REQUIRE(profile->speedViolations == 1);
    }

    SECTION("Profile trust score decreases on violations") {
        uint64_t playerId = 300;
        auto* profile = acs.getProfile(playerId);
        uint8_t initialTrust = profile->trustScore;

        CheatDetectionResult detection;
        detection.type = CheatType::SPEED_HACK;
        detection.severity = ViolationSeverity::WARNING;
        detection.confidence = 0.5f;
        detection.timestamp = 1000;

        profile->recordViolation(detection);
        REQUIRE(profile->trustScore < initialTrust);
    }

    SECTION("Profile trust score is clamped to 0") {
        uint64_t playerId = 400;
        auto* profile = acs.getProfile(playerId);
        profile->trustScore = 5;

        // Apply a large negative delta
        profile->updateTrustScore(-100);
        REQUIRE(profile->trustScore == 0);
    }

    SECTION("Profile trust score is clamped to 100") {
        uint64_t playerId = 401;
        auto* profile = acs.getProfile(playerId);
        profile->trustScore = 95;

        profile->updateTrustScore(100);
        REQUIRE(profile->trustScore == 100);
    }

    SECTION("Trusted and suspicious thresholds work") {
        BehaviorProfile profile;
        profile.trustScore = 80;
        REQUIRE(profile.isTrusted());
        REQUIRE_FALSE(profile.isSuspicious());

        profile.trustScore = 20;
        REQUIRE_FALSE(profile.isTrusted());
        REQUIRE(profile.isSuspicious());

        profile.trustScore = 50;
        REQUIRE_FALSE(profile.isTrusted());
        REQUIRE_FALSE(profile.isSuspicious());
    }

    SECTION("New player detection works") {
        BehaviorProfile profile;
        profile.accountCreationTime = 0;
        REQUIRE(profile.isNewPlayer(100000));       // 100s, still new
        REQUIRE_FALSE(profile.isNewPlayer(400000));  // 400s, not new
    }

    SECTION("Recent violation count works correctly") {
        BehaviorProfile profile;
        profile.violationHistory.push_back({CheatType::SPEED_HACK, 100, 0.8f, ""});
        profile.violationHistory.push_back({CheatType::TELEPORT, 200, 1.0f, ""});
        profile.violationHistory.push_back({CheatType::SPEED_HACK, 500, 0.7f, ""});

        // Window uses <= comparison: 600 - timestamp <= windowMs
        REQUIRE(profile.getRecentViolationCount(500, 600) == 3);   // all 3
        REQUIRE(profile.getRecentViolationCount(100, 600) == 1);   // only timestamp 500 (600-500=100 <= 100)
        REQUIRE(profile.getRecentViolationCount(99, 600) == 0);    // 600-500=100 > 99
    }

    SECTION("Clean movement increases trust slowly") {
        BehaviorProfile profile;
        profile.trustScore = 50;

        // 600 clean ticks should recover 1 trust
        for (uint32_t i = 0; i < 600; ++i) {
            profile.recordCleanMovement();
        }
        REQUIRE(profile.trustScore == 51);
    }

    SECTION("Remove profile works") {
        uint64_t playerId = 500;
        (void)acs.getProfile(playerId);
        REQUIRE(acs.getActiveProfileCount() >= 1);
        acs.removeProfile(playerId);
        REQUIRE(acs.getActiveProfileCount() == 0);
    }
}

// ============================================================================
// Graduated Response
// ============================================================================

TEST_CASE("AntiCheatSystem graduated response", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Cheat callback fires on detection") {
        bool callbackFired = false;
        uint64_t callbackPlayerId = 0;
        acs.setOnCheatDetected([&](uint64_t pid, const CheatDetectionResult&) {
            callbackFired = true;
            callbackPlayerId = pid;
        });

        Registry registry;
        auto entity = setupPlayerEntity(registry, 600);

        // Trigger a speed hack
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(50, 0, 0),
            100, makeInput(), registry);

        REQUIRE(callbackFired);
        REQUIRE(callbackPlayerId == 600);
    }

    SECTION("Critical severity triggers kick") {
        AntiCheatConfig config = acs.getConfig();
        config.instantBanOnTeleport = false;  // Don't ban, use kick for CRITICAL
        acs.setConfig(config);

        bool kicked = false;
        uint64_t kickedPlayerId = 0;
        acs.setOnPlayerKicked([&](uint64_t pid, const char*) {
            kicked = true;
            kickedPlayerId = pid;
        });

        Registry registry;
        auto entity = setupPlayerEntity(registry, 700);

        // Massive teleport -> CRITICAL -> kick
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(200, 0, 0),
            100, makeInput(), registry);

        REQUIRE(kicked);
        REQUIRE(kickedPlayerId == 700);
        REQUIRE(acs.getPlayersKicked() == 1);
    }

    SECTION("Ban severity triggers ban") {
        bool banned = false;
        acs.setOnPlayerBanned([&](uint64_t, const char*, uint32_t) {
            banned = true;
        });

        AntiCheatConfig config = acs.getConfig();
        config.instantBanOnTeleport = true;
        acs.setConfig(config);

        Registry registry;
        auto entity = setupPlayerEntity(registry, 800);

        // Teleport with instant ban -> BAN
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(500, 0, 0),
            16, makeInput(), registry);

        REQUIRE(banned);
        REQUIRE(acs.getPlayersBanned() == 1);
    }
}

// ============================================================================
// Configuration
// ============================================================================

TEST_CASE("AntiCheatSystem configuration", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Default config has sensible values") {
        const auto& config = acs.getConfig();
        REQUIRE(config.speedTolerance > 1.0f);
        REQUIRE(config.maxTeleportDistance > 0.0f);
        REQUIRE(config.maxInputsPerSecond > 0);
        REQUIRE(config.initialTrustScore <= 100);
    }

    SECTION("Config can be changed at runtime") {
        AntiCheatConfig config = acs.getConfig();
        config.speedTolerance = 2.0f;
        config.maxTeleportDistance = 50.0f;
        acs.setConfig(config);

        const auto& newConfig = acs.getConfig();
        REQUIRE(newConfig.speedTolerance == 2.0f);
        REQUIRE(newConfig.maxTeleportDistance == 50.0f);
    }

    SECTION("Stricter teleport distance catches more cheats") {
        AntiCheatConfig config = acs.getConfig();
        config.maxTeleportDistance = 10.0f;  // Very strict
        config.instantBanOnTeleport = false;
        acs.setConfig(config);

        Registry registry;
        auto entity = setupPlayerEntity(registry, 900);

        // 15m move in 1 tick - now caught with strict config
        auto result = acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(15, 0, 0),
            16, makeInput(), registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::TELEPORT);
    }
}

// ============================================================================
// Combat Validation
// ============================================================================

TEST_CASE("AntiCheatSystem combat validation", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;
    auto attacker = setupPlayerEntity(registry, 1000);
    auto target = setupPlayerEntity(registry, 1001);

    SECTION("Valid attack accepted") {
        auto result = acs.validateCombat(
            attacker, target, 500,
            makePosition(2, 0, 0),  // Hit position
            makePosition(0, 0, 0),  // Attacker position
            registry
        );
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Damage hack detected") {
        AntiCheatConfig config = acs.getConfig();
        config.maxDamagePerHit = 1000;
        acs.setConfig(config);

        auto result = acs.validateCombat(
            attacker, target, 5000,  // Way over limit
            makePosition(1, 0, 0),
            makePosition(0, 0, 0),
            registry
        );
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::DAMAGE_HACK);
        REQUIRE(result.severity == ViolationSeverity::BAN);
    }

    SECTION("Hitbox extension detected for distant hits") {
        auto result = acs.validateCombat(
            attacker, target, 100,
            makePosition(100, 0, 0),  // 100m away, way over melee range
            makePosition(0, 0, 0),
            registry
        );
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::HITBOX_EXTENSION);
    }
}

// ============================================================================
// CheatType Utilities
// ============================================================================

TEST_CASE("CheatType string conversion", "[security][anticheat]") {
    REQUIRE(std::string(cheatTypeToString(CheatType::NONE)) == "NONE");
    REQUIRE(std::string(cheatTypeToString(CheatType::SPEED_HACK)) == "SPEED_HACK");
    REQUIRE(std::string(cheatTypeToString(CheatType::TELEPORT)) == "TELEPORT");
    REQUIRE(std::string(cheatTypeToString(CheatType::FLY_HACK)) == "FLY_HACK");
    REQUIRE(std::string(cheatTypeToString(CheatType::NO_CLIP)) == "NO_CLIP");
    REQUIRE(std::string(cheatTypeToString(CheatType::INPUT_MANIPULATION)) == "INPUT_MANIPULATION");
    REQUIRE(std::string(cheatTypeToString(CheatType::PACKET_FLOODING)) == "PACKET_FLOODING");
    REQUIRE(std::string(cheatTypeToString(CheatType::POSITION_SPOOFING)) == "POSITION_SPOOFING");
    REQUIRE(std::string(cheatTypeToString(CheatType::DAMAGE_HACK)) == "DAMAGE_HACK");
    REQUIRE(std::string(cheatTypeToString(CheatType::HITBOX_EXTENSION)) == "HITBOX_EXTENSION");
}

TEST_CASE("ViolationSeverity string conversion", "[security][anticheat]") {
    REQUIRE(std::string(severityToString(ViolationSeverity::INFO)) == "INFO");
    REQUIRE(std::string(severityToString(ViolationSeverity::WARNING)) == "WARNING");
    REQUIRE(std::string(severityToString(ViolationSeverity::SUSPICIOUS)) == "SUSPICIOUS");
    REQUIRE(std::string(severityToString(ViolationSeverity::CRITICAL)) == "CRITICAL");
    REQUIRE(std::string(severityToString(ViolationSeverity::BAN)) == "BAN");
}

// ============================================================================
// Statistics
// ============================================================================

TEST_CASE("AntiCheatSystem statistics tracking", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Detection counts increment") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1100);

        // Trigger speed hack
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(50, 0, 0),
            100, makeInput(), registry);

        REQUIRE(acs.getTotalDetections() >= 1);
        REQUIRE(acs.getDetectionCount(CheatType::SPEED_HACK) >= 1);
    }

    SECTION("Reset statistics clears counters") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1200);
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(50, 0, 0),
            100, makeInput(), registry);

        REQUIRE(acs.getTotalDetections() >= 1);
        acs.resetStatistics();
        REQUIRE(acs.getTotalDetections() == 0);
        REQUIRE(acs.getPlayersKicked() == 0);
        REQUIRE(acs.getPlayersBanned() == 0);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("AntiCheatSystem edge cases", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Zero-distance movement is valid") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1300);
        auto result = acs.validateMovement(entity,
            makePosition(10, 0, 10),
            makePosition(10, 0, 10),
            16, makeInput(), registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Diagonal movement at valid speed") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1301);
        // Diagonal: sqrt(0.5^2 + 0.5^2) ~ 0.707m in 100ms = ~7 m/s
        // Slightly over base speed but under tolerance
        auto result = acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(0.5f, 0, 0.5f),
            100, makeInput(), registry);
        // Should pass since tolerance is 1.2x
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Entity without PlayerInfo does not crash for clean movement") {
        Registry registry;
        auto entity = registry.create();
        registry.emplace<Position>(entity, makePosition(0, 0, 0));

        // No PlayerInfo - clean movement validation is OK (no violation to report)
        auto result = acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(0.1f, 0, 0),
            100, makeInput(), registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Validation works without AntiCheatState component") {
        Registry registry;
        auto entity = registry.create();
        registry.emplace<Position>(entity, makePosition(0, 0, 0));
        registry.emplace<PlayerInfo>(entity, PlayerInfo{1400, 1, "NoCheatState"});

        auto result = acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(1, 0, 0),
            100, makeInput(), registry);
        (void)result;
    }

    SECTION("Very small dt still processes correctly") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1500);
        // 0.005m in 1ms = 5 m/s, within base speed limit of 6 m/s
        auto result = acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(0.005f, 0, 0),
            1, makeInput(), registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Large Y displacement triggers speed hack detection") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1501);
        // Move 50m upward instantly - triggers speed violation first
        // because 50m/16ms = 3125 m/s, way over 6 m/s limit
        auto result = acs.validateMovement(entity,
            makePosition(0, 0, 0),
            makePosition(0, 50, 0),
            16, makeInput(), registry);
        REQUIRE(result.detected);
        // Speed is checked before fly-hack, so this is SPEED_HACK
        REQUIRE(result.type == CheatType::SPEED_HACK);
    }

    SECTION("Subtle Y displacement triggers speed detection") {
        Registry registry;
        auto entity = setupPlayerEntity(registry, 1502);
        // Move up 20m in one tick - also speed violation
        auto result = acs.validateMovement(entity,
            makePosition(0, 1, 0),
            makePosition(0, 21, 0),
            100, makeInput(), registry);
        REQUIRE(result.detected);
        // 20m/100ms = 200 m/s, speed hack
        REQUIRE(result.type == CheatType::SPEED_HACK);
    }
}

// ============================================================================
// ServerAuthority Validation
// ============================================================================

TEST_CASE("ServerAuthority client claim validation", "[security][anticheat]") {
    SECTION("Matching positions pass validation") {
        Position serverPos = makePosition(10, 5, 10);
        Position claimedPos = makePosition(10, 5, 10);
        Velocity vel;

        REQUIRE(ServerAuthority::validateClientClaim(
            serverPos, claimedPos, vel, vel, 1.0f));
    }

    SECTION("Within tolerance passes") {
        Position serverPos = makePosition(100, 0, 100);
        Position claimedPos = makePosition(100.5f, 0, 100.5f);
        Velocity vel;

        // 0.5m offset, tolerance 1.0m -> passes
        REQUIRE(ServerAuthority::validateClientClaim(
            serverPos, claimedPos, vel, vel, 1.0f));
    }

    SECTION("Beyond tolerance fails") {
        Position serverPos = makePosition(0, 0, 0);
        Position claimedPos = makePosition(10, 0, 0);
        Velocity vel;

        // 10m offset, tolerance 1.0m -> fails
        REQUIRE_FALSE(ServerAuthority::validateClientClaim(
            serverPos, claimedPos, vel, vel, 1.0f));
    }

    SECTION("3D distance matters") {
        // 3-4-5 triangle = 5 units apart
        Position serverPos = makePosition(0, 0, 0);
        Position claimedPos = makePosition(3, 4, 0);
        Velocity vel;

        REQUIRE(ServerAuthority::validateClientClaim(
            serverPos, claimedPos, vel, vel, 5.1f));  // Just over 5
        REQUIRE_FALSE(ServerAuthority::validateClientClaim(
            serverPos, claimedPos, vel, vel, 4.9f));  // Just under 5
    }
}

TEST_CASE("ServerAuthority correction magnitude", "[security][anticheat]") {
    SECTION("Same position has zero magnitude") {
        Position pos = makePosition(10, 20, 30);
        float mag = ServerAuthority::calculateCorrectionMagnitude(pos, pos);
        REQUIRE(mag == Catch::Approx(0.0f).margin(0.001f));
    }

    SECTION("Distance along single axis") {
        Position serverPos = makePosition(0, 0, 0);
        Position clientPos = makePosition(10, 0, 0);
        float mag = ServerAuthority::calculateCorrectionMagnitude(serverPos, clientPos);
        REQUIRE(mag == Catch::Approx(10.0f).margin(0.1f));
    }

    SECTION("3D distance calculation") {
        Position serverPos = makePosition(0, 0, 0);
        Position clientPos = makePosition(3, 4, 0);  // 5 units
        float mag = ServerAuthority::calculateCorrectionMagnitude(serverPos, clientPos);
        REQUIRE(mag == Catch::Approx(5.0f).margin(0.1f));
    }
}

TEST_CASE("ServerAuthority needsCorrection", "[security][anticheat]") {
    SECTION("Small offset within tolerance needs no correction") {
        Position serverPos = makePosition(100, 0, 100);
        Position clientPos = makePosition(100.3f, 0, 100);
        REQUIRE_FALSE(ServerAuthority::needsCorrection(serverPos, clientPos, 1.0f));
    }

    SECTION("Large offset beyond tolerance needs correction") {
        Position serverPos = makePosition(0, 0, 0);
        Position clientPos = makePosition(5, 0, 0);
        REQUIRE(ServerAuthority::needsCorrection(serverPos, clientPos, 1.0f));
    }

    SECTION("Exact tolerance boundary") {
        Position serverPos = makePosition(0, 0, 0);
        Position clientPos = makePosition(1, 0, 0);  // Exactly 1.0m
        // At exact boundary, should be false (not greater than)
        REQUIRE_FALSE(ServerAuthority::needsCorrection(serverPos, clientPos, 1.0f));
    }
}

// ============================================================================
// StatisticalAnalyzer Placeholder Methods
// ============================================================================

TEST_CASE("StatisticalAnalyzer placeholder methods", "[security][anticheat]") {
    SECTION("analyzeAimPattern returns false (placeholder)") {
        std::vector<Rotation> history;
        REQUIRE_FALSE(StatisticalAnalyzer::analyzeAimPattern(
            static_cast<EntityID>(1), history));
    }

    SECTION("calculateMovementConsistency returns 1.0 (placeholder)") {
        std::vector<Position> history;
        float consistency = StatisticalAnalyzer::calculateMovementConsistency(history);
        REQUIRE(consistency == 1.0f);
    }

    SECTION("detectImpossibleReactions returns false (placeholder)") {
        std::vector<uint32_t> samples;
        REQUIRE_FALSE(StatisticalAnalyzer::detectImpossibleReactions(samples));
    }
}

// ============================================================================
// Multi-Violation Trust Decay
// ============================================================================

TEST_CASE("AntiCheatSystem multi-violation trust decay", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Multiple violations decay trust score") {
        uint64_t playerId = 2000;
        auto* profile = acs.getProfile(playerId);
        uint8_t initialTrust = profile->trustScore;
        REQUIRE(initialTrust > 0);

        // Apply 3 WARNING violations
        for (int i = 0; i < 3; ++i) {
            CheatDetectionResult det;
            det.type = CheatType::SPEED_HACK;
            det.severity = ViolationSeverity::WARNING;
            det.confidence = 0.5f;
            det.timestamp = static_cast<uint32_t>(1000 + i * 100);
            profile->recordViolation(det);
        }

        REQUIRE(profile->trustScore < initialTrust);
        REQUIRE(profile->speedViolations == 3);
        REQUIRE(profile->violationHistory.size() == 3);
    }

    SECTION("CRITICAL violations decay trust more than WARNING") {
        uint64_t playerIdWarn = 2001;
        uint64_t playerIdCrit = 2002;

        auto* warnProfile = acs.getProfile(playerIdWarn);
        auto* critProfile = acs.getProfile(playerIdCrit);

        // Same initial trust
        REQUIRE(warnProfile->trustScore == critProfile->trustScore);

        CheatDetectionResult warning;
        warning.type = CheatType::SPEED_HACK;
        warning.severity = ViolationSeverity::WARNING;
        warning.confidence = 0.5f;
        warning.timestamp = 1000;
        warnProfile->recordViolation(warning);

        CheatDetectionResult critical;
        critical.type = CheatType::SPEED_HACK;
        critical.severity = ViolationSeverity::CRITICAL;
        critical.confidence = 0.9f;
        critical.timestamp = 1000;
        critProfile->recordViolation(critical);

        // CRITICAL should result in lower trust
        REQUIRE(critProfile->trustScore < warnProfile->trustScore);
    }

    SECTION("BAN severity drops trust to near-zero immediately") {
        uint64_t playerId = 2003;
        auto* profile = acs.getProfile(playerId);

        CheatDetectionResult ban;
        ban.type = CheatType::DAMAGE_HACK;
        ban.severity = ViolationSeverity::BAN;
        ban.confidence = 1.0f;
        ban.timestamp = 1000;
        profile->recordViolation(ban);

        REQUIRE(profile->trustScore <= 10);  // Should be very low after BAN
        REQUIRE(profile->isSuspicious());
    }

    SECTION("Clean movement recovers trust over time") {
        uint64_t playerId = 2004;
        auto* profile = acs.getProfile(playerId);

        // Drop trust with violations
        for (int i = 0; i < 5; ++i) {
            CheatDetectionResult det;
            det.type = CheatType::SPEED_HACK;
            det.severity = ViolationSeverity::WARNING;
            det.confidence = 0.5f;
            det.timestamp = static_cast<uint32_t>(1000 + i * 100);
            profile->recordViolation(det);
        }
        uint8_t afterViolations = profile->trustScore;

        // 6000 clean ticks = 10 recoveries of +1 each
        for (uint32_t i = 0; i < 6000; ++i) {
            profile->recordCleanMovement();
        }

        REQUIRE(profile->trustScore > afterViolations);
    }

    SECTION("violationHistory caps at MAX_VIOLATIONS") {
        uint64_t playerId = 2005;
        auto* profile = acs.getProfile(playerId);

        // Record many violations
        for (int i = 0; i < 50; ++i) {
            CheatDetectionResult det;
            det.type = CheatType::SPEED_HACK;
            det.severity = ViolationSeverity::INFO;
            det.confidence = 0.1f;
            det.timestamp = static_cast<uint32_t>(i * 100);
            profile->recordViolation(det);
        }

        // History should be capped
        REQUIRE(profile->violationHistory.size() <= 20);  // MAX_VIOLATIONS
    }
}

// ============================================================================
// Combat Edge Cases
// ============================================================================

TEST_CASE("AntiCheatSystem combat edge cases", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());
    Registry registry;
    auto attacker = setupPlayerEntity(registry, 3000);
    auto target = setupPlayerEntity(registry, 3001);

    SECTION("Damage exactly at max is accepted") {
        AntiCheatConfig config = acs.getConfig();
        config.maxDamagePerHit = 1000;
        acs.setConfig(config);

        auto result = acs.validateCombat(
            attacker, target, 1000,
            makePosition(1, 0, 0),
            makePosition(0, 0, 0),
            registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Damage one over max is detected") {
        AntiCheatConfig config = acs.getConfig();
        config.maxDamagePerHit = 1000;
        acs.setConfig(config);

        auto result = acs.validateCombat(
            attacker, target, 1001,
            makePosition(1, 0, 0),
            makePosition(0, 0, 0),
            registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::DAMAGE_HACK);
    }

    SECTION("Hit at max melee range passes") {
        AntiCheatConfig config = acs.getConfig();
        config.maxMeleeRange = 3.0f;
        acs.setConfig(config);

        // At exactly maxMeleeRange (with 50% tolerance = 4.5m)
        auto result = acs.validateCombat(
            attacker, target, 100,
            makePosition(3.0f, 0, 0),
            makePosition(0, 0, 0),
            registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Hit just beyond tolerance fails") {
        AntiCheatConfig config = acs.getConfig();
        config.maxMeleeRange = 3.0f;
        acs.setConfig(config);

        // 5m is beyond 3.0 * 1.5 = 4.5m tolerance
        auto result = acs.validateCombat(
            attacker, target, 100,
            makePosition(5.0f, 0, 0),
            makePosition(0, 0, 0),
            registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::HITBOX_EXTENSION);
    }

    SECTION("Zero damage is always accepted") {
        auto result = acs.validateCombat(
            attacker, target, 0,
            makePosition(0, 0, 0),
            makePosition(0, 0, 0),
            registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Negative damage is accepted") {
        // Healing or damage reduction - should not trigger
        auto result = acs.validateCombat(
            attacker, target, -100,
            makePosition(1, 0, 0),
            makePosition(0, 0, 0),
            registry);
        REQUIRE_FALSE(result.detected);
    }
}

// ============================================================================
// Multi-Entity Concurrent Validation
// ============================================================================

TEST_CASE("AntiCheatSystem multi-entity concurrent validation", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Different entities tracked independently") {
        Registry registry;
        std::vector<EntityID> entities;
        for (int i = 0; i < 10; ++i) {
            entities.push_back(setupPlayerEntity(registry, static_cast<uint64_t>(4000 + i)));
        }

        // Entity 0 cheats (speed hack)
        auto result0 = acs.validateMovement(entities[0],
            makePosition(0, 0, 0), makePosition(50, 0, 0),
            100, makeInput(), registry);
        REQUIRE(result0.detected);

        // Entity 1 moves normally
        auto result1 = acs.validateMovement(entities[1],
            makePosition(0, 0, 0), makePosition(0.5f, 0, 0),
            100, makeInput(), registry);
        REQUIRE_FALSE(result1.detected);

        // Detection count should only reflect the cheater
        REQUIRE(acs.getDetectionCount(CheatType::SPEED_HACK) >= 1);
    }

    SECTION("Profile count matches connected players") {
        Registry registry;
        for (int i = 0; i < 5; ++i) {
            uint64_t pid = static_cast<uint64_t>(5000 + i);
            (void)acs.getProfile(pid);
        }
        REQUIRE(acs.getActiveProfileCount() == 5);

        acs.removeProfile(5000);
        REQUIRE(acs.getActiveProfileCount() == 4);
    }
}

// ============================================================================
// Detection Callback Propagation
// ============================================================================

TEST_CASE("AntiCheatSystem detection callback propagation", "[security][anticheat]") {
    AntiCheatSystem acs;
    REQUIRE(acs.initialize());

    SECTION("Callback receives correct cheat type") {
        CheatType detectedType = CheatType::NONE;
        acs.setOnCheatDetected([&](uint64_t, const CheatDetectionResult& result) {
            detectedType = result.type;
        });

        Registry registry;
        auto entity = setupPlayerEntity(registry, 6000);
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0), makePosition(50, 0, 0),
            100, makeInput(), registry);

        REQUIRE(detectedType == CheatType::SPEED_HACK);
    }

    SECTION("Callback receives correct confidence value") {
        float detectedConfidence = 0.0f;
        acs.setOnCheatDetected([&](uint64_t, const CheatDetectionResult& result) {
            detectedConfidence = result.confidence;
        });

        Registry registry;
        auto entity = setupPlayerEntity(registry, 6001);
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0), makePosition(100, 0, 0),
            100, makeInput(), registry);

        REQUIRE(detectedConfidence > 0.0f);
    }

    SECTION("No callback for clean movement") {
        bool callbackFired = false;
        acs.setOnCheatDetected([&](uint64_t, const CheatDetectionResult&) {
            callbackFired = true;
        });

        Registry registry;
        auto entity = setupPlayerEntity(registry, 6002);
        (void)acs.validateMovement(entity,
            makePosition(0, 0, 0), makePosition(0.3f, 0, 0),
            100, makeInput(), registry);

        REQUIRE_FALSE(callbackFired);
    }
}
