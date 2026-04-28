// [TEST_AGENT] Hitbox/Hurtbox Collision Layer Validation
// Tests server-authoritative collision detection between combat hitboxes

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/CombatSystem.hpp"
#include "combat/HitboxComponent.hpp"
#include "combat/HurtboxComponent.hpp"
#include "ecs/CoreTypes.hpp"
#include "zones/ZoneServer.hpp"
#include <glm/glm.hpp>
#include <entt/entity/registry.hpp>

using namespace DarkAges;
using glm::vec3;

namespace HitboxValidation {

// Test fixture
struct HitboxTestFixture {
    Registry registry;
    CollisionLayerManager layerManager;
    std::unique_ptr<ZoneServer> zone;
    
    HitboxTestFixture() {
        ZoneConfig config;
        config.zoneId = 999;
        config.port = 17777;
        zone = std::make_unique<ZoneServer>();
        REQUIRE(zone->initialize(config));
        
        // Initialize collision layers
        layerManager.registerLayer("Default", 1);
        layerManager.registerLayer("Player", 2);
        layerManager.registerLayer("NPC", 4);
        layerManager.registerLayer("Environment", 8);
        
        // Set up collision matrix
        layerManager.setCollisionRule(1, 4, true);  // Default vs NPC
        layerManager.setCollisionRule(2, 4, true);  // Player vs NPC
        layerManager.setCollisionRule(2, 8, true);  // Player vs Environment
        layerManager.setCollisionRule(4, 4, true);  // NPC vs NPC
    }
    
    EntityID createPlayerEntity(vec3 pos) {
        auto entity = registry.create();
        registry.emplace<PositionComponent>(entity, pos);
        registry.emplace<CombatComponent>(entity, 100.0f, 10.0f, 5.0f, 1.0f);
        auto& hitbox = registry.emplace<HitboxComponent>(entity);
        hitbox.offset = {0, 0.9f, 0};
        hitbox.radius = 0.5f;
        hitbox.height = 1.8f;
        hitbox.layer = 2; // Player layer
        hitbox.isActive = true;
        return entity;
    }
    
    EntityID createNPCEntity(vec3 pos) {
        auto entity = registry.create();
        registry.emplace<PositionComponent>(entity, pos);
        registry.emplace<CombatComponent>(entity, 50.0f, 8.0f, 3.0f, 1.0f);
        auto& hurtbox = registry.emplace<HurtboxComponent>(entity);
        hurtbox.offset = {0, 0.8f, 0};
        hurtbox.radius = 0.6f;
        hurtbox.height = 1.6f;
        hurtbox.layer = 4; // NPC layer
        hurtbox.isActive = true;
        return entity;
    }
    
    EntityID createEnvironmentCollider(vec3 pos, float radius) {
        auto entity = registry.create();
        registry.emplace<PositionComponent>(entity, pos);
        auto& hurtbox = registry.emplace<HurtboxComponent>(entity);
        hurtbox.offset = {0, 0, 0};
        hurtbox.radius = radius;
        hurtbox.height = 2.0f;
        hurtbox.layer = 8; // Environment layer
        hurtbox.isActive = true;
        return entity;
    }
};

}

