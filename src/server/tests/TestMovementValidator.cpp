// [SECURITY_AGENT] Unit tests for MovementValidator subsystem

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "security/MovementValidator.hpp"
#include "security/ViolationTracker.hpp"
#include "physics/SpatialHash.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;
using namespace DarkAges::Security;

// Helper to create a MovementValidator with a ViolationTracker
static std::pair<MovementValidator, std::shared_ptr<ViolationTracker>> createValidator() {
    MovementValidator validator;
    auto tracker = std::make_shared<ViolationTracker>();
    validator.setViolationTracker(tracker.get());
    return {std::move(validator), tracker};
}

// ============================================================================
// Speed Hack Detection
// ============================================================================

TEST_CASE("MovementValidator speed hack detection", "[security][movement]") {
    auto [validator, tracker] = createValidator();
    Registry registry;

    EntityID player = registry.create();
    registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

    SECTION("Normal movement is not flagged") {
        // 1 meter in 1 second = 1 m/s (well under 6 m/s limit)
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(1.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.detectSpeedHack(player, oldPos, newPos, 1000, input, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Sprint speed is allowed") {
        // 8 meters in 1 second = 8 m/s (under 9 m/s sprint limit with tolerance)
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(8.0f, 0.0f, 0.0f), 0);
        InputState input;
        input.sprint = 1;

        auto result = validator.detectSpeedHack(player, oldPos, newPos, 1000, input, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Detects obvious speed hack") {
        // 100 meters in 1 second = 100 m/s (way over limit)
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.detectSpeedHack(player, oldPos, newPos, 1000, input, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::SPEED_HACK);
        REQUIRE(result.severity == ViolationSeverity::CRITICAL);
        REQUIRE(result.confidence > 0.0f);
    }

    SECTION("Detects speed hack just over tolerance") {
        // Max speed 6 + 20% tolerance = 7.2 m/s. Moving at ~10 m/s should be detected.
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(10.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.detectSpeedHack(player, oldPos, newPos, 1000, input, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::SPEED_HACK);
    }

    SECTION("Zero dt does not crash") {
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.detectSpeedHack(player, oldPos, newPos, 0, input, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Corrected position reverts to old position") {
        Position oldPos = Position::fromVec3(glm::vec3(5.0f, 0.0f, 3.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(500.0f, 0.0f, 3.0f), 0);
        InputState input;

        auto result = validator.detectSpeedHack(player, oldPos, newPos, 100, input, registry);
        REQUIRE(result.detected);
        REQUIRE(result.correctedPosition.x == oldPos.x);
        REQUIRE(result.correctedPosition.z == oldPos.z);
    }
}

// ============================================================================
// Teleport Detection
// ============================================================================

TEST_CASE("MovementValidator teleport detection", "[security][movement]") {
    auto [validator, tracker] = createValidator();
    Registry registry;

    EntityID player = registry.create();
    registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

    SECTION("Short distance is not teleport") {
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(10.0f, 0.0f, 0.0f), 0);

        auto result = validator.detectTeleport(player, oldPos, newPos, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Detects large position jump") {
        // > 100 meters is a teleport
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(500.0f, 0.0f, 0.0f), 0);

        auto result = validator.detectTeleport(player, oldPos, newPos, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::TELEPORT);
        REQUIRE(result.confidence == 1.0f);
    }

    SECTION("Teleport severity is BAN when instantBanOnTeleport is true") {
        AntiCheatConfig config;
        config.instantBanOnTeleport = true;
        validator.setConfig(config);

        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(1000.0f, 0.0f, 0.0f), 0);

        auto result = validator.detectTeleport(player, oldPos, newPos, registry);
        REQUIRE(result.detected);
        REQUIRE(result.severity == ViolationSeverity::BAN);
    }

    SECTION("Teleport severity is CRITICAL when instantBanOnTeleport is false") {
        AntiCheatConfig config;
        config.instantBanOnTeleport = false;
        validator.setConfig(config);

        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(1000.0f, 0.0f, 0.0f), 0);

        auto result = validator.detectTeleport(player, oldPos, newPos, registry);
        REQUIRE(result.detected);
        REQUIRE(result.severity == ViolationSeverity::CRITICAL);
    }

    SECTION("Corrected position is old position on teleport") {
        Position oldPos = Position::fromVec3(glm::vec3(1.0f, 0.0f, 2.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(999.0f, 0.0f, 999.0f), 0);

        auto result = validator.detectTeleport(player, oldPos, newPos, registry);
        REQUIRE(result.detected);
        REQUIRE(result.correctedPosition.x == oldPos.x);
        REQUIRE(result.correctedPosition.z == oldPos.z);
    }
}

// ============================================================================
// Fly Hack Detection
// ============================================================================

TEST_CASE("MovementValidator fly hack detection", "[security][movement]") {
    auto [validator, tracker] = createValidator();
    Registry registry;

    EntityID player = registry.create();
    registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

    SECTION("Detects position above world max Y") {
        // WORLD_MAX_Y = 500
        Position pos = Position::fromVec3(glm::vec3(0.0f, 600.0f, 0.0f), 0);
        Velocity vel{0, 0, 0};
        InputState input;

        auto result = validator.detectFlyHack(player, pos, vel, input, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::FLY_HACK);
        REQUIRE(result.confidence == 1.0f);
    }

    SECTION("Corrected Y is clamped to world max") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 600.0f, 0.0f), 0);
        Velocity vel{0, 0, 0};
        InputState input;

        auto result = validator.detectFlyHack(player, pos, vel, input, registry);
        REQUIRE(result.detected);

        float correctedY = result.correctedPosition.y * Constants::FIXED_TO_FLOAT;
        // Allow small float precision margin from fixed-point round-trip
        REQUIRE(correctedY <= Constants::WORLD_MAX_Y + 0.01f);
    }

    SECTION("Normal height is not flagged") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 1.0f, 0.0f), 0);
        Velocity vel{0, 0, 0};
        InputState input;

        auto result = validator.detectFlyHack(player, pos, vel, input, registry);
        // Should not be detected (below max Y, grounded default)
        REQUIRE_FALSE(result.detected);
    }
}

