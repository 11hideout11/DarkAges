// Core ECS types unit tests - covers Position, Velocity, Rotation, InputState,
// CombatState, BoundingVolume, CollisionLayer, EntityState, and other core types

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ecs/CoreTypes.hpp"

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// Position
// ============================================================================

TEST_CASE("Position default construction", "[foundation]") {
    Position pos{};

    REQUIRE(pos.x == 0);
    REQUIRE(pos.y == 0);
    REQUIRE(pos.z == 0);
    REQUIRE(pos.timestamp_ms == 0);
}

TEST_CASE("Position toVec3 conversion", "[foundation]") {
    Position pos{};
    pos.x = 1000;  // 1.0 in fixed-point
    pos.y = 2000;  // 2.0
    pos.z = 3000;  // 3.0

    auto v = pos.toVec3();
    REQUIRE(v.x == Approx(1.0f).margin(0.001f));
    REQUIRE(v.y == Approx(2.0f).margin(0.001f));
    REQUIRE(v.z == Approx(3.0f).margin(0.001f));
}

TEST_CASE("Position fromVec3 conversion", "[foundation]") {
    auto pos = Position::fromVec3(glm::vec3(2.5f, -1.0f, 0.0f), 42);

    REQUIRE(pos.x == 2500);
    REQUIRE(pos.y == -1000);
    REQUIRE(pos.z == 0);
    REQUIRE(pos.timestamp_ms == 42);
}

TEST_CASE("Position fromVec3 round-trip", "[foundation]") {
    glm::vec3 original(3.14f, 0.0f, -7.5f);
    auto pos = Position::fromVec3(original);
    auto recovered = pos.toVec3();

    // Fixed-point loses precision, but within tolerance
    REQUIRE(recovered.x == Approx(original.x).margin(0.002f));
    REQUIRE(recovered.y == Approx(original.y).margin(0.002f));
    REQUIRE(recovered.z == Approx(original.z).margin(0.002f));
}

TEST_CASE("Position distanceSqTo same point", "[foundation]") {
    Position a{};
    a.x = 1000;
    a.z = 2000;

    REQUIRE(a.distanceSqTo(a) == 0);
}

TEST_CASE("Position distanceSqTo horizontal distance", "[foundation]") {
    Position a{};
    Position b{};
    a.x = 0;
    a.z = 0;
    b.x = 3000;  // 3.0 units
    b.z = 4000;  // 4.0 units

    // distance = 5.0, distanceSq = 25.0
    // In fixed-point: (3000^2 + 4000^2) / 1000 = (9M + 16M) / 1000 = 25000
    auto distSq = a.distanceSqTo(b);
    REQUIRE(distSq == 25000);
}

TEST_CASE("Position distanceSqTo with negative deltas", "[foundation]") {
    Position a{};
    Position b{};
    a.x = -1000;
    a.z = -1000;
    b.x = 1000;
    b.z = 1000;

    // dx=2000, dz=2000, distSq = (4M + 4M) / 1000 = 8000
    auto distSq = a.distanceSqTo(b);
    REQUIRE(distSq == 8000);
}

// ============================================================================
// Velocity
// ============================================================================

TEST_CASE("Velocity default construction", "[foundation]") {
    Velocity v{};

    REQUIRE(v.dx == 0);
    REQUIRE(v.dy == 0);
    REQUIRE(v.dz == 0);
}

TEST_CASE("Velocity speed calculation", "[foundation]") {
    Velocity v{};
    v.dx = 3000;  // 3.0 units/s
    v.dz = 4000;  // 4.0 units/s

    REQUIRE(v.speed() == Approx(5.0f).margin(0.01f));
    REQUIRE(v.speedSq() == Approx(25.0f).margin(0.01f));
}

TEST_CASE("Velocity zero speed", "[foundation]") {
    Velocity v{};
    REQUIRE(v.speed() == Approx(0.0f).margin(0.001f));
    REQUIRE(v.speedSq() == Approx(0.0f).margin(0.001f));
}

// ============================================================================
// Rotation
// ============================================================================

TEST_CASE("Rotation default construction", "[foundation]") {
    Rotation r{};

    REQUIRE(r.yaw == 0.0f);
    REQUIRE(r.pitch == 0.0f);
}

// ============================================================================
// InputState
// ============================================================================

TEST_CASE("InputState default construction", "[foundation]") {
    InputState input{};

    REQUIRE(input.forward == 0);
    REQUIRE(input.backward == 0);
    REQUIRE(input.left == 0);
    REQUIRE(input.right == 0);
    REQUIRE(input.jump == 0);
    REQUIRE(input.attack == 0);
    REQUIRE(input.block == 0);
    REQUIRE(input.sprint == 0);
    REQUIRE(input.sequence == 0);
    REQUIRE(input.timestamp_ms == 0);
    REQUIRE(input.yaw == 0.0f);
    REQUIRE(input.pitch == 0.0f);
    REQUIRE_FALSE(input.hasMovementInput());
}

