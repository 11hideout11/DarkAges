// [TEST_AGENT] Hitbox/Hurtbox Edge-Case Validation
// Tests edge cases not covered in the primary test suite:
// - Multi-hit simultaneous detection (only first registers)
// - Hitbox deactivation during active attack
// - Hurtbox invulnerability frames (iframes)
// - Rewind boundary conditions
// - Hitbox offset + rotation edge cases
// - Multiple hurtboxes on same entity
// Build: part of FINAL_TEST_SOURCES, uses Catch2

#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <functional>
#include <vector>

// Forward declarations - match what server uses
namespace DarkAges {
    struct CollisionLayer { uint32_t value; };
    struct Hitbox { 
        CollisionLayer layer; 
        float radius; 
        float height; 
        bool isActive{true};
        bool hasIframes{false};
    };
    struct Hurtbox { 
        CollisionLayer layer; 
        float radius; 
        float height; 
        bool isActive{true};
        bool isInvulnerable{false};  // iframes
    };
    enum class Team { Neutral = 0, TeamA = 1, TeamB = 2 };
    struct Position { float x, y, z; };
}

// Minimal API surface we need to validate
namespace HitboxEdgeCaseTesting {

using namespace DarkAges;

// Check if hit is valid considering activation state
inline bool CanHit(const Hitbox& hitbox, const Hurtbox& hurtbox, Team teamAttacker, Team teamDefender) {
    if (!hitbox.isActive || !hurtbox.isActive) return false;
    if (teamAttacker == teamDefender && teamAttacker != Team::Neutral) return false;
    if (hurtbox.isInvulnerable) return false;  // iframes blocking
    
    const uint32_t HITBOX_LAYER = 1 << 3;
    const uint32_t HURTBOX_LAYER = 1 << 4;
    if ((hitbox.layer.value & HITBOX_LAYER) == 0 || (hurtbox.layer.value & HURTBOX_LAYER) == 0) return false;
    return true;
}

// Cylinder-sphere intersection (server approximation)
inline bool IntersectsCylinderSphere(const Hitbox& hb, const Hurtbox& hb2, float dx, float dz) {
    float rSum = hb.radius + hb2.radius;
    return (dx*dx + dz*dz) <= (rSum * rSum);
}

// Returns index of the first attacker in the list that scored a hit; -1 if none.
// Models the "first hit wins" deduplication rule: when multiple hitboxes
// overlap the same hurtbox simultaneously, only the first one in processing
// order registers damage.
inline int FirstHitWins(const std::vector<bool>& hitResults) {
    for (int i = 0; i < static_cast<int>(hitResults.size()); ++i) {
        if (hitResults[i]) return i;
    }
    return -1;
}

// Check if target is in melee cone
inline bool InMeleeCone(const float attackerYaw, const float targetAngle, const float coneDegrees) {
    float halfCone = coneDegrees * 0.5f * (M_PI / 180.0f);
    float angleDiff = std::abs(std::fmod(targetAngle - attackerYaw + M_PI, 2*M_PI) - M_PI);
    return angleDiff <= halfCone;
}

} // namespace HitboxEdgeCaseTesting

// Test: Multi-hit simultaneous detection
TEST_CASE("Edge-case: Two hitboxes overlapping same hurtbox - only first registers", 
    "[combat][hitbox][edge-case]") {
    using namespace HitboxEdgeCaseTesting;
    
    Hitbox attacker1{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f };
    Hitbox attacker2{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f };
    Hurtbox target{ .layer = CollisionLayer{1u << 4}, .radius = 0.6f, .height = 1.0f };
    
    // Both hitboxes technically overlap the same target
    bool hit1 = IntersectsCylinderSphere(attacker1, target, 0.3f, 0.2f);  // close
    bool hit2 = IntersectsCylinderSphere(attacker2, target, 0.4f, 0.2f);  // also close
    
    // Both can hit from collision perspective
    REQUIRE(hit1 == true);
    REQUIRE(hit2 == true);
    
    // Apply first-hit-wins rule: only the first attacker in processing order
    // receives credit; the second is blocked because the target was already hit.
    int winner = FirstHitWins({hit1, hit2});
    CHECK(winner == 0);  // First attacker (index 0) gets credit
    CHECK(winner != 1);  // Second attacker does NOT get credit
}