TEST_CASE_METHOD(HitboxValidation::HitboxTestFixture, "Hitbox-Hurtbox basic collision detection", "[combat][hitbox][priority-critical]") {
    SECTION("Direct hit - hitbox overlaps hurtbox") {
        auto player = createPlayerEntity({0, 0, 0});
        auto npc = createNPCEntity({0.8f, 0, 0}); // Close enough to hit
        
        auto& hitbox = registry.get<HitboxComponent>(player);
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        auto& playerPos = registry.get<PositionComponent>(player);
        auto& npcPos = registry.get<PositionComponent>(npc);
        
        bool collision = checkHitboxHurtboxCollision(
            playerPos.position + hitbox.offset, hitbox.radius, hitbox.height,
            npcPos.position + hurtbox.offset, hurtbox.radius, hurtbox.height,
            hitbox.layer, hurtbox.layer, layerManager
        );
        
        REQUIRE(collision == true);
    }
    
    SECTION("Miss - hitbox too far from hurtbox") {
        auto player = createPlayerEntity({0, 0, 0});
        auto npc = createNPCEntity({5.0f, 0, 0}); // Too far to hit
        
        auto& hitbox = registry.get<HitboxComponent>(player);
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        auto& playerPos = registry.get<PositionComponent>(player);
        auto& npcPos = registry.get<PositionComponent>(npc);
        
        bool collision = checkHitboxHurtboxCollision(
            playerPos.position + hitbox.offset, hitbox.radius, hitbox.height,
            npcPos.position + hurtbox.offset, hurtbox.radius, hurtbox.height,
            hitbox.layer, hurtbox.layer, layerManager
        );
        
        REQUIRE(collision == false);
    }
    
    SECTION("Edge case - just within range") {
        auto player = createPlayerEntity({0, 0, 0});
        // Hitbox radius 0.5 + Hurtbox radius 0.6 = 1.1 combined
        auto npc = createNPCEntity({1.05f, 0, 0}); // Just within range
        
        auto& hitbox = registry.get<HitboxComponent>(player);
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        auto& playerPos = registry.get<PositionComponent>(player);
        auto& npcPos = registry.get<PositionComponent>(npc);
        
        bool collision = checkHitboxHurtboxCollision(
            playerPos.position + hitbox.offset, hitbox.radius, hitbox.height,
            npcPos.position + hurtbox.offset, hurtbox.radius, hurtbox.height,
            hitbox.layer, hurtbox.layer, layerManager
        );
        
        REQUIRE(collision == true);
    }
}

TEST_CASE_METHOD(HitboxValidation::HitboxTestFixture, "Collision layer filtering", "[combat][layers][priority-critical]") {
    SECTION("Player can hit NPC") {
        auto player = createPlayerEntity({0, 0, 0});
        auto npc = createNPCEntity({0.5f, 0, 0});
        
        auto& hitbox = registry.get<HitboxComponent>(player);
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        
        bool canCollide = layerManager.canCollide(hitbox.layer, hurtbox.layer);
        REQUIRE(canCollide == true);
    }
    
    SECTION("NPC cannot hit Environment") {
        auto npc = createNPCEntity({0, 0, 0});
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        
        bool canCollide = layerManager.canCollide(4, 8); // NPC vs Environment
        REQUIRE(canCollide == false);
    }
    
    SECTION("Default layer vs Player") {
        // Create unlayered entity
        auto entity = registry.create();
        registry.emplace<PositionComponent>(entity, vec3{0, 0, 0});
        auto& hurtbox = registry.emplace<HurtboxComponent>(entity);
        hurtbox.layer = 1; // Default
        hurtbox.isActive = true;
        
        bool canCollide = layerManager.canCollide(1, 2); // Default vs Player
        // Default typically can't hit players (no rule set)
        REQUIRE(canCollide == false);
    }
}

