#pragma once

#include "combat/WorldProgressionSystem.hpp"
#include "zones/ZoneObjectiveSystem.hpp"
#include <entt/entt.hpp>

// ============================================================================
// Zone Completion Handler — PRD-037 Integration
// Listens to ZoneObjectiveSystem and triggers world progression unlocks
// ============================================================================

namespace DarkAges {

class ZoneCompletionHandler {
public:
    ZoneCompletionHandler(entt::registry& registry, WorldProgressionSystem& worldSys)
        : registry_(registry), worldSys_(worldSys) {}
    
    // Initialize - subscribe to zone completion signals
    void initialize(ZoneObjectiveSystem& zoneObjSys) {
        // Connect to zone completion signal
        zoneObjSys.OnZoneCompleted().connect<&ZoneCompletionHandler::onZoneCompleted>(*this);
    }
    
    // Handle zone completion - unlock next zones
    void onZoneCompleted(EntityID player, uint32_t zoneId) {
        PlayerProgression* prog = registry_.try_get<PlayerProgression>(player);
        if (!prog) return;
        
        if (zoneId == ZONE_TUTORIAL) {
            worldSys_.completeTutorial(registry_, player);
        } else if (zoneId == ZONE_ARENA) {
            worldSys_.completeArena(registry_, player);
        } else if (zoneId == ZONE_BOSS) {
            worldSys_.completeBoss(registry_, player);
        }
    }
    
private:
    entt::registry& registry_;
    WorldProgressionSystem& worldSys_;
};

} // namespace DarkAges