// Test: Hitbox deactivation during active attack
TEST_CASE("Edge-case: Hitbox disabled mid-attack", "[combat][hitbox][edge-case]") {
    using namespace HitboxEdgeCaseTesting;
    
    Hitbox activeHitbox{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f, .isActive = true };
    Hitbox inactiveHitbox{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f, .isActive = false };
    Hurtbox target{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f, .height = 1.0f };
    
    bool activeHit = CanHit(activeHitbox, target, Team::TeamA, Team::TeamB);
    bool inactiveHit = CanHit(inactiveHitbox, target, Team::TeamA, Team::TeamB);
    
    REQUIRE(activeHit == true);
    REQUIRE(inactiveHit == false);
}

// Test: Hurtbox invulnerability frames (iframes)
TEST_CASE("Edge-case: Hurtbox iframes block damage", "[combat][hitbox][edge-case]") {
    using namespace HitboxEdgeCaseTesting;
    
    Hitbox attacker{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f, .height = 1.0f };
    Hurtbox normalTarget{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f, .height = 1.0f, .isInvulnerable = false };
    Hurtbox iframeTarget{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f, .height = 1.0f, .isInvulnerable = true };
    
    bool normalHits = CanHit(attacker, normalTarget, Team::TeamA, Team::TeamB);
    bool iframeBlocks = CanHit(attacker, iframeTarget, Team::TeamA, Team::TeamB);
    
    REQUIRE(normalHits == true);
    REQUIRE(iframeBlocks == false);  // iframes block the hit
    
    // After iframes expire
    Hurtbox postIframeTarget{ .layer = CollisionLayer{1u << 4}, .radius = 0.4f, .height = 1.0f, .isInvulnerable = false };
    bool afterIframes = CanHit(attacker, postIframeTarget, Team::TeamA, Team::TeamB);
    CHECK(afterIframes == true);
}

// Test: Rewind to exact 2.000s boundary
TEST_CASE("Edge-case: Position history rewind boundary", "[combat][hitbox][lag-compensation][edge-case]") {
    // Simulate position history at 60Hz (16.67ms intervals)
    struct HistorySample { double time; float x; float z; };
    HistorySample history[5] = {
        {1.500, 1.5f, 0.0f},
        {1.700, 1.7f, 0.0f},
        {1.900, 1.9f, 0.0f},
        {2.000, 2.0f, 0.0f},  // exactly at boundary
        {2.200, 2.2f, 0.0f},
    };
    
    // Server lookup: find sample at or before client timestamp
    auto FindSampleAtOrBefore = [&](double clientTime) -> const HistorySample* {
        for (int i = 4; i >= 0; i--) {
            if (history[i].time <= clientTime) return &history[i];
        }
        return nullptr;
    };
    
    // Query exactly at 2000ms boundary - should return sample at 2000ms
    const HistorySample* sample = FindSampleAtOrBefore(2.000);
    REQUIRE(sample != nullptr);
    CHECK(std::abs(sample->time - 2.000) < 0.001);
    CHECK(std::abs(sample->x - 2.0f) < 0.01f);
    
    // Query at 1999ms - should return earlier sample
    const HistorySample* beforeBoundary = FindSampleAtOrBefore(1.999);
    REQUIRE(beforeBoundary != nullptr);
    CHECK(beforeBoundary->time < 2.000);
}