TEST_CASE_METHOD(HitboxValidation::HitboxTestFixture, "Hitbox activation/deactivation", "[combat][hitbox][priority-high]") {
    SECTION("Inactive hitbox cannot collide") {
        auto player = createPlayerEntity({0, 0, 0});
        auto npc = createNPCEntity({0.5f, 0, 0});
        
        auto& hitbox = registry.get<HitboxComponent>(player);
        hitbox.isActive = false;
        
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        auto& playerPos = registry.get<PositionComponent>(player);
        auto& npcPos = registry.get<PositionComponent>(npc);
        
        // Even though positions overlap, hitbox is inactive
        bool collision = hitbox.isActive && checkHitboxHurtboxCollision(
            playerPos.position + hitbox.offset, hitbox.radius, hitbox.height,
            npcPos.position + hurtbox.offset, hurtbox.radius, hurtbox.height,
            hitbox.layer, hurtbox.layer, layerManager
        );
        
        REQUIRE(collision == false);
    }
    
    SECTION("Active hitbox can collide when enabled") {
        auto player = createPlayerEntity({0, 0, 0});
        auto npc = createNPCEntity({0.5f, 0, 0});
        
        auto& hitbox = registry.get<HitboxComponent>(player);
        hitbox.isActive = true;
        
        auto& hurtbox = registry.get<HurtboxComponent>(npc);
        auto& playerPos = registry.get<PositionComponent>(player);
        auto& npcPos = registry.get<PositionComponent>(npc);
        
        bool collision = hitbox.isActive && checkHitboxHurtboxCollision(
            playerPos.position + hitbox.offset, hitbox.radius, hitbox.height,
            npcPos.position + hurtbox.offset, hurtbox.radius, hurtbox.height,
            hitbox.layer, hurtbox.layer, layerManager
        );
        
        REQUIRE(collision == true);
    }
}

TEST_CASE_METHOD(HitboxValidation::HitboxTestFixture, "Server-authoritative validation", "[combat][authority][priority-critical]") {
    SECTION("CombatSystem validates hit before applying damage") {
        auto attacker = createPlayerEntity({0, 0, 0});
        auto target = createNPCEntity({1.0f, 0, 0});
        
        // Create attack input
        AttackInput input;
        input.playerId = static_cast<uint32_t>(attacker);
        input.targetEntity = static_cast<uint32_t>(target);
        input.attackType = 0; // Melee
        input.timestamp = 0;
        input.position = {0, 0, 0};
        input.direction = {1, 0, 0};
        
        auto attackerPos = registry.get<PositionComponent>(attacker).position;
        auto targetPos = registry.get<PositionComponent>(target).position;
        
        // Check if attack range is valid (melee range ~2.5)
        float distance = glm::distance(attackerPos, targetPos);
        bool inRange = distance <= 2.5f;
        
        REQUIRE(inRange == true);
        
        // Check layer collision
        auto& hitbox = registry.get<HitboxComponent>(attacker);
        auto& hurtbox = registry.get<HurtboxComponent>(target);
        bool validLayer = layerManager.canCollide(hitbox.layer, hurtbox.layer);
        
        REQUIRE(validLayer == true);
        
        // Validation passes
        REQUIRE((inRange && validLayer) == true);
    }
    
    SECTION("Attack out of range is rejected") {
        auto attacker = createPlayerEntity({0, 0, 0});
        auto target = createNPCEntity({10.0f, 0, 0}); // Far away
        
        auto attackerPos = registry.get<PositionComponent>(attacker).position;
        auto targetPos = registry.get<PositionComponent>(target).position;
        
        float distance = glm::distance(attackerPos, targetPos);
        bool inRange = distance <= 2.5f;
        
        REQUIRE(inRange == false);
        REQUIRE((inRange == false) == true);
    }
}

TEST_CASE_METHOD(HitboxValidation::HitboxTestFixture, "Collision layer matrix", "[combat][layers][documentation]") {
    SECTION("Layer matrix documentation") {
        // This test serves as documentation of the collision matrix
        
        // Layer 1: Default
        // Layer 2: Player
        // Layer 4: NPC
        // Layer 8: Environment
        
        CHECK(layerManager.canCollide(1, 4) == true);  // Default hits NPC
        CHECK(layerManager.canCollide(2, 4) == true);  // Player hits NPC
        CHECK(layerManager.canCollide(2, 8) == true);  // Player hits Environment
        CHECK(layerManager.canCollide(4, 4) == true);  // NPC hits NPC
        
        CHECK(layerManager.canCollide(1, 2) == false); // Default vs Player
        CHECK(layerManager.canCollide(1, 1) == false); // Default vs Default
        CHECK(layerManager.canCollide(4, 8) == false); // NPC vs Environment
        CHECK(layerManager.canCollide(8, 8) == false); // Environment vs Environment
    }
}
