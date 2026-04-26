#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <glm/glm.hpp>
#include <cmath>

using namespace DarkAges;

namespace {
Position makePos(float x, float y, float z, uint32_t ts = 0) {
    Position p;
    p.x = static_cast<Constants::Fixed>(x * Constants::FLOAT_TO_FIXED);
    p.y = static_cast<Constants::Fixed>(y * Constants::FLOAT_TO_FIXED);
    p.z = static_cast<Constants::Fixed>(z * Constants::FLOAT_TO_FIXED);
    p.timestamp_ms = ts;
    return p;
}

Velocity makeVel(float dx, float dy, float dz) {
    Velocity v;
    v.dx = static_cast<Constants::Fixed>(dx * Constants::FLOAT_TO_FIXED);
    v.dy = static_cast<Constants::Fixed>(dy * Constants::FLOAT_TO_FIXED);
    v.dz = static_cast<Constants::Fixed>(dz * Constants::FLOAT_TO_FIXED);
    return v;
}
}  // namespace

// ============================================================================
// Position Tests
// ============================================================================

TEST_CASE("[core-types] Position default constructor", "[ecs]") {
    Position p;
    REQUIRE(p.x == Constants::Fixed(0));
    REQUIRE(p.y == Constants::Fixed(0));
    REQUIRE(p.z == Constants::Fixed(0));
    REQUIRE(p.timestamp_ms == 0);
}

TEST_CASE("[core-types] Position toVec3 conversion", "[ecs]") {
    Position p = makePos(10.0f, 5.0f, 20.0f);
    glm::vec3 v = p.toVec3();
    REQUIRE(v.x == Catch::Approx(10.0f).margin(0.01f));
    REQUIRE(v.y == Catch::Approx(5.0f).margin(0.01f));
    REQUIRE(v.z == Catch::Approx(20.0f).margin(0.01f));
}

TEST_CASE("[core-types] Position fromVec3 round-trip", "[ecs]") {
    glm::vec3 input(15.0f, 7.5f, 30.0f);
    Position p = Position::fromVec3(input, 1000);
    glm::vec3 result = p.toVec3();
    REQUIRE(result.x == Catch::Approx(15.0f).margin(0.1f));
    REQUIRE(result.y == Catch::Approx(7.5f).margin(0.1f));
    REQUIRE(result.z == Catch::Approx(30.0f).margin(0.1f));
    REQUIRE(p.timestamp_ms == 1000);
}

TEST_CASE("[core-types] Position distanceSqTo same position", "[ecs]") {
    Position p = makePos(10.0f, 0.0f, 20.0f);
    Position same = makePos(10.0f, 0.0f, 20.0f);
    REQUIRE(p.distanceSqTo(same) == Constants::Fixed(0));
}

TEST_CASE("[core-types] Position distanceSqTo horizontal", "[ecs]") {
    Position p = makePos(0.0f, 0.0f, 0.0f);
    Position other = makePos(3.0f, 0.0f, 4.0f);
    auto distSq = p.distanceSqTo(other);
    // 3^2 + 4^2 = 25, divided by FIXED_PRECISION
    REQUIRE(distSq > Constants::Fixed(0));
}

// ============================================================================
// Velocity Tests
// ============================================================================

TEST_CASE("[core-types] Velocity default constructor", "[ecs]") {
    Velocity v;
    REQUIRE(v.dx == Constants::Fixed(0));
    REQUIRE(v.dy == Constants::Fixed(0));
    REQUIRE(v.dz == Constants::Fixed(0));
}

TEST_CASE("[core-types] Velocity speed zero", "[ecs]") {
    Velocity v;
    REQUIRE(v.speed() == Catch::Approx(0.0f).margin(0.001f));
}

TEST_CASE("[core-types] Velocity speed calculation", "[ecs]") {
    Velocity v = makeVel(3.0f, 0.0f, 4.0f);
    // 3-4-5 triangle, speed should be 5
    REQUIRE(v.speed() == Catch::Approx(5.0f).margin(0.01f));
}

