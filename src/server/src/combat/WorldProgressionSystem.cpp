#include "combat/WorldProgressionSystem.hpp"
#include "ecs/CoreTypes.hpp"

namespace DarkAges {

// ============================================================================
// World Progression System Implementation - PRD-037
// ============================================================================

bool WorldProgressionSystem::canAccessZone(entt::registry& registry, 
                                         EntityID player, uint32_t zoneId) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return false;
    
    // Tutorial always accessible
    if (zoneId == ZONE_TUTORIAL) return true;
    
    // Arena requires Tutorial complete
    if (zoneId == ZONE_ARENA) {
        return prog->tutorialComplete || prog->highestZoneUnlocked >= ZONE_ARENA;
    }
    
    // Boss requires Arena complete
    if (zoneId == ZONE_BOSS) {
        return prog->arenaComplete || prog->highestZoneUnlocked >= ZONE_BOSS;
    }
    
    // Future zones require highest unlock
    return zoneId <= prog->highestZoneUnlocked;
}

void WorldProgressionSystem::unlockZone(entt::registry& registry, 
                                      EntityID player, uint32_t zoneId) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return;
    
    if (zoneId > prog->highestZoneUnlocked) {
        prog->highestZoneUnlocked = zoneId;
    }
}

void WorldProgressionSystem::completeTutorial(entt::registry& registry, 
                                             EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return;
    
    prog->tutorialComplete = true;
    unlockZone(registry, player, ZONE_ARENA);
    
    // Also set initial spawn if first time
    if (prog->highestZoneUnlocked < ZONE_ARENA) {
        prog->highestZoneUnlocked = ZONE_ARENA;
    }
}

void WorldProgressionSystem::completeArena(entt::registry& registry, 
                                         EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return;
    
    prog->arenaComplete = true;
    unlockZone(registry, player, ZONE_BOSS);
}

void WorldProgressionSystem::completeBoss(entt::registry& registry, 
                                    EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return;
    
    prog->bossComplete = true;
    // Future: unlock open world zones
}

uint32_t WorldProgressionSystem::getVisibleZones(entt::registry& registry, 
                                             EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return 1 << ZONE_TUTORIAL; // Just tutorial
    
    uint32_t zones = 0;
    
    // Tutorial always visible
    zones |= (1 << ZONE_TUTORIAL);
    
    // Arena visible if tutorial started or complete
    if (prog->tutorialComplete || prog->highestZoneUnlocked >= ZONE_ARENA) {
        zones |= (1 << ZONE_ARENA);
    }
    
    // Boss visible if arena complete
    if (prog->arenaComplete || prog->highestZoneUnlocked >= ZONE_BOSS) {
        zones |= (1 << ZONE_ARENA);
        zones |= (1 << ZONE_BOSS);
    }
    
    return zones;
}

uint32_t WorldProgressionSystem::getAccessibleZones(entt::registry& registry, 
                                                  EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return 1 << ZONE_TUTORIAL;
    
    uint32_t zones = 0;
    
    // All unlocked zones are accessible
    for (uint32_t zone = ZONE_TUTORIAL; zone <= prog->highestZoneUnlocked && zone <= ZONE_BOSS; zone++) {
        if (canAccessZone(registry, player, zone)) {
            zones |= (1 << zone);
        }
    }
    
    return zones;
}

uint32_t WorldProgressionSystem::getSpawnZone(entt::registry& registry, 
                                             EntityID player) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return ZONE_TUTORIAL;
    
    // If player has never unlocked anything, start in Tutorial
    if (prog->highestZoneUnlocked <= ZONE_TUTORIAL && !prog->tutorialComplete) {
        return ZONE_TUTORIAL;
    }
    
    // Return to highest unlocked zone
    return prog->highestZoneUnlocked;
}

} // namespace DarkAges