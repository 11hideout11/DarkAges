#pragma once

#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

// ============================================================================
// Progression Calculator — PRD-036 Integration
// Calculates final character stats from base, level, and equipment
// ============================================================================

namespace DarkAges {

// Forward declarations
class ItemSystem;
class InventoryComponent;

class ProgressionCalculator {
public:
    ProgressionCalculator() = default;
    void setItemSystem(ItemSystem* system) { itemSystem_ = system; }
    
    // PRD-036: Recalculate all stats for a player entity
    // Called when: level changes, equipment changes, stats spent
    void recalculateAllStats(entt::registry& registry, EntityID player);
    
    // PRD-036: Apply level-up stat bonuses
    void applyLevelUp(entt::registry& registry, EntityID player, uint32_t newLevel);
    
    // PRD-036: Scale equipment with player level
    // Returns scaled stat bonus based on player level and item level scale
    static int16_t scaleEquipmentBonus(int16_t baseBonus, uint32_t playerLevel, 
                            int16_t levelScalePercent = 100);
    
    // PRD-036: Get final combat stats breakdown
    struct StatsBreakdown {
        int16_t totalHP{0};
        int16_t totalDamage{0};
        int16_t totalArmor{0};
        int16_t totalSTR{0};
        int16_t totalDEX{0};
        int16_t totalVIT{0};
    };
    
    StatsBreakdown getStatsBreakdown(entt::registry& registry, EntityID player);
    
private:
    ItemSystem* itemSystem_{nullptr};
};

} // namespace DarkAges