TEST_CASE("[core-types] Velocity speedSq calculation", "[ecs]") {
    Velocity v = makeVel(3.0f, 0.0f, 4.0f);
    REQUIRE(v.speedSq() == Catch::Approx(25.0f).margin(0.01f));
}

// ============================================================================
// Rotation Tests
// ============================================================================

TEST_CASE("[core-types] Rotation default constructor", "[ecs]") {
    Rotation r;
    REQUIRE(r.yaw == 0.0f);
    REQUIRE(r.pitch == 0.0f);
}

TEST_CASE("[core-types] Rotation with values", "[ecs]") {
    Rotation r{1.5f, 0.3f};
    REQUIRE(r.yaw == 1.5f);
    REQUIRE(r.pitch == 0.3f);
}

// ============================================================================
// InputState Tests
// ============================================================================

TEST_CASE("[core-types] InputState default constructor", "[ecs]") {
    InputState input;
    REQUIRE(input.forward == 0);
    REQUIRE(input.backward == 0);
    REQUIRE(input.left == 0);
    REQUIRE(input.right == 0);
    REQUIRE(input.jump == 0);
    REQUIRE(input.attack == 0);
    REQUIRE(input.block == 0);
    REQUIRE(input.sprint == 0);
    REQUIRE(input.sequence == 0);
}

TEST_CASE("[core-types] InputState hasMovementInput no input", "[ecs]") {
    InputState input;
    REQUIRE(input.hasMovementInput() == false);
}

TEST_CASE("[core-types] InputState hasMovementInput forward", "[ecs]") {
    InputState input;
    input.forward = 1;
    REQUIRE(input.hasMovementInput() == true);
}

TEST_CASE("[core-types] InputState hasMovementInput diagonal", "[ecs]") {
    InputState input;
    input.forward = 1;
    input.right = 1;
    REQUIRE(input.hasMovementInput() == true);
}

TEST_CASE("[core-types] InputState getInputDirection no input", "[ecs]") {
    InputState input;
    glm::vec3 dir = input.getInputDirection();
    REQUIRE(dir == glm::vec3(0.0f));
}

TEST_CASE("[core-types] InputState getInputDirection forward", "[ecs]") {
    InputState input;
    input.forward = 1;
    glm::vec3 dir = input.getInputDirection();
    REQUIRE(dir.z < 0.0f);  // Forward is -Z
}

// ============================================================================
// CombatState Tests
// ============================================================================

TEST_CASE("[core-types] CombatState default constructor", "[ecs]") {
    CombatState cs;
    REQUIRE(cs.health == 10000);
    REQUIRE(cs.maxHealth == 10000);
    REQUIRE(cs.teamId == 0);
    REQUIRE(cs.classType == 0);
    REQUIRE(cs.isDead == false);
}

TEST_CASE("[core-types] CombatState healthPercent full", "[ecs]") {
    CombatState cs;
    cs.health = 10000;
    cs.maxHealth = 10000;
    REQUIRE(cs.healthPercent() == Catch::Approx(100.0f).margin(0.01f));
}

TEST_CASE("[core-types] CombatState healthPercent half", "[ecs]") {
    CombatState cs;
    cs.health = 5000;
    cs.maxHealth = 10000;
    REQUIRE(cs.healthPercent() == Catch::Approx(50.0f).margin(0.01f));
}

TEST_CASE("[core-types] CombatState healthPercent zero", "[ecs]") {
    CombatState cs;
    cs.health = 0;
    cs.maxHealth = 10000;
    REQUIRE(cs.healthPercent() == Catch::Approx(0.0f).margin(0.01f));
}

// ============================================================================
// SpatialCell Tests
// ============================================================================

TEST_CASE("[core-types] SpatialCell default constructor", "[ecs]") {
    SpatialCell cell;
    REQUIRE(cell.cellX == 0);
    REQUIRE(cell.cellZ == 0);
    REQUIRE(cell.zoneId == 0);
}

