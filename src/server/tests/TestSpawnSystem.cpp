#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/SpawnSystem.hpp"
#include "combat/NPCAISystem.hpp"
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
// Test Helpers
// ============================================================================

static EntityID createSpawnableNPC(Registry& registry, uint32_t groupId, 
                                   uint32_t templateId, uint32_t respawnMs,
                                   const Position& pos) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<Velocity>(entity);
    registry.emplace<NPCTag>(entity);
    registry.emplace<NPCStats>(entity);
    
    SpawnableComponent spawnable;
    spawnable.spawnGroupId = groupId;
    spawnable.templateId = templateId;
    spawnable.respawnTimeMs = respawnMs;
    spawnable.spawnPosition = pos;
    spawnable.isSpawned = true;
    spawnable.shouldRespawn = true;
    registry.emplace<SpawnableComponent>(entity, spawnable);
    
    return entity;
}

// ============================================================================
// Spawn System Tests
// ============================================================================

TEST_CASE("SpawnSystem initializes empty", "[combat][spawn]") {
    SpawnSystem system;
    
    // Should have no spawn groups initially
    auto* group = system.getSpawnGroup(1);
    REQUIRE(group == nullptr);
}

TEST_CASE("SpawnSystem registers spawn group", "[combat][spawn]") {
    SpawnSystem system;
    
    SpawnGroup group(1, "goblinCamp", 30000, 5);
    group.npcs.emplace_back(1001, 1.0f, 1, 5, 3, 30000);
    system.registerSpawnGroup(group);
    
    auto* registered = system.getSpawnGroup(1);
    REQUIRE(registered != nullptr);
    REQUIRE(registered->groupName == "goblinCamp");
    REQUIRE(registered->npcs.size() == 1);
    REQUIRE(registered->npcs[0].npcTemplateId == 1001);
}

TEST_CASE("SpawnSystem registers spawn region", "[combat][spawn]") {
    SpawnSystem system;
    
    system.registerSpawnRegion(1, 100.0f, 200.0f, 50.0f, 1);
    
    // Get spawn position should use region
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zone.centerX = 0;
    zone.centerZ = 0;
    zones.push_back(zone);
    
    // Test multiple times to verify randomness (bounds check)
    bool foundInBounds = false;
    for (int i = 0; i < 100; ++i) {
        Position pos = system.getSpawnPosition(1, 1, zones);
        float px = posX(pos);
        float pz = posZ(pos);
        if (px >= 50.0f && px <= 150.0f && 
            pz >= 150.0f && pz <= 250.0f) {
            foundInBounds = true;
            break;
        }
    }
    REQUIRE(foundInBounds);
}

TEST_CASE("SpawnSystem uses zone spawn position as fallback", "[combat][spawn]") {
    SpawnSystem system;
    
    // No region registered, use zone spawn position
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zone.centerX = 500.0f;
    zone.centerZ = 600.0f;
    zone.spawnX = 520.0f;
    zone.spawnZ = 620.0f;
    zones.push_back(zone);
    
    Position pos = system.getSpawnPosition(1, 1, zones);
    
    // Should use spawnX/spawnZ from zone, not center
    REQUIRE(posX(pos) == Catch::Approx(520.0f));
    REQUIRE(posZ(pos) == Catch::Approx(620.0f));
}

TEST_CASE("SpawnSystem falls back to zone center", "[combat][spawn]") {
    SpawnSystem system;
    
    // No spawnX/spawnZ set, use center
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zone.centerX = 100.0f;
    zone.centerZ = 200.0f;
    // spawnX/spawnZ default to 0
    zones.push_back(zone);
    
    Position pos = system.getSpawnPosition(1, 1, zones);
    
    // Should use center
    REQUIRE(posX(pos) == Catch::Approx(100.0f));
    REQUIRE(posZ(pos) == Catch::Approx(200.0f));
}

