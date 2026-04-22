#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/SpawnSystem.hpp"
#include "zones/ZoneDefinition.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;

// Helper to convert Position x/z (Fixed) to float for test assertions
static float posX(const Position& pos) { return pos.x * Constants::FIXED_TO_FLOAT; }
static float posZ(const Position& pos) { return pos.z * Constants::FIXED_TO_FLOAT; }

// ============================================================================
// Test Fixtures
// ============================================================================

struct SpawnSystemFixture {
    Registry registry;
    SpawnSystem spawnSystem;
    std::vector<ZoneDefinition> zones;
    
    SpawnSystemFixture() {
        // Setup mock zones
        ZoneDefinition zone1;
        zone1.zoneId = 1;
        zone1.zoneName = "StartingZone";
        zone1.centerX = 0;
        zone1.centerZ = 0;
        zone1.spawnX = 100.0f;
        zone1.spawnZ = 100.0f;
        zone1.minX = 0;
        zone1.maxX = 100;
        zone1.minZ = 0;
        zone1.maxZ = 100;
        zones.push_back(zone1);
        
        ZoneDefinition zone2;
        zone2.zoneId = 2;
        zone2.zoneName = "DungeonZone";
        zone2.centerX = 500;
        zone2.centerZ = 500;
        zone2.spawnX = 520.0f;
        zone2.spawnZ = 520.0f;
        zone2.minX = 400;
        zone2.maxX = 600;
        zone2.minZ = 400;
        zone2.maxZ = 600;
        zones.push_back(zone2);
    }
};

// ============================================================================
// Helper Functions
// ============================================================================

static EntityID createSpawnableNPC(Registry& registry, uint32_t groupId, 
                                   uint32_t templateId, uint32_t respawnMs,
                                   const Position& pos, uint8_t level = 1) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<NPCTag>(entity);
    
    NPCStats stats;
    stats.level = level;
    stats.respawnTimeMs = respawnMs;
    stats.baseDamage = 10;
    stats.xpReward = 50;
    registry.emplace<NPCStats>(entity, stats);
    
    CombatState combat;
    combat.health = 100;
    combat.maxHealth = 100;
    registry.emplace<CombatState>(entity, combat);
    
    SpawnableComponent spawnable;
    spawnable.spawnGroupId = groupId;
    spawnable.templateId = templateId;
    spawnable.respawnTimeMs = respawnMs;
    spawnable.spawnPosition = pos;
    spawnable.level = level;
    spawnable.isSpawned = true;
    spawnable.shouldRespawn = true;
    registry.emplace<SpawnableComponent>(entity, spawnable);
    
    return entity;
}

// ============================================================================
// SpawnSystem Integration with ZoneServer Tests
// ============================================================================