TEST_CASE("[core-types] SpatialCell with values", "[ecs]") {
    SpatialCell cell{10, -5, 3};
    REQUIRE(cell.cellX == 10);
    REQUIRE(cell.cellZ == -5);
    REQUIRE(cell.zoneId == 3);
}

// ============================================================================
// BoundingVolume Tests
// ============================================================================

TEST_CASE("[core-types] BoundingVolume default constructor", "[ecs]") {
    BoundingVolume bv;
    REQUIRE(bv.radius == Catch::Approx(0.5f));
    REQUIRE(bv.height == Catch::Approx(1.8f));
}

TEST_CASE("[core-types] BoundingVolume intersects same position", "[ecs]") {
    BoundingVolume bv;
    Position p = makePos(10.0f, 0.0f, 20.0f);
    REQUIRE(bv.intersects(p, p) == true);
}

TEST_CASE("[core-types] BoundingVolume intersects far apart", "[ecs]") {
    BoundingVolume bv;
    Position p1 = makePos(0.0f, 0.0f, 0.0f);
    Position p2 = makePos(100.0f, 0.0f, 100.0f);
    REQUIRE(bv.intersects(p1, p2) == false);
}

// ============================================================================
// CollisionLayerMask Tests
// ============================================================================

TEST_CASE("[core-types] CollisionLayerMask values", "[ecs]") {
    REQUIRE(CollisionLayerMask::NONE == 0);
    REQUIRE(CollisionLayerMask::PLAYER == 1);
    REQUIRE(CollisionLayerMask::NPC == 2);
    REQUIRE(CollisionLayerMask::HITBOX == 4);
    REQUIRE(CollisionLayerMask::HURTBOX == 8);
    REQUIRE(CollisionLayerMask::PROJECTILE == 16);
    REQUIRE(CollisionLayerMask::STATIC == 32);
    REQUIRE(CollisionLayerMask::TRIGGER == 64);
}

TEST_CASE("[core-types] CollisionLayerMask default masks", "[ecs]") {
    // Updated: added HITBOX (4) and HURTBOX (8); STATIC moved to bit5 (32)
    REQUIRE(CollisionLayerMask::PLAYER_DEFAULT == (1 | 2 | 4 | 8 | 32));  // PLAYER | NPC | HITBOX | HURTBOX | STATIC
    REQUIRE(CollisionLayerMask::NPC_DEFAULT == (1 | 2 | 4 | 8 | 16 | 32));  // PLAYER | NPC | HITBOX | HURTBOX | PROJECTILE | STATIC
}

// ============================================================================
// CollisionLayer Tests
// ============================================================================

TEST_CASE("[core-types] CollisionLayer makePlayer", "[ecs]") {
    CollisionLayer layer = CollisionLayer::makePlayer();
    REQUIRE(layer.layer == CollisionLayerMask::PLAYER);
    REQUIRE(layer.collidesWith == CollisionLayerMask::PLAYER_DEFAULT);
    REQUIRE(static_cast<bool>(layer.ownerEntity == entt::null));
}

TEST_CASE("[core-types] CollisionLayer makeNPC", "[ecs]") {
    CollisionLayer layer = CollisionLayer::makeNPC();
    REQUIRE(layer.layer == CollisionLayerMask::NPC);
    REQUIRE(layer.collidesWith == CollisionLayerMask::NPC_DEFAULT);
}

TEST_CASE("[core-types] CollisionLayer makeProjectile", "[ecs]") {
    CollisionLayer layer = CollisionLayer::makeProjectile();
    REQUIRE(layer.layer == CollisionLayerMask::PROJECTILE);
    REQUIRE(layer.collidesWith == CollisionLayerMask::PROJECTILE_DEFAULT);
}

TEST_CASE("[core-types] CollisionLayer makeStatic", "[ecs]") {
    CollisionLayer layer = CollisionLayer::makeStatic();
    REQUIRE(layer.layer == CollisionLayerMask::STATIC);
    REQUIRE(layer.collidesWith == CollisionLayerMask::STATIC_DEFAULT);
}

// ============================================================================
// EntityState Tests
// ============================================================================