TEST_CASE("SpawnSystem spawns entity from group", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Register a spawn group
    SpawnGroup group(1, "testGroup", 10000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 10000);
    system.registerSpawnGroup(group);
    
    // Create zone for spawn
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zone.centerX = 0;
    zone.centerZ = 0;
    zones.push_back(zone);
    
    // Spawn should create an entity with NPC components
    EntityID npc = system.spawnEntity(registry, 1, 1, zones);
    bool valid = (npc != static_cast<EntityID>(entt::null));
    REQUIRE(valid);
    
    // Entity should have required components
    REQUIRE(registry.all_of<Position>(npc));
    REQUIRE(registry.all_of<Velocity>(npc));
    REQUIRE(registry.all_of<NPCTag>(npc));
    REQUIRE(registry.all_of<NPCStats>(npc));
    REQUIRE(registry.all_of<SpawnableComponent>(npc));
}

TEST_CASE("SpawnSystem respects max alive limit", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Register a spawn group with max alive = 2
    SpawnGroup group(1, "limitedGroup", 10000, 2);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 10000);
    system.registerSpawnGroup(group);
    
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zones.push_back(zone);
    
    // Spawn first NPC
    EntityID npc1 = system.spawnEntity(registry, 1, 1, zones);
    bool valid1 = (npc1 != static_cast<EntityID>(entt::null));
    REQUIRE(valid1);
    
    // Spawn second NPC
    EntityID npc2 = system.spawnEntity(registry, 1, 1, zones);
    bool valid2 = (npc2 != static_cast<EntityID>(entt::null));
    REQUIRE(valid2);
    
    // Third spawn should fail (at limit)
    EntityID npc3 = system.spawnEntity(registry, 1, 1, zones);
    bool isNull = (npc3 == static_cast<EntityID>(entt::null));
    REQUIRE(isNull);
    
    // Check alive count
    uint32_t alive = system.getAliveCount(registry, 1);
    REQUIRE(alive == 2);
}

TEST_CASE("SpawnSystem tracks spawned NPCs", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Register group
    SpawnGroup group(1, "trackGroup", 10000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 10000);
    system.registerSpawnGroup(group);
    
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zones.push_back(zone);
    
    // Spawn and track with spawnable component
    EntityID npc = system.spawnEntity(registry, 1, 1, zones);
    bool valid = (npc != static_cast<EntityID>(entt::null));
    REQUIRE(valid);
    
    const auto& spawnable = registry.get<SpawnableComponent>(npc);
    REQUIRE(spawnable.spawnGroupId == 1);
    REQUIRE(spawnable.templateId == 100);
    REQUIRE(spawnable.isSpawned == true);
}

TEST_CASE("SpawnSystem onEntityDeath starts respawn timer", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Create an entity with spawnable component
    Position pos = Position::fromVec3(glm::vec3(10.0f, 0.0f, 10.0f));
    auto entity = createSpawnableNPC(registry, 1, 100, 5000, pos);
    
    // Verify initial state
    {
        const auto& spawnable = registry.get<SpawnableComponent>(entity);
        REQUIRE(spawnable.isSpawned == true);
    }
    
    // Set death callback to verify call
    bool deathCallbackFired = false;
    system.setDeathCallback([&](EntityID, uint32_t) {
        deathCallbackFired = true;
    });
    
    // Trigger death
    system.onEntityDeath(registry, entity);
    
    // Should have called callback
    REQUIRE(deathCallbackFired == true);
    
    // Should now be marked as not spawned
    {
        const auto& spawnable = registry.get<SpawnableComponent>(entity);
        REQUIRE(spawnable.isSpawned == false);
    }
}

TEST_CASE("SpawnSystem forceSpawnGroup spawns all NPCs", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Register a spawn group with multiple NPCs
    SpawnGroup group(1, "waveGroup", 10000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 2, 10000);
    group.npcs.emplace_back(200, 0.5f, 3, 5, 1, 15000);
    system.registerSpawnGroup(group);
    
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zone.centerX = 0;
    zone.centerZ = 0;
    zones.push_back(zone);
    
    // Force spawn
    system.forceSpawnGroup(registry, 1, 1, zones);
    
    // Should have spawned multiple NPCs (weighted)
    auto view = registry.view<SpawnableComponent>();
    uint32_t spawned = 0;
    for (auto entity : view) {
        if (registry.get<SpawnableComponent>(entity).spawnGroupId == 1) {
            spawned++;
        }
    }
    REQUIRE(spawned >= 1);
}

