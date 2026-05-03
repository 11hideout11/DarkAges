#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>

// Experience System — handles XP rewards and player leveling
// Awards XP when a player kills an NPC, handles level-up logic

namespace DarkAges {

class ProgressionCalculator;  // Forward declaration for PRD-036 wiring

class ExperienceSystem {
public:
    ExperienceSystem() = default;

    // Award XP to a player for killing an entity with XP reward
    // Returns true if the player leveled up
    bool awardKillXP(Registry& registry, EntityID killer, EntityID victim);

    // Award a flat XP amount to a player
    // Returns true if the player leveled up
    bool awardXP(Registry& registry, EntityID player, uint64_t xpAmount);

    // Check if player can level up and apply if so
    // Returns true if level-up occurred
    bool checkAndApplyLevelUp(Registry& registry, EntityID player);

    // Get XP needed for a given level (scaling formula)
    static uint64_t xpForLevel(uint32_t level);

    // Callback for level-up events
    using LevelUpCallback = std::function<void(EntityID player, uint32_t newLevel)>;
    void setLevelUpCallback(LevelUpCallback cb) { levelUpCallback_ = std::move(cb); }

    // Callback for XP gain events (for UI/combat log)
    using XPGainCallback = std::function<void(EntityID player, uint64_t xpGained)>;
    void setXPGainCallback(XPGainCallback cb) { xpGainCallback_ = std::move(cb); }

    // PRD-036: Set progression calculator for equipment-scaled level-up stats
    void setProgressionCalculator(ProgressionCalculator* calc) { progressionCalculator_ = calc; }

private:
    LevelUpCallback levelUpCallback_;
    XPGainCallback xpGainCallback_;
    ProgressionCalculator* progressionCalculator_{nullptr};
};

// ============================================================================
// Loot System — handles item drops on NPC death
// ============================================================================

class LootSystem {
public:
    LootSystem() = default;

    // Generate loot drops from a killed NPC
    // Creates loot entities on the ground at the NPC's position
    // Returns number of items dropped
    uint32_t generateLoot(Registry& registry, EntityID npc, EntityID killer);

    // Try to pick up loot for a player
    // Returns true if loot was picked up
    bool pickupLoot(Registry& registry, EntityID player, EntityID lootEntity);

    // Despawn expired loot drops
    void update(Registry& registry, uint32_t currentTimeMs);

    // Callback for loot drop events
    using LootDropCallback = std::function<void(EntityID lootEntity, uint32_t itemId,
                                                 uint32_t quantity, float gold)>;
    void setLootDropCallback(LootDropCallback cb) { lootDropCallback_ = std::move(cb); }

    // Callback for loot pickup events
    using LootPickupCallback = std::function<void(EntityID player, uint32_t itemId,
                                                   uint32_t quantity, float gold)>;
    void setLootPickupCallback(LootPickupCallback cb) { lootPickupCallback_ = std::move(cb); }

private:
    LootDropCallback lootDropCallback_;
    LootPickupCallback lootPickupCallback_;

    // Loot entity lifetime (60 seconds)
    static constexpr uint32_t LOOT_LIFETIME_MS = 60000;
};

} // namespace DarkAges