TEST_CASE("InputState hasMovementInput detects forward", "[foundation]") {
    InputState input{};
    input.forward = 1;
    REQUIRE(input.hasMovementInput());
}

TEST_CASE("InputState hasMovementInput detects left+right", "[foundation]") {
    InputState input{};
    input.left = 1;
    input.right = 1;
    REQUIRE(input.hasMovementInput());
}

TEST_CASE("InputState getInputDirection forward", "[foundation]") {
    InputState input{};
    input.forward = 1;

    auto dir = input.getInputDirection();
    // forward maps to -Z
    REQUIRE(dir.x == Approx(0.0f).margin(0.001f));
    REQUIRE(dir.z == Approx(-1.0f).margin(0.001f));
}

TEST_CASE("InputState getInputDirection diagonal", "[foundation]") {
    InputState input{};
    input.forward = 1;
    input.right = 1;

    auto dir = input.getInputDirection();
    float len = glm::length(dir);
    REQUIRE(len == Approx(1.0f).margin(0.01f));
}

TEST_CASE("InputState getInputDirection no input", "[foundation]") {
    InputState input{};
    auto dir = input.getInputDirection();
    REQUIRE(glm::length(dir) == Approx(0.0f).margin(0.001f));
}

// ============================================================================
// CombatState
// ============================================================================

TEST_CASE("CombatState default construction", "[foundation]") {
    CombatState cs{};

    REQUIRE(cs.health == 10000);
    REQUIRE(cs.maxHealth == 10000);
    REQUIRE(cs.teamId == 0);
    REQUIRE(cs.classType == 0);
    REQUIRE(cs.lastAttacker == entt::null);
    REQUIRE(cs.lastAttackTime == 0);
    REQUIRE_FALSE(cs.isDead);
}

TEST_CASE("CombatState healthPercent full health", "[foundation]") {
    CombatState cs{};
    REQUIRE(cs.healthPercent() == Approx(100.0f).margin(0.01f));
}

TEST_CASE("CombatState healthPercent half health", "[foundation]") {
    CombatState cs{};
    cs.health = 5000;
    REQUIRE(cs.healthPercent() == Approx(50.0f).margin(0.01f));
}

TEST_CASE("CombatState healthPercent zero health", "[foundation]") {
    CombatState cs{};
    cs.health = 0;
    REQUIRE(cs.healthPercent() == Approx(0.0f).margin(0.01f));
}

// ============================================================================
// BoundingVolume
// ============================================================================

TEST_CASE("BoundingVolume default construction", "[foundation]") {
    BoundingVolume bv{};
    REQUIRE(bv.radius == Approx(0.5f));
    REQUIRE(bv.height == Approx(1.8f));
}

TEST_CASE("BoundingVolume intersects same position", "[foundation]") {
    BoundingVolume bv{};
    Position a{};
    a.x = 1000;
    a.z = 2000;
    Position b = a;

    // Two entities at same position should intersect
    REQUIRE(bv.intersects(a, b));
}

TEST_CASE("BoundingVolume intersects nearby positions", "[foundation]") {
    BoundingVolume bv{};
    Position a{};
    Position b{};
    // Default radius 0.5, so minDist = 1.0 (1000 fixed-point units)
    // Place them close (< 1000 apart)
    a.x = 0;
    a.z = 0;
    b.x = 500;  // 0.5 units apart
    b.z = 0;

    REQUIRE(bv.intersects(a, b));
}

TEST_CASE("BoundingVolume no intersect far positions", "[foundation]") {
    BoundingVolume bv{};
    Position a{};
    Position b{};
    a.x = 0;
    a.z = 0;
    b.x = 5000;  // 5.0 units apart, well beyond 2*radius
    b.z = 0;

    REQUIRE_FALSE(bv.intersects(a, b));
}

// ============================================================================
// CollisionLayerMask
// ============================================================================

TEST_CASE("CollisionLayerMask values are correct", "[foundation]") {
    using namespace CollisionLayerMask;
    REQUIRE(NONE == 0);
    REQUIRE(PLAYER == 1);
    REQUIRE(NPC == 2);
    REQUIRE(PROJECTILE == 4);
    REQUIRE(STATIC == 8);
    REQUIRE(TRIGGER == 16);
}

TEST_CASE("CollisionLayerMask default masks include expected layers", "[foundation]") {
    using namespace CollisionLayerMask;
    REQUIRE((PLAYER_DEFAULT & PLAYER) != 0);
    REQUIRE((PLAYER_DEFAULT & NPC) != 0);
    REQUIRE((PLAYER_DEFAULT & STATIC) != 0);
    REQUIRE((PLAYER_DEFAULT & PROJECTILE) == 0);

    REQUIRE((NPC_DEFAULT & PLAYER) != 0);
    REQUIRE((NPC_DEFAULT & PROJECTILE) != 0);

    REQUIRE((PROJECTILE_DEFAULT & PLAYER) != 0);
    REQUIRE((PROJECTILE_DEFAULT & NPC) != 0);
}