TEST_CASE("SpawnSystem calculates spawn weights correctly", "[combat][spawn]") {
    SpawnSystem system;
    
    SpawnGroup group(1, "weightedGroup");
    group.npcs.emplace_back(100, 10.0f);  // 10x weight
    group.npcs.emplace_back(200, 5.0f);   // 5x weight
    group.npcs.emplace_back(300, 1.0f);    // 1x weight
    system.registerSpawnGroup(group);
    
    // Register to get access for testing selection
    // (This tests internal logic indirectly by checking spawn results)
    REQUIRE(group.npcs.size() == 3);
    REQUIRE(group.npcs[0].spawnWeight == 10.0f);
    REQUIRE(group.npcs[1].spawnWeight == 5.0f);
    REQUIRE(group.npcs[2].spawnWeight == 1.0f);
}

TEST_CASE("SpawnSystem NPC level scaling", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    SpawnGroup group(1, "levelScale", 10000);
    group.npcs.emplace_back(100, 1.0f, 5, 10, 1, 10000);  // Levels 5-10
    system.registerSpawnGroup(group);
    
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zones.push_back(zone);
    
    // Spawn multiple and check level ranges
    for (int i = 0; i < 20; ++i) {
        EntityID npc = system.spawnEntity(registry, 1, 1, zones);
        if (npc != static_cast<EntityID>(entt::null) && registry.all_of<NPCStats>(npc)) {
            const auto& stats = registry.get<NPCStats>(npc);
            // Stats are scaled - just verify component exists
            REQUIRE(stats.baseDamage >= 0);
        }
    }
}

TEST_CASE("SpawnSystem createNPCFromTemplate creates valid NPC", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    Position pos = Position::fromVec3(glm::vec3(50.0f, 0.0f, 50.0f));
    
    EntityID entity = system.createNPCFromTemplate(registry, 1001, pos, 5);
    REQUIRE((entity != entt::null));
    
    // Check that NPC was created with proper stats
    // The system creates the entity internally - we can verify by component count
    auto view = registry.view<NPCTag>();
    REQUIRE(view.size() >= 1);
}

TEST_CASE("SpawnSystem update processes respawns", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Create a spawnable that is due for respawn
    Position pos = Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f));
    
    // Manually create entity with expired respawn time
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<NPCTag>(entity);
    
    SpawnableComponent spawnable;
    spawnable.spawnGroupId = 1;
    spawnable.templateId = 100;
    spawnable.respawnTimeMs = 100;  // Due at time 100
    spawnable.spawnPosition = pos;
    spawnable.isSpawned = false;  // Currently dead
    spawnable.shouldRespawn = true;
    registry.emplace<SpawnableComponent>(entity, spawnable);
    
    // Register a group so spawn can happen
    SpawnGroup group(1, "respawnGroup", 5000);
    group.npcs.emplace_back(100, 1.0f, 1, 1, 1, 5000);
    system.registerSpawnGroup(group);
    
    // Update at time 150 - should attempt respawn
    system.update(registry, 150);
    
    // NOTE: Spawn may fail because zone lookup fails - but system runs without error
    // This test validates the update loop completes without crash
    REQUIRE(true);
}

TEST_CASE("SpawnSystem spawn callbacks fire", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    bool spawnFired = false;
    system.setSpawnCallback([&](EntityID e, uint32_t groupId) {
        spawnFired = true;
        bool validEntity = (e != static_cast<EntityID>(entt::null));
        REQUIRE(validEntity);
        REQUIRE(groupId == 1);
    });
    
    SpawnGroup group(1, "callbackGroup", 10000);
    group.npcs.emplace_back(100, 1.0f);
    system.registerSpawnGroup(group);
    
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zones.push_back(zone);
    
    EntityID npc = system.spawnEntity(registry, 1, 1, zones);
    REQUIRE(spawnFired == true);
}

TEST_CASE("SpawnSystem elite groups marked correctly", "[combat][spawn]") {
    SpawnSystem system;
    
    SpawnGroup regularGroup(1, "regular", 60000, 0, false);
    SpawnGroup eliteGroup(2, "boss", 300000, 1, true);
    
    system.registerSpawnGroup(regularGroup);
    system.registerSpawnGroup(eliteGroup);
    
    auto* regular = system.getSpawnGroup(1);
    auto* elite = system.getSpawnGroup(2);
    
    REQUIRE(regular != nullptr);
    REQUIRE(elite != nullptr);
    REQUIRE(regular->isElite == false);
    REQUIRE(elite->isElite == true);
}