// Test: Hitbox offset + rotation edge cases
TEST_CASE("Edge-case: Hitbox offset at various positions", "[combat][hitbox][edge-case]") {
    using namespace HitboxEdgeCaseTesting;
    
    Hitbox hitbox{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f };
    
    // Test at different position offsets relative to target
    struct Offset { float dx, dz; };
    Offset offsets[] = {
        {0.0f, 0.0f},    // center
        {0.5f, 0.0f},    // forward
        {-0.5f, 0.0f},  // backward
        {0.0f, 0.5f},   // right
        {0.0f, -0.5f}, // left
    };
    
    for (const auto& off : offsets) {
        Hurtbox offsetTarget{ .layer = CollisionLayer{1u << 4}, .radius = hitbox.radius };
        // Verify intersection calculation works
        bool hit = IntersectsCylinderSphere(hitbox, offsetTarget, off.dx, off.dz);
        // At 0.5 radius each, combined radius = 1.0; offsets <= 1.0 should be hits
        if (off.dx*off.dx + off.dz*off.dz <= (hitbox.radius + offsetTarget.radius) *
                                               (hitbox.radius + offsetTarget.radius)) {
            CHECK(hit == true);
        }
    }
}

// Test: Multiple hurtboxes on same entity
TEST_CASE("Edge-case: Entity with multiple hitbox regions", "[combat][hitbox][edge-case]") {
    using namespace HitboxEdgeCaseTesting;
    
    Hitbox attacker{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f };
    
    // Entity can have multiple hurtboxes (body hitbox + shield hitbox)
    Hurtbox bodyHurtbox{ .layer = CollisionLayer{1u << 4}, .radius = 0.6f };
    Hurtbox shieldHurtbox{ .layer = CollisionLayer{1u << 4}, .radius = 0.3f };
    
    // Check collision with each region independently
    // Body at 0.8 away: 0.5 + 0.6 = 1.1 would hit
    bool bodyHit = IntersectsCylinderSphere(attacker, bodyHurtbox, 0.8f, 0.0f);
    // Shield at 0.2 away: 0.5 + 0.3 = 0.8 would hit
    bool shieldHit = IntersectsCylinderSphere(attacker, shieldHurtbox, 0.2f, 0.0f);
    
    CHECK((bodyHit || shieldHit));
}

// Test: Max world boundary - no overflow
TEST_CASE("Edge-case: Hitbox at maximum world boundary", "[combat][hitbox][boundary]") {
    using namespace HitboxEdgeCaseTesting;
    
    constexpr float WORLD_MAX = 10000.0f;
    constexpr float WORLD_MIN = -10000.0f;
    
    Hitbox hitbox{ .layer = CollisionLayer{1u << 3}, .radius = 0.5f };
    Hurtbox hurtbox{ .layer = CollisionLayer{1u << 4}, .radius = 0.3f };
    
    const float maxDx = WORLD_MAX - 0.1f;
    const float minDx = WORLD_MIN + 0.1f;
    const float dz = 0.0f;
    const float maxDistanceSq = maxDx * maxDx + dz * dz;
    const float minDistanceSq = minDx * minDx + dz * dz;
    
    // At max boundary - should not overflow
    bool atMaxHit = IntersectsCylinderSphere(hitbox, hurtbox, maxDx, dz);
    // At min boundary - should not underflow
    bool atMinHit = IntersectsCylinderSphere(hitbox, hurtbox, minDx, dz);
    
    // The distance math used for these boundary inputs should remain finite.
    CHECK(std::isfinite(maxDistanceSq));
    CHECK(std::isfinite(minDistanceSq));

    // These positions are far outside the combined radii and should not register as hits.
    CHECK(atMaxHit == false);
    CHECK(atMinHit == false);
}

// Test: Melee cone boundary at exact edge
TEST_CASE("Edge-case: Melee cone at exact boundary", "[combat][hitbox]") {
    using namespace HitboxEdgeCaseTesting;
    
    // 60-degree cone (half-angle = 30 degrees)
    float attackerYaw = 0.0f;
    
    // Exactly at 30 degrees (boundary) - should hit
    bool boundaryHit = InMeleeCone(attackerYaw, 30.0f * (M_PI / 180.0f), 60.0f);
    CHECK(boundaryHit == true);
    
    // At 31 degrees (just outside) - should miss
    bool justOutside = InMeleeCone(attackerYaw, 31.0f * (M_PI / 180.0f), 60.0f);
    CHECK(justOutside == false);
    
    // At exactly 0 (directly in front) - should hit
    bool frontHit = InMeleeCone(attackerYaw, 0.0f, 60.0f);
    CHECK(frontHit == true);
}