// ============================================================================
// CollisionLayer
// ============================================================================

TEST_CASE("CollisionLayer makePlayer", "[foundation]") {
    auto layer = CollisionLayer::makePlayer();
    REQUIRE(layer.layer == CollisionLayerMask::PLAYER);
    REQUIRE(layer.collidesWith == CollisionLayerMask::PLAYER_DEFAULT);
    REQUIRE(layer.ownerEntity == entt::null);
}

TEST_CASE("CollisionLayer makeNPC", "[foundation]") {
    auto layer = CollisionLayer::makeNPC();
    REQUIRE(layer.layer == CollisionLayerMask::NPC);
    REQUIRE(layer.collidesWith == CollisionLayerMask::NPC_DEFAULT);
}

TEST_CASE("CollisionLayer makeProjectile", "[foundation]") {
    auto layer = CollisionLayer::makeProjectile();
    REQUIRE(layer.layer == CollisionLayerMask::PROJECTILE);
    REQUIRE(layer.collidesWith == CollisionLayerMask::PROJECTILE_DEFAULT);
}

TEST_CASE("CollisionLayer makeStatic", "[foundation]") {
    auto layer = CollisionLayer::makeStatic();
    REQUIRE(layer.layer == CollisionLayerMask::STATIC);
    REQUIRE(layer.collidesWith == CollisionLayerMask::STATIC_DEFAULT);
}

// ============================================================================
// EntityState
// ============================================================================

TEST_CASE("EntityState default construction", "[foundation]") {
    EntityState state{};

    REQUIRE(state.id == 0);
    REQUIRE(state.type == EntityType::PLAYER);
    REQUIRE(state.healthPercent == 100);
    REQUIRE(state.animState == AnimationState::IDLE);
    REQUIRE(state.teamId == 0);
}

TEST_CASE("EntityType enum values", "[foundation]") {
    REQUIRE(static_cast<uint8_t>(EntityType::PLAYER) == 0);
    REQUIRE(static_cast<uint8_t>(EntityType::PROJECTILE) == 1);
    REQUIRE(static_cast<uint8_t>(EntityType::LOOT) == 2);
    REQUIRE(static_cast<uint8_t>(EntityType::NPC) == 3);
}

TEST_CASE("AnimationState enum values", "[foundation]") {
    REQUIRE(static_cast<uint8_t>(AnimationState::IDLE) == 0);
    REQUIRE(static_cast<uint8_t>(AnimationState::WALK) == 1);
    REQUIRE(static_cast<uint8_t>(AnimationState::RUN) == 2);
    REQUIRE(static_cast<uint8_t>(AnimationState::ATTACK) == 3);
    REQUIRE(static_cast<uint8_t>(AnimationState::BLOCK) == 4);
    REQUIRE(static_cast<uint8_t>(AnimationState::DEAD) == 5);
}

// ============================================================================
// PlayerInfo
// ============================================================================

TEST_CASE("PlayerInfo default construction", "[foundation]") {
    PlayerInfo info{};

    REQUIRE(info.playerId == 0);
    REQUIRE(info.connectionId == 0);
    REQUIRE(info.sessionStart == 0);
    REQUIRE(info.username[0] == '\0');
}

// ============================================================================
// NetworkState
// ============================================================================

TEST_CASE("NetworkState default construction", "[foundation]") {
    NetworkState ns{};

    REQUIRE(ns.lastInputSequence == 0);
    REQUIRE(ns.lastInputTime == 0);
    REQUIRE(ns.rttMs == 0);
    REQUIRE(ns.packetLoss == Approx(0.0f));
    REQUIRE(ns.snapshotSequence == 0);
}

// ============================================================================
// SpatialCell
// ============================================================================

TEST_CASE("SpatialCell default construction", "[foundation]") {
    SpatialCell cell{};

    REQUIRE(cell.cellX == 0);
    REQUIRE(cell.cellZ == 0);
    REQUIRE(cell.zoneId == 0);
}

// ============================================================================
// Mana and Ability
// ============================================================================

TEST_CASE("Mana default construction", "[foundation]") {
    Mana mana{};

    REQUIRE(mana.current == Approx(100.0f));
    REQUIRE(mana.max == Approx(100.0f));
    REQUIRE(mana.regenerationRate == Approx(1.0f));
}

TEST_CASE("Ability default construction", "[foundation]") {
    Ability ab{};

    REQUIRE(ab.abilityId == 0);
    REQUIRE(ab.cooldownRemaining == Approx(0.0f));
    REQUIRE(ab.manaCost == Approx(0.0f));
}

TEST_CASE("Abilities default construction", "[foundation]") {
    Abilities abs{};

    REQUIRE(abs.count == 0);
    REQUIRE(abs.MAX_ABILITIES == 8);
}