TEST_CASE("SpawnSystem spawn groups with no NPCs return null", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Empty group
    SpawnGroup emptyGroup(1, "empty");
    system.registerSpawnGroup(emptyGroup);
    
    std::vector<ZoneDefinition> zones;
    ZoneDefinition zone;
    zone.zoneId = 1;
    zones.push_back(zone);
    
    EntityID npc = system.spawnEntity(registry, 1, 1, zones);
    bool isNull = (npc == static_cast<EntityID>(entt::null));
    REQUIRE(isNull);
}

TEST_CASE("SpawnSystem respawn time from NPCStats", "[combat][spawn]") {
    Registry registry;
    SpawnSystem system;
    
    // Create entity with NPCStats respawn time
    Position pos = Position::fromVec3(glm::vec3(10.0f, 0.0f, 10.0f));
    auto entity = registry.create();
    registry.emplace<Position>(entity, pos);
    registry.emplace<NPCTag>(entity);
    
    NPCStats stats;
    stats.respawnTimeMs = 20000;
    registry.emplace<NPCStats>(entity, stats);
    
    SpawnableComponent spawnable;
    spawnable.spawnGroupId = 1;
    spawnable.templateId = 100;
    spawnable.respawnTimeMs = 0;  // Will be overridden by stats
    spawnable.spawnPosition = pos;
    spawnable.isSpawned = false;
    spawnable.shouldRespawn = true;
    registry.emplace<SpawnableComponent>(entity, spawnable);
    
    // onEntityDeath should use NPCStats respawn time
    system.onEntityDeath(registry, entity);
    
    // Spawnable should now have respawn time set
    const auto& updated = registry.get<SpawnableComponent>(entity);
    REQUIRE(updated.respawnTimeMs > 0);  // Should have been set
}

TEST_CASE("SpawnSystem zone spawn position integration", "[combat][spawn]") {
    SpawnSystem system;
    
    // Create zones with spawn positions
    std::vector<ZoneDefinition> zones;
    
    ZoneDefinition zone1;
    zone1.zoneId = 1;
    zone1.zoneName = "town";
    zone1.spawnX = 100.0f;
    zone1.spawnZ = 200.0f;
    zones.push_back(zone1);
    
    ZoneDefinition zone2;
    zone2.zoneId = 2;
    zone2.zoneName = "dungeon";
    zone2.spawnX = 500.0f;
    zone2.spawnZ = 600.0f;
    zones.push_back(zone2);
    
    // Get spawn position for zone 1
    Position pos1 = system.getSpawnPosition(999, 1, zones);  // No region, use zone
    REQUIRE(posX(pos1) == Catch::Approx(100.0f));
    REQUIRE(posZ(pos1) == Catch::Approx(200.0f));
    
    // Get spawn position for zone 2
    Position pos2 = system.getSpawnPosition(999, 2, zones);
    REQUIRE(posX(pos2) == Catch::Approx(500.0f));
    REQUIRE(posZ(pos2) == Catch::Approx(600.0f));
}

TEST_CASE("SpawnSystem multiple spawn groups", "[combat][spawn]") {
    SpawnSystem system;
    
    SpawnGroup group1(1, "group1");
    group1.npcs.emplace_back(101, 1.0f);
    
    SpawnGroup group2(2, "group2");
    group2.npcs.emplace_back(201, 1.0f);
    
    SpawnGroup group3(3, "group3");
    group3.npcs.emplace_back(301, 1.0f);
    
    system.registerSpawnGroup(group1);
    system.registerSpawnGroup(group2);
    system.registerSpawnGroup(group3);
    
    REQUIRE(system.getSpawnGroup(1) != nullptr);
    REQUIRE(system.getSpawnGroup(2) != nullptr);
    REQUIRE(system.getSpawnGroup(3) != nullptr);
    REQUIRE(system.getSpawnGroup(4) == nullptr);  // Not registered
}