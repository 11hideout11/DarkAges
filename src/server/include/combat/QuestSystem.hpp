#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Quest System — manages quest definitions, acceptance, progress tracking, and completion.
// Provides objectives like kill tracking, item collection, and level milestones.

namespace DarkAges {

class ItemSystem;

class QuestSystem {
public:
    QuestSystem() = default;

    // --- Quest Registry ---

    // Register a quest definition
    void registerQuest(const QuestDefinition& definition);

    // Get quest definition by ID
    const QuestDefinition* getQuest(uint32_t questId) const;

    // --- Quest Lifecycle ---

    // Accept a quest (adds to player's quest log)
    // Returns true if accepted successfully
    bool acceptQuest(Registry& registry, EntityID player, uint32_t questId,
                     uint32_t currentTimeMs);

    // Try to complete a quest (checks objectives, grants rewards)
    // Returns true if completed successfully
    bool completeQuest(Registry& registry, EntityID player, uint32_t questId);

    // Check if a quest can be accepted by a player
    bool canAcceptQuest(const Registry& registry, EntityID player, uint32_t questId) const;

    // Check if all objectives for an active quest are complete
    bool areObjectivesComplete(const Registry& registry, EntityID player, uint32_t questId) const;

    // --- Progress Tracking (called by game systems) ---

    // Called when an NPC is killed — updates kill objectives
    void onNPCKilled(Registry& registry, EntityID killer, uint32_t npcArchetypeId);

    // Called when an item is picked up — updates collect objectives
    void onItemCollected(Registry& registry, EntityID player, uint32_t itemId, uint32_t quantity);

    // Called when a player levels up — updates level objectives
    void onLevelUp(Registry& registry, EntityID player, uint32_t newLevel);

    // --- Starter Quests ---

    // Give new player their initial quests
    void giveStarterQuests(Registry& registry, EntityID player, uint32_t currentTimeMs);

    // --- Default Quest Database ---

    // Initialize the default quest database
    void initializeDefaults();

    // --- Callbacks ---

    using QuestCompleteCallback = std::function<void(EntityID player, uint32_t questId)>;
    void setQuestCompleteCallback(QuestCompleteCallback cb) { questCompleteCallback_ = std::move(cb); }

    using QuestProgressCallback = std::function<void(EntityID player, uint32_t questId,
                                                      uint32_t objectiveIndex, uint32_t current, uint32_t required)>;
    void setQuestProgressCallback(QuestProgressCallback cb) { questProgressCallback_ = std::move(cb); }

    // Set item system for reward distribution
    void setItemSystem(ItemSystem* is) { itemSystem_ = is; }

private:
    // Apply quest rewards to player
    void applyRewards(Registry& registry, EntityID player, const QuestReward& reward);

    // Quest registry — flat array indexed by quest ID
    static constexpr uint32_t MAX_QUESTS = 512;
    std::vector<QuestDefinition> quests_;

    QuestCompleteCallback questCompleteCallback_;
    QuestProgressCallback questProgressCallback_;
    ItemSystem* itemSystem_{nullptr};
};

} // namespace DarkAges