// ============================================================================
// No-Clip Detection
// ============================================================================

TEST_CASE("MovementValidator no-clip detection", "[security][movement]") {
    auto [validator, tracker] = createValidator();
    Registry registry;

    SECTION("Graceful degradation when spatial hash is null") {
        EntityID player = registry.create();
        registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(5.0f, 0.0f, 0.0f), 0);

        // No spatial hash set - should skip detection gracefully
        auto result = validator.detectNoClip(player, oldPos, newPos, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("No collision with spatial hash but no geometry") {
        SpatialHash spatialHash;
        validator.setSpatialHash(&spatialHash);

        EntityID player = registry.create();
        registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(5.0f, 0.0f, 0.0f), 0);

        auto result = validator.detectNoClip(player, oldPos, newPos, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Detects no-clip with overlapping static geometry") {
        SpatialHash spatialHash;
        validator.setSpatialHash(&spatialHash);

        // Create a wall
        EntityID wall = registry.create();
        Position wallPos = Position::fromVec3(glm::vec3(2.5f, 0.0f, 0.0f), 0);
        registry.emplace<Position>(wall, wallPos);
        registry.emplace<BoundingVolume>(wall, 2.0f, 3.0f);
        registry.emplace<StaticTag>(wall);
        spatialHash.insert(wall, wallPos);

        EntityID player = registry.create();
        registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});
        registry.emplace<BoundingVolume>(player, 0.5f, 1.8f);

        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(2.5f, 0.0f, 0.0f), 0);

        auto result = validator.detectNoClip(player, oldPos, newPos, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::NO_CLIP);
    }

    SECTION("No false positive when far from geometry") {
        SpatialHash spatialHash;
        validator.setSpatialHash(&spatialHash);

        EntityID wall = registry.create();
        Position wallPos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 0.0f), 0);
        registry.emplace<Position>(wall, wallPos);
        registry.emplace<BoundingVolume>(wall, 2.0f, 3.0f);
        registry.emplace<StaticTag>(wall);
        spatialHash.insert(wall, wallPos);

        EntityID player = registry.create();
        registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(5.0f, 0.0f, 0.0f), 0);

        auto result = validator.detectNoClip(player, oldPos, newPos, registry);
        REQUIRE_FALSE(result.detected);
    }
}

// ============================================================================
// Position Bounds Validation
// ============================================================================

TEST_CASE("MovementValidator position bounds validation", "[security][movement]") {
    MovementValidator validator;

    SECTION("Valid position within bounds") {
        Position pos = Position::fromVec3(glm::vec3(100.0f, 50.0f, -200.0f), 0);
        auto result = validator.validatePositionBounds(pos);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Detects X out of bounds") {
        Position pos = Position::fromVec3(glm::vec3(9999.0f, 0.0f, 0.0f), 0);
        auto result = validator.validatePositionBounds(pos);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::POSITION_SPOOFING);
    }

    SECTION("Detects Y out of bounds") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 999.0f, 0.0f), 0);
        auto result = validator.validatePositionBounds(pos);
        REQUIRE(result.detected);
    }

    SECTION("Detects Z out of bounds") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, -9999.0f), 0);
        auto result = validator.validatePositionBounds(pos);
        REQUIRE(result.detected);
    }

    SECTION("Boundary edge values are valid") {
        // Use positions slightly inside the boundary to avoid float precision
        // issues in the fixed-point round-trip conversion at exact max values
        Position pos = Position::fromVec3(glm::vec3(
            Constants::WORLD_MAX_X - 0.1f, 
            Constants::WORLD_MAX_Y - 0.1f, 
            Constants::WORLD_MAX_Z - 0.1f), 0);
        auto result = validator.validatePositionBounds(pos);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Corrected position clamps to bounds") {
        Position pos = Position::fromVec3(glm::vec3(9999.0f, 0.0f, 0.0f), 0);
        auto result = validator.validatePositionBounds(pos);
        REQUIRE(result.detected);

        float correctedX = result.correctedPosition.x * Constants::FIXED_TO_FLOAT;
        REQUIRE(correctedX <= Constants::WORLD_MAX_X);
    }
}