TEST_CASE("[core-types] EntityState default constructor", "[ecs]") {
    EntityState es;
    REQUIRE(es.id == 0);
    REQUIRE(es.type == EntityType::PLAYER);
    REQUIRE(es.healthPercent == 100);
    REQUIRE(es.animState == AnimationState::IDLE);
    REQUIRE(es.teamId == 0);
}

TEST_CASE("[core-types] EntityState all entity types", "[ecs]") {
    REQUIRE(static_cast<uint8_t>(EntityType::PLAYER) == 0);
    REQUIRE(static_cast<uint8_t>(EntityType::PROJECTILE) == 1);
    REQUIRE(static_cast<uint8_t>(EntityType::LOOT) == 2);
    REQUIRE(static_cast<uint8_t>(EntityType::NPC) == 3);
}

TEST_CASE("[core-types] EntityState all animation states", "[ecs]") {
    REQUIRE(static_cast<uint8_t>(AnimationState::IDLE) == 0);
    REQUIRE(static_cast<uint8_t>(AnimationState::WALK) == 1);
    REQUIRE(static_cast<uint8_t>(AnimationState::RUN) == 2);
    REQUIRE(static_cast<uint8_t>(AnimationState::ATTACK) == 3);
    REQUIRE(static_cast<uint8_t>(AnimationState::BLOCK) == 4);
    REQUIRE(static_cast<uint8_t>(AnimationState::DEAD) == 5);
}

// ============================================================================
// PlayerInfo Tests
// ============================================================================

TEST_CASE("[core-types] PlayerInfo default constructor", "[ecs]") {
    PlayerInfo info;
    REQUIRE(info.playerId == 0);
    REQUIRE(info.connectionId == 0);
    REQUIRE(info.sessionStart == 0);
}

TEST_CASE("[core-types] PlayerInfo with values", "[ecs]") {
    PlayerInfo info;
    info.playerId = 12345;
    info.connectionId = 99;
    info.sessionStart = 1000000;
    REQUIRE(info.playerId == 12345);
    REQUIRE(info.connectionId == 99);
}

// ============================================================================
// NetworkState Tests
// ============================================================================

TEST_CASE("[core-types] NetworkState default constructor", "[ecs]") {
    NetworkState ns;
    REQUIRE(ns.lastInputSequence == 0);
    REQUIRE(ns.lastInputTime == 0);
    REQUIRE(ns.rttMs == 0);
    REQUIRE(ns.packetLoss == 0.0f);
}

// ============================================================================
// Mana Tests
// ============================================================================

TEST_CASE("[core-types] Mana default constructor", "[ecs]") {
    Mana mana;
    REQUIRE(mana.current == Catch::Approx(100.0f));
    REQUIRE(mana.max == Catch::Approx(100.0f));
    REQUIRE(mana.regenerationRate == Catch::Approx(1.0f));
}

TEST_CASE("[core-types] Mana with values", "[ecs]") {
    Mana mana;
    mana.current = 50.0f;
    mana.max = 200.0f;
    REQUIRE(mana.current == Catch::Approx(50.0f));
    REQUIRE(mana.max == Catch::Approx(200.0f));
}

// ============================================================================
// Ability Tests
// ============================================================================

TEST_CASE("[core-types] Ability default constructor", "[ecs]") {
    Ability ability;
    REQUIRE(ability.abilityId == 0);
    REQUIRE(ability.cooldownRemaining == Catch::Approx(0.0f));
    REQUIRE(ability.manaCost == Catch::Approx(0.0f));
}

TEST_CASE("[core-types] Ability with values", "[ecs]") {
    Ability ability;
    ability.abilityId = 5;
    ability.cooldownRemaining = 10.0f;
    ability.manaCost = 25.0f;
    REQUIRE(ability.abilityId == 5);
    REQUIRE(ability.cooldownRemaining == Catch::Approx(10.0f));
}

TEST_CASE("[core-types] Abilities max count", "[ecs]") {
    Abilities abilities;
    REQUIRE(abilities.count == 0);
    REQUIRE(Abilities::MAX_ABILITIES == 8);
}