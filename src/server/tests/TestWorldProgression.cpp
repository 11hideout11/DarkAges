#include <catch2/catch_test_macros.hpp>
#include "combat/WorldProgressionSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

// ============================================================================
// World Progression System Tests - PRD-037
// ============================================================================

TEST_CASE("WorldProgression new player spawns in Tutorial", "[world][progression]") {
    entt::registry registry;
    WorldProgressionSystem worldSys;

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);

    // New player should spawn in Tutorial
    CHECK(worldSys.getSpawnZone(registry, player) == ZONE_TUTORIAL);
    
    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.highestZoneUnlocked == 98); // Default
    CHECK(prog.tutorialComplete == false);
}

TEST_CASE("WorldProgression tutorial unlock", "[world][progression]") {
    entt::registry registry;
    WorldProgressionSystem worldSys;

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);

    // Complete tutorial
    worldSys.completeTutorial(registry, player);
    
    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.tutorialComplete == true);
    CHECK(prog.highestZoneUnlocked >= ZONE_ARENA);
    
    // Arena now accessible
    CHECK(worldSys.canAccessZone(registry, player, ZONE_ARENA) == true);
    
    // Boss still locked
    CHECK(worldSys.canAccessZone(registry, player, ZONE_BOSS) == false);
}

TEST_CASE("WorldProgression arena unlock", "[world][progression]") {
    entt::registry registry;
    WorldProgressionSystem worldSys;

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);

    // First complete tutorial
    worldSys.completeTutorial(registry, player);
    
    // Then complete arena
    worldSys.completeArena(registry, player);
    
    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    CHECK(prog.arenaComplete == true);
    CHECK(prog.highestZoneUnlocked >= ZONE_BOSS);
    
    // Boss now accessible
    CHECK(worldSys.canAccessZone(registry, player, ZONE_BOSS) == true);
}

TEST_CASE("WorldProgression canAccessZone enforces locks", "[world][progression]") {
    entt::registry registry;
    WorldProgressionSystem worldSys;

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);

    // Tutorial should always be accessible
    CHECK(worldSys.canAccessZone(registry, player, ZONE_TUTORIAL) == true);
    
    // Arena should be locked without tutorial
    CHECK(worldSys.canAccessZone(registry, player, ZONE_ARENA) == false);
    
    // After tutorial complete, arena accessible
    worldSys.completeTutorial(registry, player);
    CHECK(worldSys.canAccessZone(registry, player, ZONE_ARENA) == true);
    
    // Boss still locked
    CHECK(worldSys.canAccessZone(registry, player, ZONE_BOSS) == false);
}

TEST_CASE("WorldProgression getVisibleZones", "[world][progression]") {
    entt::registry registry;
    WorldProgressionSystem worldSys;

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);

    // New player sees only tutorial
    uint32_t visible = worldSys.getVisibleZones(registry, player);
    CHECK(visible & (1 << ZONE_TUTORIAL));
    CHECK_FALSE(visible & (1 << ZONE_ARENA));
    CHECK_FALSE(visible & (1 << ZONE_BOSS));
    
    // After tutorial: tutorial + arena visible
    worldSys.completeTutorial(registry, player);
    visible = worldSys.getVisibleZones(registry, player);
    CHECK(visible & (1 << ZONE_TUTORIAL));
    CHECK(visible & (1 << ZONE_ARENA));
    CHECK_FALSE(visible & (1 << ZONE_BOSS));
    
    // After arena: all visible
    worldSys.completeArena(registry, player);
    visible = worldSys.getVisibleZones(registry, player);
    CHECK(visible & (1 << ZONE_TUTORIAL));
    CHECK(visible & (1 << ZONE_ARENA));
    CHECK(visible & (1 << ZONE_BOSS));
}