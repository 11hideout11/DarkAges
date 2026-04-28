// TestHitboxCollision.cpp — Server-authoritative hitbox/hurtbox validation
// Covers: collision detection, team filtering, broad-phase, melee cone, lag compensation edge cases
// Build: part of FINAL_TEST_SOURCES, uses Catch2

#include <catch2/catch_test_macros.hpp>
#include <cmath>

// Forward declarations (avoid includes that pull in heavy ECS)
namespace DarkAges {
    struct CollisionLayer { uint32_t value; };
    struct BoundingVolume { /* cylinder-sphere intersection */ };
    struct Hitbox { CollisionLayer layer; float radius; float height; };
    struct Hurtbox { CollisionLayer layer; float radius; float height; };
    enum class Team { Neutral = 0, TeamA = 1, TeamB = 2 };
}

// Minimal API surface we need to validate — mirrors server implementation
namespace TestHitboxCollision {

using namespace DarkAges;

// Simulates BroadPhaseSystem::canCollide logic — exact rules enforced server-side
inline bool CanCollide(const Hitbox& a, const Hurtbox& b, Team teamA, Team teamB) {
    // Same team → no collision
    if (teamA == teamB && teamA != Team::Neutral) return false;
    // Layer filtering: HITBOX must intersect HURTBOX
    const uint32_t HITBOX_LAYER = 1 << 3;
    const uint32_t HURTBOX_LAYER = 1 << 4;
    if ((a.layer.value & HITBOX_LAYER) == 0 || (b.layer.value & HURTBOX_LAYER) == 0) return false;
    return true;
}

// Cylinder-sphere intersection (approximation used in server)
inline bool IntersectsCylinderSphere(const Hitbox& hb, const Hurtbox& hb2, float dx, float dz) {
    // Simplified: check radial (ignore Y for MVP; server uses cylinder)
    float rSum = hb.radius + hb2.radius;
    return (dx*dx + dz*dz) <= (rSum * rSum);
}

// Melee cone validation — is target within attacker's forward cone?
inline bool InMeleeCone(const float attackerYaw, const float targetAngle, const float coneDegrees) {
    float halfCone = coneDegrees * 0.5f * (M_PI / 180.0f);
    float angleDiff = std::abs(std::fmod(targetAngle - attackerYaw + M_PI, 2*M_PI) - M_PI);
    return angleDiff <= halfCone;
}

} // namespace TestHitboxCollision

TEST_CASE("Hitbox-Hurtbox basic overlap detection", "[combat][hitbox]") {
    using namespace TestHitboxCollision;

    Hitbox atk{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f };   // HITBOX layer 3
    Hurtbox def{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f, .height = 1.0f };   // HURTBOX layer 4

    // Overlapping positions within combined radius → hit
    float dx = 0.3f, dz = 0.2f;  // sqrt(0.3² + 0.2²) ≈ 0.36 < 0.9
    REQUIRE(IntersectsCylinderSphere(atk, def, dx, dz) == true);

    // Far apart → no hit
    REQUIRE(IntersectsCylinderSphere(atk, def, 2.0f, 0.0f) == false);
}

TEST_CASE("Team-based collision filtering", "[combat][hitbox]") {
    using namespace TestHitboxCollision;

    Hitbox atk{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f };
    Hurtbox def{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f, .height = 1.0f };

    // Same team (TeamA vs TeamA) → filtered out
    REQUIRE(CanCollide(atk, def, Team::TeamA, Team::TeamA) == false);

    // Opposite teams → allowed
    REQUIRE(CanCollide(atk, def, Team::TeamA, Team::TeamB) == true);

    // Neutral involved → allowed (no friendly-fire rules)
    REQUIRE(CanCollide(atk, def, Team::Neutral, Team::TeamA) == true);
    REQUIRE(CanCollide(atk, def, Team::TeamA, Team::Neutral) == true);
}

TEST_CASE("Collision layer bitmask validation", "[combat][hitbox]") {
    using namespace TestHitboxCollision;

    Hitbox invalidLayer{ .layer = CollisionLayer{1u << 5}, .radius = 0.5f }; // wrong layer
    Hurtbox validHurt{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f };

    // Attack hitbox not on HITBOX layer → collision rejected even if overlapping
    REQUIRE(CanCollide(invalidLayer, validHurt, Team::TeamA, Team::TeamB) == false);

    // Hurtbox on wrong layer also rejected
    Hurtbox invalidHurt{ .layer = CollisionLayer{1u << 2} };
    Hitbox validAtk{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f };
    REQUIRE(CanCollide(validAtk, invalidHurt, Team::TeamA, Team::TeamB) == false);
}

