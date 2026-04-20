#include "combat/QuestSystem.hpp"
#include "combat/ItemSystem.hpp"
#include <cstring>
#include <iostream>

namespace DarkAges {

// ============================================================================
// Quest Registry
// ============================================================================

void QuestSystem::registerQuest(const QuestDefinition& definition) {
    if (definition.questId == 0) return;

    if (definition.questId >= quests_.size()) {
        quests_.resize(definition.questId + 1);
    }

    quests_[definition.questId] = definition;
}

const QuestDefinition* QuestSystem::getQuest(uint32_t questId) const {
    if (questId == 0 || questId >= quests_.size()) return nullptr;
    const auto& quest = quests_[questId];
    if (quest.questId == 0) return nullptr;
    return &quest;
}

// ============================================================================
// Quest Lifecycle
// ============================================================================

bool QuestSystem::canAcceptQuest(const Registry& registry, EntityID player,
                                  uint32_t questId) const {
    const QuestDefinition* def = getQuest(questId);
    if (!def) return false;

    // Check level requirement first (no prerequisites needed for this check)
    const PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (prog && prog->level < def->requiredLevel) return false;

    // Get player's quest log
    const QuestLog* log = registry.try_get<QuestLog>(player);
    if (!log) return true; // No quest log = can accept (passed level check above)

    // Check if already active
    if (log->hasActiveQuest(questId)) return false;

    // Check if already completed (and not repeatable)
    if (log->hasCompletedQuest(questId) && !def->repeatable) return false;

    // Check prerequisite quest
    if (def->prerequisiteQuestId > 0) {
        if (!log->hasCompletedQuest(def->prerequisiteQuestId)) return false;
    }

    return true;
}

bool QuestSystem::acceptQuest(Registry& registry, EntityID player, uint32_t questId,
                               uint32_t currentTimeMs) {
    if (!canAcceptQuest(registry, player, questId)) return false;

    // Ensure quest log
    if (!registry.all_of<QuestLog>(player)) {
        registry.emplace<QuestLog>(player);
    }

    QuestLog& log = registry.get<QuestLog>(player);
    if (!log.addActiveQuest(questId, currentTimeMs)) return false;

    std::cout << "[QUEST] Player " << static_cast<uint32_t>(player)
              << " accepted quest " << questId << std::endl;
    return true;
}

bool QuestSystem::areObjectivesComplete(const Registry& registry, EntityID player,
                                         uint32_t questId) const {
    const QuestDefinition* def = getQuest(questId);
    if (!def) return false;

    const QuestLog* log = registry.try_get<QuestLog>(player);
    if (!log) return false;

    const QuestProgress* progress = log->getActiveQuest(questId);
    if (!progress || !progress->accepted) return false;

    // Check each objective
    for (uint32_t i = 0; i < def->objectiveCount; ++i) {
        if (!progress->objectives[i].completed) return false;
    }

    return true;
}

bool QuestSystem::completeQuest(Registry& registry, EntityID player, uint32_t questId) {
    const QuestDefinition* def = getQuest(questId);
    if (!def) return false;

    if (!areObjectivesComplete(registry, player, questId)) return false;

    // Apply rewards
    applyRewards(registry, player, def->reward);

    // Move from active to completed
    QuestLog& log = registry.get<QuestLog>(player);
    log.completeQuest(questId);

    std::cout << "[QUEST] Player " << static_cast<uint32_t>(player)
              << " completed quest " << questId << " (" << def->name << ")" << std::endl;

    // Fire callback
    if (questCompleteCallback_) {
        questCompleteCallback_(player, questId);
    }

    return true;
}

// ============================================================================
// Progress Tracking
// ============================================================================

void QuestSystem::onNPCKilled(Registry& registry, EntityID killer, uint32_t npcArchetypeId) {
    QuestLog* log = registry.try_get<QuestLog>(killer);
    if (!log) return;

    for (uint32_t i = 0; i < log->activeCount; ++i) {
        QuestProgress& progress = log->activeQuests[i];
        if (!progress.accepted || progress.completed) continue;

        const QuestDefinition* def = getQuest(progress.questId);
        if (!def) continue;

        for (uint32_t obj = 0; obj < def->objectiveCount; ++obj) {
            if (progress.objectives[obj].completed) continue;

            const QuestObjective& objective = def->objectives[obj];
            if (objective.type == QuestObjectiveType::KillNPC &&
                objective.targetId == npcArchetypeId) {
                progress.objectives[obj].currentCount++;
                if (progress.objectives[obj].currentCount >= objective.requiredCount) {
                    progress.objectives[obj].completed = true;
                    std::cout << "[QUEST] Player " << static_cast<uint32_t>(killer)
                              << " completed objective " << obj << " of quest "
                              << progress.questId << std::endl;
                }

                // Fire progress callback
                if (questProgressCallback_) {
                    questProgressCallback_(killer, progress.questId, obj,
                                           progress.objectives[obj].currentCount,
                                           objective.requiredCount);
                }
            }
        }
    }
}

void QuestSystem::onItemCollected(Registry& registry, EntityID player,
                                   uint32_t itemId, uint32_t quantity) {
    QuestLog* log = registry.try_get<QuestLog>(player);
    if (!log) return;

    for (uint32_t i = 0; i < log->activeCount; ++i) {
        QuestProgress& progress = log->activeQuests[i];
        if (!progress.accepted || progress.completed) continue;

        const QuestDefinition* def = getQuest(progress.questId);
        if (!def) continue;

        for (uint32_t obj = 0; obj < def->objectiveCount; ++obj) {
            if (progress.objectives[obj].completed) continue;

            const QuestObjective& objective = def->objectives[obj];
            if (objective.type == QuestObjectiveType::CollectItem &&
                objective.targetId == itemId) {
                progress.objectives[obj].currentCount += quantity;
                if (progress.objectives[obj].currentCount >= objective.requiredCount) {
                    progress.objectives[obj].completed = true;
                }

                if (questProgressCallback_) {
                    questProgressCallback_(player, progress.questId, obj,
                                           progress.objectives[obj].currentCount,
                                           objective.requiredCount);
                }
            }
        }
    }
}

void QuestSystem::onLevelUp(Registry& registry, EntityID player, uint32_t newLevel) {
    QuestLog* log = registry.try_get<QuestLog>(player);
    if (!log) return;

    for (uint32_t i = 0; i < log->activeCount; ++i) {
        QuestProgress& progress = log->activeQuests[i];
        if (!progress.accepted || progress.completed) continue;

        const QuestDefinition* def = getQuest(progress.questId);
        if (!def) continue;

        for (uint32_t obj = 0; obj < def->objectiveCount; ++obj) {
            if (progress.objectives[obj].completed) continue;

            const QuestObjective& objective = def->objectives[obj];
            if (objective.type == QuestObjectiveType::ReachLevel &&
                newLevel >= objective.requiredCount) {
                progress.objectives[obj].currentCount = newLevel;
                progress.objectives[obj].completed = true;

                if (questProgressCallback_) {
                    questProgressCallback_(player, progress.questId, obj,
                                           newLevel, objective.requiredCount);
                }
            }
        }
    }
}

// ============================================================================
// Rewards
// ============================================================================

void QuestSystem::applyRewards(Registry& registry, EntityID player,
                                const QuestReward& reward) {
    // Grant XP
    if (reward.xpReward > 0) {
        PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
        if (prog) {
            prog->currentXP += reward.xpReward;
            std::cout << "[QUEST] Granted " << reward.xpReward << " XP to player "
                      << static_cast<uint32_t>(player) << std::endl;
        }
    }

    // Grant gold
    if (reward.goldReward > 0) {
        Inventory* inv = registry.try_get<Inventory>(player);
        if (inv) {
            inv->gold += static_cast<float>(reward.goldReward);
            std::cout << "[QUEST] Granted " << reward.goldReward << " gold to player "
                      << static_cast<uint32_t>(player) << std::endl;
        }
    }

    // Grant item
    if (reward.itemId > 0 && reward.itemQuantity > 0 && itemSystem_) {
        uint32_t overflow = itemSystem_->addToInventory(registry, player,
                                                         reward.itemId, reward.itemQuantity);
        if (overflow > 0) {
            std::cout << "[QUEST] Player " << static_cast<uint32_t>(player)
                      << " inventory full — " << overflow << " reward items lost" << std::endl;
        }
    }
}

// ============================================================================
// Starter Quests
// ============================================================================

void QuestSystem::giveStarterQuests(Registry& registry, EntityID player,
                                     uint32_t currentTimeMs) {
    // Give the first quest (tutorial)
    if (canAcceptQuest(registry, player, 1)) {
        acceptQuest(registry, player, 1, currentTimeMs);
    }
}

// ============================================================================
// Default Quest Database
// ============================================================================

void QuestSystem::initializeDefaults() {
    quests_.clear();
    quests_.resize(100);

    // Quest 1: "Proving Grounds" — Kill 5 Melee NPCs
    {
        QuestDefinition quest{};
        quest.questId = 1;
        std::strncpy(quest.name, "Proving Grounds", sizeof(quest.name) - 1);
        std::strncpy(quest.description, "Prove your worth by defeating the creatures near town.",
                      sizeof(quest.description) - 1);
        std::strncpy(quest.startDialogue,
                      "Welcome, adventurer! The creatures outside grow bold. "
                      "Slay 5 of them and return to me.",
                      sizeof(quest.startDialogue) - 1);
        std::strncpy(quest.completionDialogue,
                      "Well done! You have proven yourself worthy. "
                      "Take this reward and continue your journey.",
                      sizeof(quest.completionDialogue) - 1);
        quest.requiredLevel = 1;
        quest.objectiveCount = 1;
        quest.objectives[0].type = QuestObjectiveType::KillNPC;
        quest.objectives[0].targetId = static_cast<uint32_t>(NPCArchetype::Melee);
        quest.objectives[0].requiredCount = 5;
        std::strncpy(quest.objectives[0].description, "Kill 5 Melee creatures",
                      sizeof(quest.objectives[0].description) - 1);
        quest.reward.xpReward = 100;
        quest.reward.goldReward = 50;
        quest.reward.itemId = 10; // Health Potion
        quest.reward.itemQuantity = 3;
        registerQuest(quest);
    }

    // Quest 2: "The Hunt Deepens" — Kill 3 Ranged NPCs
    {
        QuestDefinition quest{};
        quest.questId = 2;
        std::strncpy(quest.name, "The Hunt Deepens", sizeof(quest.name) - 1);
        std::strncpy(quest.description, "The ranged creatures are more dangerous. Hunt them down.",
                      sizeof(quest.description) - 1);
        std::strncpy(quest.startDialogue,
                      "The archers in the field are picking off travelers. "
                      "Take care of 3 of them.",
                      sizeof(quest.startDialogue) - 1);
        std::strncpy(quest.completionDialogue,
                      "You are becoming quite the hunter! Here is your reward.",
                      sizeof(quest.completionDialogue) - 1);
        quest.requiredLevel = 3;
        quest.prerequisiteQuestId = 1;
        quest.objectiveCount = 1;
        quest.objectives[0].type = QuestObjectiveType::KillNPC;
        quest.objectives[0].targetId = static_cast<uint32_t>(NPCArchetype::Ranged);
        quest.objectives[0].requiredCount = 3;
        std::strncpy(quest.objectives[0].description, "Kill 3 Ranged creatures",
                      sizeof(quest.objectives[0].description) - 1);
        quest.reward.xpReward = 200;
        quest.reward.goldReward = 100;
        quest.reward.itemId = 3; // Iron Longsword
        quest.reward.itemQuantity = 1;
        registerQuest(quest);
    }

    // Quest 3: "Arcane Threat" — Kill 3 Caster NPCs
    {
        QuestDefinition quest{};
        quest.questId = 3;
        std::strncpy(quest.name, "Arcane Threat", sizeof(quest.name) - 1);
        std::strncpy(quest.description, "The casters' magic grows stronger. Put an end to them.",
                      sizeof(quest.description) - 1);
        std::strncpy(quest.startDialogue,
                      "The mages in the ruins are conjuring dark forces. "
                      "Defeat 3 of them before it's too late!",
                      sizeof(quest.startDialogue) - 1);
        std::strncpy(quest.completionDialogue,
                      "The dark magic subsides. You have done well, warrior.",
                      sizeof(quest.completionDialogue) - 1);
        quest.requiredLevel = 5;
        quest.prerequisiteQuestId = 2;
        quest.objectiveCount = 1;
        quest.objectives[0].type = QuestObjectiveType::KillNPC;
        quest.objectives[0].targetId = static_cast<uint32_t>(NPCArchetype::Caster);
        quest.objectives[0].requiredCount = 3;
        std::strncpy(quest.objectives[0].description, "Kill 3 Caster creatures",
                      sizeof(quest.objectives[0].description) - 1);
        quest.reward.xpReward = 350;
        quest.reward.goldReward = 200;
        quest.reward.itemId = 4; // Flame Staff
        quest.reward.itemQuantity = 1;
        registerQuest(quest);
    }

    // Quest 4: "The Alpha Predator" — Kill a Boss
    {
        QuestDefinition quest{};
        quest.questId = 4;
        std::strncpy(quest.name, "The Alpha Predator", sizeof(quest.name) - 1);
        std::strncpy(quest.description, "A powerful boss creature terrorizes the land. Slay it.",
                      sizeof(quest.description) - 1);
        std::strncpy(quest.startDialogue,
                      "A great beast has been spotted. It has killed many adventurers. "
                      "You must be the one to end its reign of terror.",
                      sizeof(quest.startDialogue) - 1);
        std::strncpy(quest.completionDialogue,
                      "You have slain the beast! The land is safe once more. "
                      "You are a true hero!",
                      sizeof(quest.completionDialogue) - 1);
        quest.requiredLevel = 8;
        quest.prerequisiteQuestId = 3;
        quest.objectiveCount = 1;
        quest.objectives[0].type = QuestObjectiveType::KillNPC;
        quest.objectives[0].targetId = static_cast<uint32_t>(NPCArchetype::Boss);
        quest.objectives[0].requiredCount = 1;
        std::strncpy(quest.objectives[0].description, "Kill 1 Boss creature",
                      sizeof(quest.objectives[0].description) - 1);
        quest.reward.xpReward = 500;
        quest.reward.goldReward = 500;
        quest.reward.itemId = 8; // Lucky Amulet
        quest.reward.itemQuantity = 1;
        registerQuest(quest);
    }

    // Quest 5: "The Collector" — Collect materials (repeatable)
    {
        QuestDefinition quest{};
        quest.questId = 5;
        std::strncpy(quest.name, "The Collector", sizeof(quest.name) - 1);
        std::strncpy(quest.description, "Gather materials from fallen creatures for research.",
                      sizeof(quest.description) - 1);
        std::strncpy(quest.startDialogue,
                      "I need wolf pelts for my research. Bring me 10 and I'll reward you.",
                      sizeof(quest.startDialogue) - 1);
        std::strncpy(quest.completionDialogue,
                      "Excellent specimens! Here is your payment.",
                      sizeof(quest.completionDialogue) - 1);
        quest.requiredLevel = 1;
        quest.objectiveCount = 1;
        quest.objectives[0].type = QuestObjectiveType::CollectItem;
        quest.objectives[0].targetId = 20; // Wolf Pelt
        quest.objectives[0].requiredCount = 10;
        std::strncpy(quest.objectives[0].description, "Collect 10 Wolf Pelts",
                      sizeof(quest.objectives[0].description) - 1);
        quest.reward.xpReward = 75;
        quest.reward.goldReward = 30;
        quest.repeatable = true;
        registerQuest(quest);
    }

    // Quest 6: "The Journey Begins" — Reach level 5
    {
        QuestDefinition quest{};
        quest.questId = 6;
        std::strncpy(quest.name, "The Journey Begins", sizeof(quest.name) - 1);
        std::strncpy(quest.description, "Grow stronger by gaining experience.",
                      sizeof(quest.description) - 1);
        std::strncpy(quest.startDialogue,
                      "Every hero must start somewhere. Gain experience and reach level 5.",
                      sizeof(quest.startDialogue) - 1);
        std::strncpy(quest.completionDialogue,
                      "You have grown strong! Your journey is just beginning.",
                      sizeof(quest.completionDialogue) - 1);
        quest.requiredLevel = 1;
        quest.objectiveCount = 1;
        quest.objectives[0].type = QuestObjectiveType::ReachLevel;
        quest.objectives[0].targetId = 0;
        quest.objectives[0].requiredCount = 5;
        std::strncpy(quest.objectives[0].description, "Reach level 5",
                      sizeof(quest.objectives[0].description) - 1);
        quest.reward.xpReward = 200;
        quest.reward.goldReward = 100;
        quest.reward.itemId = 5; // Iron Helmet
        quest.reward.itemQuantity = 1;
        registerQuest(quest);
    }

    std::cout << "[QUESTS] Initialized " << quests_.size() << " quest definitions" << std::endl;
}

} // namespace DarkAges
