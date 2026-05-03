#include "combat/ProgressionCalculator.hpp"
#include "combat/ItemSystem.hpp"
#include "ecs/CoreTypes.hpp"

namespace DarkAges {

// ============================================================================
// Progression Calculator Implementation - PRD-036
// ============================================================================

int16_t ProgressionCalculator::scaleEquipmentBonus(int16_t baseBonus, uint32_t playerLevel,
                                              int16_t levelScalePercent) {
    if (baseBonus == 0 || levelScalePercent == 100) {
        return baseBonus;
    }
    
    // PRD-036 Formula: LevelBonus = PlayerLevel * ScalePercent / 100 * BaseBonus
    // Example: Level 5, Scale 150%, Base 10 → 5 * 1.5 * 10 = 75
    float scale = static_cast<float>(playerLevel) * levelScalePercent / 100.0f;
    return static_cast<int16_t>(baseBonus * scale);
}

void ProgressionCalculator::recalculateAllStats(entt::registry& registry, EntityID player) {
    // Get progression component
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return;
    
    // Get combat state
    CombatState* combat = registry.try_get<CombatState>(player);
    if (!combat) return;
    
    uint32_t playerLevel = prog->level;
    
    // Start with base stats
    int16_t totalSTR = combat->strength;
    int16_t totalDEX = combat->dexterity;
    int16_t totalVIT = combat->vitality;
    int16_t totalArmor = combat->finalArmor;
    int16_t totalBonusDamage = 0;
    int16_t totalBonusHP = 0;
    
    // Get equipment bonuses from inventory
    Inventory* inv = registry.try_get<Inventory>(player);
    if (inv && itemSystem_) {
        // Iterate equipped items
        for (const auto& slot : inv->slots) {
            if (slot.itemId == 0 || slot.quantity == 0) continue;
            
            // Get item definition
            const ItemDefinition* def = itemSystem_->getItem(slot.itemId);
            if (!def) continue;
            
            const ItemStats& stats = def->stats;
            
            // Skip non-equipment items
            if (def->equipSlot == EquipSlot::None) continue;
            
            // Apply level scaling to stats
            totalBonusDamage += scaleEquipmentBonus(
                stats.damageBonus, playerLevel, stats.levelScalePercent);
            totalBonusHP += scaleEquipmentBonus(
                stats.healthBonus, playerLevel, stats.levelScalePercent);
            totalArmor += scaleEquipmentBonus(
                static_cast<int16_t>(stats.healthBonus), 
                playerLevel, stats.levelScalePercent); // Use health as armor proxy
            
            // Add stat bonuses (not scaled directly)
            totalSTR += stats.strengthBonus;
            totalDEX += stats.dexterityBonus;
            totalVIT += stats.vitalityBonus;
        }
    }
    
    // Store base stats (before equipment)
    combat->strength = totalSTR;
    combat->dexterity = totalDEX;
    combat->vitality = totalVIT;
    
    // Apply combat's recalculate (adds base modifiers)
    combat->recalculateStats();
    
    // Add equipment bonuses
    combat->finalDamage += totalBonusDamage;
    combat->finalArmor = totalArmor;
    combat->maxHealth += totalBonusHP;
    
    // Ensure health doesn't exceed max after recalc
    if (combat->health > combat->maxHealth) {
        combat->health = combat->maxHealth;
    }
}

void ProgressionCalculator::applyLevelUp(entt::registry& registry, 
                                          EntityID player, uint32_t newLevel) {
    PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return;
    
    CombatState* combat = registry.try_get<CombatState>(player);
    if (!combat) return;
    
    // PRD-036: Base stat bonuses: +1 STR, +1 DEX, +1 VIT per level
    combat->strength += 1;
    combat->dexterity += 1;
    combat->vitality += 1;
    
    // Trigger stat recalculation
    recalculateAllStats(registry, player);
}

ProgressionCalculator::StatsBreakdown 
ProgressionCalculator::getStatsBreakdown(entt::registry& registry, EntityID player) {
    StatsBreakdown breakdown;
    
    CombatState* combat = registry.try_get<CombatState>(player);
    if (combat) {
        breakdown.totalHP = combat->maxHealth;
        breakdown.totalDamage = combat->finalDamage;
        breakdown.totalArmor = combat->finalArmor;
        breakdown.totalSTR = combat->strength;
        breakdown.totalDEX = combat->dexterity;
        breakdown.totalVIT = combat->vitality;
    }
    
    return breakdown;
}

} // namespace DarkAges