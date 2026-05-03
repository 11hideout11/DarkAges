#pragma once

#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

// ============================================================================
// World Progression System — PRD-037 Integration
// Enforces zone progression: Tutorial → Arena → Boss → Open World
// ============================================================================

namespace DarkAges {

// Zone IDs from PRD-037
constexpr uint32_t ZONE_TUTORIAL = 98;
constexpr uint32_t ZONE_ARENA = 99;
constexpr uint32_t ZONE_BOSS = 100;

class WorldProgressionSystem {
public:
    WorldProgressionSystem() = default;
    
    // PRD-037: Check if player can access zone
    // Returns true if player has unlocked the zone
    bool canAccessZone(entt::registry& registry, EntityID player, uint32_t zoneId);
    
    // PRD-037: CompleteTutorial - unlock Arena
    // Call when player completes all tutorial objectives
    void completeTutorial(entt::registry& registry, EntityID player);
    
    // PRD-037: CompleteArena - unlock Boss
    // Call when player defeats Arena champion
    void completeArena(entt::registry& registry, EntityID player);
    
    // PRD-037: CompleteBoss - unlock Open World
    // Call when player defeats Boss
    void completeBoss(entt::registry& registry, EntityID player);
    
    // PRD-037: Get visible zones based on progression
    // Returns bitmask of zones player can see on world map
    uint32_t getVisibleZones(entt::registry& registry, EntityID player);
    
    // PRD-037: Get accessible zones
    // Returns bitmask of zones player can teleport to
    uint32_t getAccessibleZones(entt::registry& registry, EntityID player);
    
    // PRD-037: Enforce tutorial entry for new characters
    // Returns zone player should spawn in (tutorial for new characters)
    uint32_t getSpawnZone(entt::registry& registry, EntityID player);
    
private:
    void unlockZone(entt::registry& registry, EntityID player, uint32_t zoneId);
};

} // namespace DarkAges