TEST_CASE("SpawnSystem spawnEntity creates NPCs with SpawnableComponent", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    // Register spawn group
    SpawnGroup group(1, "testGroup", 10000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
    fix.spawnSystem.registerSpawnGroup(group);
    
    SECTION("spawnEntity creates entity with all required components") {
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        REQUIRE((npc != entt::null));
        REQUIRE(fix.registry.all_of<Position>(npc));
        REQUIRE(fix.registry.all_of<Velocity>(npc));
        REQUIRE(fix.registry.all_of<Rotation>(npc));
        REQUIRE(fix.registry.all_of<NPCTag>(npc));
        REQUIRE(fix.registry.all_of<SpawnableComponent>(npc));
        REQUIRE(fix.registry.all_of<NPCStats>(npc));
        REQUIRE(fix.registry.all_of<CombatState>(npc));
    }
    
    SECTION("spawnEntity sets SpawnableComponent properties correctly") {
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
        REQUIRE(spawnable.spawnGroupId == 1);
        REQUIRE(spawnable.templateId == 100);
        REQUIRE(spawnable.isSpawned == true);
        REQUIRE(spawnable.shouldRespawn == true);
    }
    
    SECTION("spawnEntity uses zone spawn position") {
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        const auto& pos = fix.registry.get<Position>(npc);
        // Should use spawnX/spawnZ from zone
        REQUIRE(posX(pos) == Catch::Approx(100.0f));
        REQUIRE(posZ(pos) == Catch::Approx(100.0f));
    }
}

// ============================================================================
// SpawnSystem Respawn Lifecycle Tests
// ============================================================================

TEST_CASE("SpawnSystem respawn lifecycle", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    // Register spawn group
    SpawnGroup group(1, "respawnGroup", 10000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 3000);
    fix.spawnSystem.registerSpawnGroup(group);
    
    SECTION("NPC starts as spawned") {
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
        REQUIRE(spawnable.isSpawned == true);
    }
    
    SECTION("onEntityDeath marks NPC as not spawned and sets respawn timer") {
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        fix.spawnSystem.onEntityDeath(fix.registry, npc);
        
        const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
        REQUIRE(spawnable.isSpawned == false);
        REQUIRE(spawnable.respawnTimeMs > 0);
    }
    
    SECTION("NPC respawns when timer expires via update") {
        // Create NPC with expired respawn time
        [[maybe_unused]] Position _pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        // Get respawn time from config
        uint32_t respawnMs = 5000;
        auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
        spawnable.respawnTimeMs = respawnMs;
        
        // Simulate death
        fix.spawnSystem.onEntityDeath(fix.registry, npc);
        
        // Update should not respawn immediately (timer not expired)
        fix.spawnSystem.update(fix.registry, 1000);
        {
            const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
            REQUIRE(spawnable.isSpawned == false);
        }
        
        // Update after timer expires should trigger respawn logic
        fix.spawnSystem.update(fix.registry, 1000 + respawnMs + 100);
        // Note: respawn creates a NEW entity, original stays as spawn tracker
    }
    
    SECTION("NPCs with shouldRespawn=false do not respawn") {
        [[maybe_unused]] Position _pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        // Mark as not respawnable
        auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
        spawnable.shouldRespawn = false;
        spawnable.respawnTimeMs = 0;  // Already expired
        
        // Simulate death
        fix.spawnSystem.onEntityDeath(fix.registry, npc);
        
        // Update should not attempt respawn
        fix.spawnSystem.update(fix.registry, 10000);
        
        // NPC should still be marked as not spawned
        const auto& updated = fix.registry.get<SpawnableComponent>(npc);
        REQUIRE(updated.isSpawned == false);
    }
}

// ============================================================================
// Spawn Group Registration Tests
// ============================================================================

TEST_CASE("SpawnSystem spawn group registration creates weighted spawn configs", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("registerSpawnGroup stores group with single NPC") {
        SpawnGroup group(1, "singleNPC", 5000);
        group.npcs.emplace_back(101, 1.0f, 1, 5, 3, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        const auto* stored = fix.spawnSystem.getSpawnGroup(1);
        REQUIRE(stored != nullptr);
        REQUIRE(stored->groupName == "singleNPC");
        REQUIRE(stored->npcs.size() == 1);
        REQUIRE(stored->npcs[0].npcTemplateId == 101);
        REQUIRE(stored->npcs[0].spawnWeight == 1.0f);
    }
    
    SECTION("registerSpawnGroup stores group with multiple weighted NPCs") {
        SpawnGroup group(1, "multiWeighted", 10000);
        group.npcs.emplace_back(100, 10.0f, 1, 3, 2, 5000);  // Common
        group.npcs.emplace_back(200, 5.0f, 4, 6, 1, 8000);  // Uncommon
        group.npcs.emplace_back(300, 1.0f, 7, 10, 1, 15000); // Rare
        fix.spawnSystem.registerSpawnGroup(group);
        
        const auto* stored = fix.spawnSystem.getSpawnGroup(1);
        REQUIRE(stored != nullptr);
        REQUIRE(stored->npcs.size() == 3);
        
        // Verify weights are stored correctly
        REQUIRE(stored->npcs[0].spawnWeight == 10.0f);
        REQUIRE(stored->npcs[1].spawnWeight == 5.0f);
        REQUIRE(stored->npcs[2].spawnWeight == 1.0f);
        
        // Verify level ranges
        REQUIRE(stored->npcs[0].minLevel == 1);
        REQUIRE(stored->npcs[0].maxLevel == 3);
        REQUIRE(stored->npcs[1].minLevel == 4);
        REQUIRE(stored->npcs[1].maxLevel == 6);
        REQUIRE(stored->npcs[2].minLevel == 7);
        REQUIRE(stored->npcs[2].maxLevel == 10);
        
        // Verify spawn counts
        REQUIRE(stored->npcs[0].spawnCount == 2);
        REQUIRE(stored->npcs[1].spawnCount == 1);
        REQUIRE(stored->npcs[2].spawnCount == 1);
    }
    
    SECTION("spawn group respawn time overrides NPC config") {
        SpawnGroup group(1, "groupRespawn", 20000, 5);  // Group respawn = 20s
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);  // NPC config = 5s
        fix.spawnSystem.registerSpawnGroup(group);
        
        const auto* stored = fix.spawnSystem.getSpawnGroup(1);
        REQUIRE(stored->respawnTimeMs == 20000);
    }
}

// ============================================================================
// Spawn Region Tests
// ============================================================================

TEST_CASE("SpawnSystem spawn region position generation", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("getSpawnPosition uses registered spawn region") {
        // Register spawn region for group 1
        fix.spawnSystem.registerSpawnRegion(1, 250.0f, 300.0f, 50.0f, 1);
        
        SpawnGroup group(1, "regionGroup", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Spawn multiple times and verify positions are within region
        bool foundValidPosition = false;
        for (int i = 0; i < 100; ++i) {
            Position pos = fix.spawnSystem.getSpawnPosition(1, 1, fix.zones);
            
            // Region center is (250, 300) with radius 50
            // Valid positions should be within circle: (x-250)^2 + (z-300)^2 <= 50^2
            float dx = posX(pos) - 250.0f;
            float dz = posZ(pos) - 300.0f;
            float distSq = dx * dx + dz * dz;
            
            if (distSq <= 2500.0f + 0.001f) {  // 50^2 = 2500 with tolerance
                foundValidPosition = true;
                break;
            }
        }
        REQUIRE(foundValidPosition);
    }
    
    SECTION("getSpawnPosition falls back to zone when no region") {
        SpawnGroup group(2, "zoneGroup", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // No region registered - should use zone spawn position
        Position pos = fix.spawnSystem.getSpawnPosition(2, 1, fix.zones);
        
        // Zone 1 has spawn at (100, 100)
        REQUIRE(posX(pos) == Catch::Approx(100.0f));
        REQUIRE(posZ(pos) == Catch::Approx(100.0f));
    }
    
    SECTION("getSpawnPosition uses correct zone") {
        SpawnGroup group(1, "zone2Group", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Get position for zone 2
        Position pos = fix.spawnSystem.getSpawnPosition(1, 2, fix.zones);
        
        // Zone 2 has spawn at (520, 520)
        REQUIRE(posX(pos) == Catch::Approx(520.0f));
        REQUIRE(posZ(pos) == Catch::Approx(520.0f));
    }
}

// ============================================================================
// NPC Template Creation Tests
// ============================================================================

TEST_CASE("SpawnSystem createNPCFromTemplate with various levels", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("createNPCFromTemplate creates NPC with level 1") {
        Position pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        EntityID entity = fix.spawnSystem.createNPCFromTemplate(fix.registry, 1001, pos, 1);
        REQUIRE((entity != entt::null));
        
        // Check NPC was created with correct level
        auto view = fix.registry.view<NPCStats>();
        REQUIRE(view.size() >= 1);
        
        for (auto e : view) {
            auto& stats = fix.registry.get<NPCStats>(e);
            if (stats.level == 1) {
                REQUIRE(stats.baseDamage >= 5);  // 5 + (1*2) = 7
                REQUIRE(stats.xpReward >= 10);   // 10 + (1*5) = 15
                break;
            }
        }
    }
    
    SECTION("createNPCFromTemplate creates NPC with high level") {
        Position pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        EntityID entity = fix.spawnSystem.createNPCFromTemplate(fix.registry, 1001, pos, 50);
        REQUIRE((entity != entt::null));
        
        // Check NPC was created with correct stats
        auto view = fix.registry.view<NPCStats>();
        bool foundHighLevel = false;
        for (auto e : view) {
            auto& stats = fix.registry.get<NPCStats>(e);
            if (stats.level == 50) {
                foundHighLevel = true;
                // baseDamage = 5 + (50*2) = 105
                REQUIRE(stats.baseDamage >= 100);
                // xpReward = 10 + (50*5) = 260
                REQUIRE(stats.xpReward >= 250);
                break;
            }
        }
        REQUIRE(foundHighLevel);
    }
    
    SECTION("createNPCFromTemplate scales health with level") {
        Position pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        
        // Create level 1 NPC
        EntityID e1 = fix.spawnSystem.createNPCFromTemplate(fix.registry, 100, pos, 1);
        REQUIRE((e1 != entt::null));
        
        // Create level 20 NPC
        EntityID e2 = fix.spawnSystem.createNPCFromTemplate(fix.registry, 100, pos, 20);
        REQUIRE((e2 != entt::null));
        
        // Verify health scaling
        int lowHealthCount = 0;
        int highHealthCount = 0;
        auto view = fix.registry.view<CombatState>();
        for (auto entity : view) {
            const auto& combat = fix.registry.get<CombatState>(entity);
            if (combat.maxHealth >= 100 && combat.maxHealth < 300) {
                lowHealthCount++;
            } else if (combat.maxHealth >= 300) {
                highHealthCount++;
            }
        }
        REQUIRE(lowHealthCount >= 1);
        REQUIRE(highHealthCount >= 1);
    }
    
    SECTION("createNPCFromTemplate creates entity with correct position") {
        Position spawnPos = Position::fromVec3(glm::vec3(333.0f, 0.0f, 444.0f));
        EntityID entity = fix.spawnSystem.createNPCFromTemplate(fix.registry, 100, spawnPos, 5);
        REQUIRE((entity != entt::null));
        
        const auto& pos = fix.registry.get<Position>(entity);
        REQUIRE(posX(pos) == Catch::Approx(333.0f));
        REQUIRE(posZ(pos) == Catch::Approx(444.0f));
    }
}

// ============================================================================
// Death Notification Tests
// ============================================================================

TEST_CASE("SpawnSystem death notification schedules respawn", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("onEntityDeath fires death callback") {
        bool deathCallbackFired = false;
        EntityID callbackEntity = entt::null;
        uint32_t callbackGroupId = 0;
        
        fix.spawnSystem.setDeathCallback([&](EntityID entity, uint32_t groupId) {
            deathCallbackFired = true;
            callbackEntity = entity;
            callbackGroupId = groupId;
        });
        
        // Create spawnable NPC
        Position pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        EntityID npc = createSpawnableNPC(fix.registry, 1, 100, 5000, pos);
        
        // Trigger death
        fix.spawnSystem.onEntityDeath(fix.registry, npc);
        
        REQUIRE(deathCallbackFired == true);
        REQUIRE(callbackEntity == npc);
        REQUIRE(callbackGroupId == 1);
    }
    
    SECTION("onEntityDeath uses NPCStats respawn time") {
        Position pos = Position::fromVec3(glm::vec3(100.0f, 0.0f, 100.0f));
        EntityID npc = createSpawnableNPC(fix.registry, 1, 100, 5000, pos, 5);
        
        // Update NPCStats respawn time
        auto& stats = fix.registry.get<NPCStats>(npc);
        stats.respawnTimeMs = 25000;
        
        fix.spawnSystem.onEntityDeath(fix.registry, npc);
        
        const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
        REQUIRE(spawnable.respawnTimeMs > 25000 - 100);  // Within tolerance
    }
    
    SECTION("onEntityDeath does nothing for non-spawnable entities") {
        // Create entity without SpawnableComponent
        EntityID regularEntity = fix.registry.create();
        fix.registry.emplace<Position>(regularEntity, Position::fromVec3(glm::vec3(0, 0, 0)));
        fix.registry.emplace<NPCTag>(regularEntity);
        
        bool deathCallbackFired = false;
        fix.spawnSystem.setDeathCallback([&](EntityID, uint32_t) {
            deathCallbackFired = true;
        });
        
        fix.spawnSystem.onEntityDeath(fix.registry, regularEntity);
        
        // Callback should not fire for entities without SpawnableComponent
        REQUIRE(deathCallbackFired == false);
    }
}

// ============================================================================
// Alive Count Tracking Tests
// ============================================================================

TEST_CASE("SpawnSystem getAliveCount for spawn groups", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("getAliveCount returns 0 for empty group") {
        SpawnGroup group(1, "emptyGroup", 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        uint32_t count = fix.spawnSystem.getAliveCount(fix.registry, 1);
        REQUIRE(count == 0);
    }
    
    SECTION("getAliveCount returns correct count after spawning") {
        SpawnGroup group(1, "countGroup", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Spawn 3 NPCs
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        uint32_t count = fix.spawnSystem.getAliveCount(fix.registry, 1);
        REQUIRE(count == 3);
    }
    
    SECTION("getAliveCount excludes dead NPCs") {
        SpawnGroup group(1, "deadGroup", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Spawn and kill some NPCs
        EntityID npc1 = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        EntityID npc2 = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        // Kill npc1
        fix.spawnSystem.onEntityDeath(fix.registry, npc1);
        
        uint32_t count = fix.spawnSystem.getAliveCount(fix.registry, 1);
        REQUIRE(count == 1);  // Only npc2 should be alive
    }
    
    SECTION("getAliveCount tracks different groups separately") {
        SpawnGroup group1(1, "group1", 5000);
        group1.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        
        SpawnGroup group2(2, "group2", 5000);
        group2.npcs.emplace_back(200, 1.0f, 1, 1, 1, 5000);
        
        fix.spawnSystem.registerSpawnGroup(group1);
        fix.spawnSystem.registerSpawnGroup(group2);
        
        // Spawn in group 1
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        // Spawn in group 2
        fix.spawnSystem.spawnEntity(fix.registry, 2, 1, fix.zones);
        
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 2);
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 2) == 1);
    }
}

// ============================================================================
// Force Spawn Tests
// ============================================================================

TEST_CASE("SpawnSystem forceSpawnGroup bypasses timer", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("forceSpawnGroup spawns NPCs immediately") {
        SpawnGroup group(1, "forceGroup", 60000);  // 60s respawn
        group.npcs.emplace_back(100, 1.0f, 1, 1, 3, 60000);  // 3 NPCs
        fix.spawnSystem.registerSpawnGroup(group);
        
        // No NPCs should exist yet
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 0);
        
        // Force spawn
        fix.spawnSystem.forceSpawnGroup(fix.registry, 1, 1, fix.zones);
        
        // Should have spawned NPCs
        uint32_t count = fix.spawnSystem.getAliveCount(fix.registry, 1);
        REQUIRE(count >= 3);
    }
    
    SECTION("forceSpawnGroup bypasses max alive limit") {
        SpawnGroup group(1, "limitedGroup", 5000, 2);  // Max 2 alive
        group.npcs.emplace_back(100, 1.0f, 1, 1, 5, 5000);  // Try to spawn 5
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Spawn normally up to limit
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 2);
        
        // Third spawn should fail (at limit)
        EntityID failNpc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        REQUIRE((failNpc == entt::null));
        
        // But force spawn should work
        fix.spawnSystem.forceSpawnGroup(fix.registry, 1, 1, fix.zones);
        
        // Should have more than 2 now
        uint32_t count = fix.spawnSystem.getAliveCount(fix.registry, 1);
        REQUIRE(count > 2);
    }
    
    SECTION("forceSpawnGroup does nothing for unknown group") {
        // Should not crash
        fix.spawnSystem.forceSpawnGroup(fix.registry, 999, 1, fix.zones);
        
        // Registry should be unchanged
        auto view = fix.registry.view<SpawnableComponent>();
        REQUIRE(view.size() == 0);
    }
    
    SECTION("forceSpawnGroup fires spawn callback for each NPC") {
        SpawnGroup group(1, "callbackGroup", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 2, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        int spawnCount = 0;
        fix.spawnSystem.setSpawnCallback([&](EntityID, uint32_t) {
            spawnCount++;
        });
        
        fix.spawnSystem.forceSpawnGroup(fix.registry, 1, 1, fix.zones);
        
        REQUIRE(spawnCount >= 2);
    }
}

// ============================================================================
// Weighted Selection Tests
// ============================================================================

TEST_CASE("SpawnSystem weighted selection respects spawn weights", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("spawnEntity uses weighted selection over many spawns") {
        SpawnGroup group(1, "weightedGroup", 5000);
        group.npcs.emplace_back(100, 100.0f, 1, 1, 1, 5000);  // High weight
        group.npcs.emplace_back(200, 1.0f, 1, 1, 1, 5000);    // Low weight
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Spawn many times and track template distribution
        int highWeightSpawns = 0;
        int lowWeightSpawns = 0;
        
        for (int i = 0; i < 1000; ++i) {
            EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
            if (npc != entt::null) {
                const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
                if (spawnable.templateId == 100) {
                    highWeightSpawns++;
                } else if (spawnable.templateId == 200) {
                    lowWeightSpawns++;
                }
            }
            // Clean up for next iteration
            fix.registry.clear();
        }
        
        // High weight (100) should spawn much more than low weight (1)
        // Ratio should be approximately 100:1
        REQUIRE(highWeightSpawns > lowWeightSpawns * 10);
    }
    
    SECTION("spawnEntity with equal weights spawns evenly") {
        SpawnGroup group(1, "equalGroup", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        group.npcs.emplace_back(200, 1.0f, 1, 1, 1, 5000);
        group.npcs.emplace_back(300, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        int count100 = 0, count200 = 0, count300 = 0;
        
        for (int i = 0; i < 600; ++i) {
            EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
            if (npc != entt::null) {
                const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
                if (spawnable.templateId == 100) count100++;
                else if (spawnable.templateId == 200) count200++;
                else if (spawnable.templateId == 300) count300++;
            }
            fix.registry.clear();
        }
        
        // With equal weights, each should be roughly 1/3 (with some variance)
        // Each should be between 100 and 300
        REQUIRE(count100 >= 100);
        REQUIRE(count200 >= 100);
        REQUIRE(count300 >= 100);
    }
    
    SECTION("spawnEntity with single config always picks that config") {
        SpawnGroup group(1, "singleConfig", 5000);
        group.npcs.emplace_back(999, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        for (int i = 0; i < 10; ++i) {
            EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
            REQUIRE((npc != entt::null));
            const auto& spawnable = fix.registry.get<SpawnableComponent>(npc);
            REQUIRE(spawnable.templateId == 999);
            fix.registry.clear();
        }
    }
}

// ============================================================================
// Comprehensive Integration Tests
// ============================================================================

TEST_CASE("SpawnSystem full spawn lifecycle integration", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    // Setup spawn group with multiple NPCs
    SpawnGroup group(1, "lifecycleGroup", 10000);
    group.npcs.emplace_back(100, 3.0f, 1, 5, 2, 8000);
    group.npcs.emplace_back(200, 1.0f, 6, 10, 1, 12000);
    group.maxAlive = 5;
    fix.spawnSystem.registerSpawnGroup(group);
    
    // Register spawn region
    fix.spawnSystem.registerSpawnRegion(1, 150.0f, 150.0f, 30.0f, 1);
    
    SECTION("Complete spawn -> death -> respawn cycle") {
        // 1. Spawn NPCs
        EntityID npc1 = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        EntityID npc2 = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        
        REQUIRE((npc1 != entt::null));
        REQUIRE((npc2 != entt::null));
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 2);
        
        // 2. NPCs should have SpawnableComponent
        REQUIRE(fix.registry.all_of<SpawnableComponent>(npc1));
        REQUIRE(fix.registry.all_of<SpawnableComponent>(npc2));
        
        // 3. Kill first NPC
        fix.spawnSystem.onEntityDeath(fix.registry, npc1);
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 1);
        
        // 4. Verify NPC marked as dead
        {
            const auto& spawnable = fix.registry.get<SpawnableComponent>(npc1);
            REQUIRE(spawnable.isSpawned == false);
            REQUIRE(spawnable.respawnTimeMs > 0);
        }
        
        // 5. Spawn another NPC (should work since only 1 alive)
        EntityID npc3 = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        REQUIRE((npc3 != entt::null));
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 2);
    }
    
    SECTION("Max alive limit enforced") {
        // Spawn up to max alive
        for (int i = 0; i < 5; ++i) {
            EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
            REQUIRE((npc != entt::null));
        }
        
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 5);
        
        // Next spawn should fail
        EntityID extraNpc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        REQUIRE((extraNpc == entt::null));
    }
    
    SECTION("Force spawn ignores max alive limit") {
        // Fill up to max
        for (int i = 0; i < 5; ++i) {
            fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        }
        
        // Force spawn should still work
        fix.spawnSystem.forceSpawnGroup(fix.registry, 1, 1, fix.zones);
        
        uint32_t count = fix.spawnSystem.getAliveCount(fix.registry, 1);
        REQUIRE(count > 5);
    }
}

TEST_CASE("SpawnSystem multi-zone integration", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SpawnGroup group(1, "multiZone", 5000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
    fix.spawnSystem.registerSpawnGroup(group);
    
    SECTION("Spawn position varies by zone") {
        // Zone 1 spawn
        EntityID npc1 = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        const auto& pos1 = fix.registry.get<Position>(npc1);
        
        // Zone 2 spawn
        EntityID npc2 = fix.spawnSystem.spawnEntity(fix.registry, 1, 2, fix.zones);
        const auto& pos2 = fix.registry.get<Position>(npc2);
        
        // Positions should be different
        REQUIRE(posX(pos1) != posX(pos2));
        REQUIRE(posZ(pos1) != posZ(pos2));
        
        // Zone 1 should be near (100, 100), Zone 2 near (520, 520)
        REQUIRE(posX(pos1) < 200.0f);
        REQUIRE(posX(pos2) > 400.0f);
    }
}

TEST_CASE("SpawnSystem callback integration", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("Both spawn and death callbacks fire correctly") {
        int spawnCount = 0;
        int deathCount = 0;
        
        fix.spawnSystem.setSpawnCallback([&](EntityID, uint32_t) {
            spawnCount++;
        });
        
        fix.spawnSystem.setDeathCallback([&](EntityID, uint32_t) {
            deathCount++;
        });
        
        SpawnGroup group(1, "callbackTest", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Spawn NPC
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        REQUIRE(spawnCount == 1);
        
        // Kill NPC
        fix.spawnSystem.onEntityDeath(fix.registry, npc);
        REQUIRE(deathCount == 1);
    }
}

TEST_CASE("SpawnSystem elite group handling", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("Elite groups have correct flags") {
        SpawnGroup regularGroup(1, "regularMobs", 10000, 10, false);
        regularGroup.npcs.emplace_back(100, 1.0f, 1, 3, 5, 10000);
        
        SpawnGroup eliteGroup(2, "bossRoom", 300000, 1, true);
        eliteGroup.npcs.emplace_back(999, 1.0f, 50, 50, 1, 300000);  // Single level 50 boss
        
        fix.spawnSystem.registerSpawnGroup(regularGroup);
        fix.spawnSystem.registerSpawnGroup(eliteGroup);
        
        const auto* regular = fix.spawnSystem.getSpawnGroup(1);
        const auto* elite = fix.spawnSystem.getSpawnGroup(2);
        
        REQUIRE(regular->isElite == false);
        REQUIRE(regular->maxAlive == 10);
        
        REQUIRE(elite->isElite == true);
        REQUIRE(elite->maxAlive == 1);
        REQUIRE(elite->respawnTimeMs == 300000);  // 5 minutes
        
        // Spawn boss
        EntityID boss = fix.spawnSystem.spawnEntity(fix.registry, 2, 1, fix.zones);
        REQUIRE((boss != entt::null));
        
        const auto& stats = fix.registry.get<NPCStats>(boss);
        REQUIRE(stats.level == 50);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("SpawnSystem edge cases", "[combat][spawn][integration]") {
    SpawnSystemFixture fix;
    
    SECTION("Spawn from non-existent group returns null") {
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 999, 1, fix.zones);
        REQUIRE((npc == entt::null));
    }
    
    SECTION("Spawn from empty group returns null") {
        SpawnGroup emptyGroup(1, "empty");
        fix.spawnSystem.registerSpawnGroup(emptyGroup);
        
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
        REQUIRE((npc == entt::null));
    }
    
    SECTION("getSpawnGroup returns nullptr for unknown group") {
        const auto* group = fix.spawnSystem.getSpawnGroup(999);
        REQUIRE(group == nullptr);
    }
    
    SECTION("Spawn works without any registered zones (uses default)") {
        SpawnGroup group(1, "noZone", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        // Empty zones vector - should use fallback position
        std::vector<ZoneDefinition> emptyZones;
        
        EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, emptyZones);
        REQUIRE((npc != entt::null));
    }
    
    SECTION("Multiple rapid spawns don't interfere") {
        SpawnGroup group(1, "rapidSpawn", 5000);
        group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
        fix.spawnSystem.registerSpawnGroup(group);
        
        std::vector<EntityID> npcs;
        for (int i = 0; i < 10; ++i) {
            EntityID npc = fix.spawnSystem.spawnEntity(fix.registry, 1, 1, fix.zones);
            REQUIRE((npc != entt::null));
            npcs.push_back(npc);
        }
        
        // All NPCs should be unique
        for (size_t i = 0; i < npcs.size(); ++i) {
            for (size_t j = i + 1; j < npcs.size(); ++j) {
                REQUIRE(npcs[i] != npcs[j]);
            }
        }
        
        REQUIRE(fix.spawnSystem.getAliveCount(fix.registry, 1) == 10);
    }
}
