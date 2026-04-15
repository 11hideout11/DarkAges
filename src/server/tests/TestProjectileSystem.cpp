// [COMBAT_AGENT] Unit tests for ranged/projectile combat system

#include <catch2/catch_test_macros.hpp>
#include "combat/CombatSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;

TEST_CASE("ProjectileSystem ranged attack misses when no targets", "[Projectile]") {
    Registry registry;
    CombatSystem combat;
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == false);
    REQUIRE(result.hitType == std::string("miss"));
}

TEST_CASE("ProjectileSystem ranged attack hits target in front", "[Projectile]") {
    Registry registry;
    CombatSystem combat;
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    EntityID target = registry.create();
    CombatState& targetCombat = registry.emplace<CombatState>(target);
    targetCombat.health = 10000;
    targetCombat.teamId = 2;
    registry.emplace<Position>(target, Position::fromVec3(glm::vec3(0, 0, 10.0f), 0));  // 10m in front
    registry.emplace<Rotation>(target);
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == true);
    REQUIRE(result.target == target);
    REQUIRE(result.damageDealt > 0);
    REQUIRE(result.hitType == std::string("ranged"));
    REQUIRE(targetCombat.health < 10000);
}

TEST_CASE("ProjectileSystem ranged attack misses target behind attacker", "[Projectile]") {
    Registry registry;
    CombatSystem combat;
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    EntityID target = registry.create();
    CombatState& targetCombat = registry.emplace<CombatState>(target);
    targetCombat.health = 10000;
    targetCombat.teamId = 2;
    registry.emplace<Position>(target, Position::fromVec3(glm::vec3(0, 0, -10.0f), 0));  // 10m behind
    registry.emplace<Rotation>(target);
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == false);
    REQUIRE(result.hitType == std::string("miss"));
}

TEST_CASE("ProjectileSystem ranged attack hits closest target", "[Projectile]") {
    Registry registry;
    CombatSystem combat;
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    EntityID farTarget = registry.create();
    CombatState& farCombat = registry.emplace<CombatState>(farTarget);
    farCombat.health = 10000;
    farCombat.teamId = 2;
    registry.emplace<Position>(farTarget, Position::fromVec3(glm::vec3(0, 0, 40.0f), 0));  // 40m in front
    registry.emplace<Rotation>(farTarget);
    
    EntityID closeTarget = registry.create();
    CombatState& closeCombat = registry.emplace<CombatState>(closeTarget);
    closeCombat.health = 10000;
    closeCombat.teamId = 2;
    registry.emplace<Position>(closeTarget, Position::fromVec3(glm::vec3(0, 0, 5.0f), 0));  // 5m in front
    registry.emplace<Rotation>(closeTarget);
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == true);
    REQUIRE(result.target == closeTarget);
}

TEST_CASE("ProjectileSystem ranged attack misses dead targets", "[Projectile]") {
    Registry registry;
    CombatSystem combat;
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    EntityID target = registry.create();
    CombatState& targetCombat = registry.emplace<CombatState>(target);
    targetCombat.health = 0;
    targetCombat.isDead = true;
    targetCombat.teamId = 2;
    registry.emplace<Position>(target, Position::fromVec3(glm::vec3(0, 0, 10.0f), 0));
    registry.emplace<Rotation>(target);
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == false);
    REQUIRE(result.hitType == std::string("miss"));
}

TEST_CASE("ProjectileSystem ranged attack respects team damage rules", "[Projectile]") {
    Registry registry;
    CombatConfig config;
    config.allowFriendlyFire = false;
    CombatSystem combat(config);
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    attackerCombat.teamId = 1;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    EntityID friendlyTarget = registry.create();
    CombatState& friendlyCombat = registry.emplace<CombatState>(friendlyTarget);
    friendlyCombat.health = 10000;
    friendlyCombat.teamId = 1;
    registry.emplace<Position>(friendlyTarget, Position::fromVec3(glm::vec3(0, 0, 10.0f), 0));
    registry.emplace<Rotation>(friendlyTarget);
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == false);
    REQUIRE(result.hitType == std::string("miss"));
}

TEST_CASE("ProjectileSystem ranged attack ignores targets beyond 50m", "[Projectile]") {
    Registry registry;
    CombatSystem combat;
    
    EntityID attacker = registry.create();
    CombatState& attackerCombat = registry.emplace<CombatState>(attacker);
    attackerCombat.health = 10000;
    registry.emplace<Position>(attacker, Position::fromVec3(glm::vec3(0, 0, 0), 0));
    registry.emplace<Rotation>(attacker, Rotation{0.0f, 0.0f});  // Facing +Z
    
    EntityID target = registry.create();
    CombatState& targetCombat = registry.emplace<CombatState>(target);
    targetCombat.health = 10000;
    targetCombat.teamId = 2;
    registry.emplace<Position>(target, Position::fromVec3(glm::vec3(0, 0, 60.0f), 0));  // 60m in front
    registry.emplace<Rotation>(target);
    
    HitResult result = combat.performRangedAttack(registry, attacker, glm::vec3(0, 0, 1), 1000);
    
    REQUIRE(result.hit == false);
    REQUIRE(result.hitType == std::string("miss"));
}