// ============================================================================
// Utility Methods
// ============================================================================

TEST_CASE("MovementValidator utility methods", "[security][movement]") {
    MovementValidator validator;

    SECTION("calculateDistance returns correct 3D distance") {
        Position a = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position b = Position::fromVec3(glm::vec3(3.0f, 4.0f, 0.0f), 0);

        float dist = validator.calculateDistance(a, b);
        REQUIRE(dist == Catch::Approx(5.0f).margin(0.01f));
    }

    SECTION("calculateSpeed returns zero for zero dt") {
        Position a = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position b = Position::fromVec3(glm::vec3(10.0f, 0.0f, 0.0f), 0);

        float speed = validator.calculateSpeed(a, b, 0);
        REQUIRE(speed == 0.0f);
    }

    SECTION("calculateSpeed returns correct speed") {
        Position a = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position b = Position::fromVec3(glm::vec3(10.0f, 0.0f, 0.0f), 0);

        // 10 meters in 1000ms = 10 m/s
        float speed = validator.calculateSpeed(a, b, 1000);
        REQUIRE(speed == Catch::Approx(10.0f).margin(0.01f));
    }

    SECTION("calculateMaxAllowedSpeed returns normal speed without sprint") {
        InputState input;
        float speed = validator.calculateMaxAllowedSpeed(input);
        REQUIRE(speed == Catch::Approx(Constants::MAX_PLAYER_SPEED).margin(0.01f));
    }

    SECTION("calculateMaxAllowedSpeed returns sprint speed with sprint") {
        InputState input;
        input.sprint = 1;
        float speed = validator.calculateMaxAllowedSpeed(input);
        REQUIRE(speed == Catch::Approx(Constants::MAX_SPRINT_SPEED).margin(0.01f));
    }

    SECTION("isWithinWorldBounds returns true for valid position") {
        Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        REQUIRE(validator.isWithinWorldBounds(pos));
    }

    SECTION("isWithinWorldBounds returns false for out-of-bounds position") {
        Position pos = Position::fromVec3(glm::vec3(99999.0f, 0.0f, 0.0f), 0);
        REQUIRE_FALSE(validator.isWithinWorldBounds(pos));
    }

    SECTION("applyPositionCorrection updates entity position") {
        Registry registry;
        EntityID entity = registry.create();
        Position pos = Position::fromVec3(glm::vec3(10.0f, 0.0f, 0.0f), 0);
        registry.emplace<Position>(entity, pos);

        Position corrected = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        validator.applyPositionCorrection(entity, corrected, registry);

        Position& updated = registry.get<Position>(entity);
        REQUIRE(updated.x == corrected.x);
        REQUIRE(updated.z == corrected.z);
    }
}

// ============================================================================
// Full Validation Pipeline
// ============================================================================

TEST_CASE("MovementValidator full validation pipeline", "[security][movement]") {
    auto [validator, tracker] = createValidator();
    Registry registry;

    EntityID player = registry.create();
    registry.emplace<PlayerInfo>(player, PlayerInfo{1, 0, "test", 0});

    SECTION("Clean movement passes all checks") {
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(3.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.validateMovement(player, oldPos, newPos, 1000, input, registry);
        REQUIRE_FALSE(result.detected);
    }

    SECTION("Out-of-bounds position is caught first") {
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(99999.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.validateMovement(player, oldPos, newPos, 1000, input, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::POSITION_SPOOFING);
    }

    SECTION("Teleport is caught even if speed would be OK") {
        // Teleport is checked before speed hack
        Position oldPos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0);
        Position newPos = Position::fromVec3(glm::vec3(200.0f, 0.0f, 0.0f), 0);
        InputState input;

        auto result = validator.validateMovement(player, oldPos, newPos, 1000, input, registry);
        REQUIRE(result.detected);
        REQUIRE(result.type == CheatType::TELEPORT);
    }
}