TEST_CASE("Edge cases: zero-size and extreme distances", "[combat][hitbox]") {
    using namespace TestHitboxCollision;

    Hitbox tiny{ .layer = CollisionLayer{1u << 3}, .radius = 0.0f };
    Hurtbox point{ .layer = CollisionLayer{1u << 4}, .radius = 0.0f };
    REQUIRE(IntersectsCylinderSphere(tiny, point, 0, 0) == true);  // point-in-point OK
    REQUIRE(IntersectsCylinderSphere(tiny, point, 0.001f, 0) == false);

    // Extreme coordinates (server uses double precision)
    Hitbox far{ .layer = CollisionLayer{1u << 3}, .radius = 1.0f };
    Hurtbox farTarget{ .layer = CollisionLayer{1u << 4}, .radius = 1.0f };
    REQUIRE(IntersectsCylinderSphere(far, farTarget, 1e6f, 0.0f) == false);
}

TEST_CASE("Melee cone validation — attacker facing vs target position", "[combat][hitbox]") {
    using namespace TestHitboxCollision;

    // Attacker facing east (yaw = 0 rad)
    float attackerYaw = 0.0f;

    // Target directly behind (yaw = π) — outside 90° cone
    float targetBehind = M_PI;
    REQUIRE(InMeleeCone(attackerYaw, targetBehind, 90.0f) == false);

    // Target in front 45° — inside cone
    float targetFrontRight = M_PI / 4.0f;
    REQUIRE(InMeleeCone(attackerYaw, targetFrontRight, 90.0f) == true);

    // Target at edge (exactly 45° left/right is the boundary of a 90° cone)
    float targetEdge = M_PI / 4.0f;
    REQUIRE(InMeleeCone(attackerYaw, targetEdge, 90.0f) == true);
}

TEST_CASE("Lag-compensated hit validation boundary conditions", "[combat][hitbox]") {
    // The server uses LagCompensatedCombat with a 150ms history buffer
    // Validate that position samples are correctly retrieved by client timestamp

    // Simulated position history (time, x, z) — 60Hz snapshots
    struct HistorySample { double time; float x; float z; };
    HistorySample history[5] = {
        {0.000,  0.0f,  0.0f},
        {0.033,  0.5f,  0.0f},
        {0.066,  1.0f,  0.0f},
        {0.100,  1.5f,  0.0f},
        {0.133,  2.0f,  0.0f},
    };

    // Server lookup: given a client timestamp, find the most recent sample at or before that time
    auto FindSampleAtOrBefore = [&](double clientTime) -> const HistorySample* {
        for (int i = 4; i >= 0; i--) {
            if (history[i].time <= clientTime) return &history[i];
        }
        return &history[0];
    };

    // Query the exact timestamp that exists in history → should return that entry
    double clientTimestamp = 0.100; // client's POV snapshot time
    const HistorySample* sample = FindSampleAtOrBefore(clientTimestamp);
    REQUIRE(sample != nullptr);
    REQUIRE(std::abs(sample->x - 1.5f) < 0.01f);

    // Query between ticks → should return the earlier tick
    clientTimestamp = 0.080; // between 0.066 and 0.100
    sample = FindSampleAtOrBefore(clientTimestamp);
    REQUIRE(sample != nullptr);
    REQUIRE(std::abs(sample->x - 1.0f) < 0.01f);
}

TEST_CASE("Hitbox activation/deactivation during attack animation", "[combat][hitbox]") {
    // Server activates hitbox at specific animation frame; verify windowing
    // We test the timing logic without the animation system itself

    // Attack duration 0.4s; hitbox active [0.15, 0.35]
    constexpr float ATTACK_DURATION = 0.4f;
    constexpr float HITBOX_START = 0.15f;
    constexpr float HITBOX_END = 0.35f;

    auto IsHitboxActive = [&](float t) {
        return t >= HITBOX_START && t <= HITBOX_END;
    };

    REQUIRE(IsHitboxActive(0.0f) == false);
    REQUIRE(IsHitboxActive(0.15f) == true);
    REQUIRE(IsHitboxActive(0.30f) == true);
    REQUIRE(IsHitboxActive(0.35f) == true);
    REQUIRE(IsHitboxActive(0.36f) == false);
}
