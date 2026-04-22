#include "combat/DialogueSystem.hpp"
#include "combat/QuestSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "combat/ChatSystem.hpp"
#include <cstring>
#include <iostream>

namespace DarkAges {

// ============================================================================
// Dialogue Tree Registry
// ============================================================================

void DialogueSystem::registerTree(const DialogueTree& tree) {
    if (tree.treeId == 0) return;

    // Expand registry if needed
    if (tree.treeId >= trees_.size()) {
        trees_.resize(tree.treeId + 1);
    }

    trees_[tree.treeId] = tree;
}

const DialogueTree* DialogueSystem::getTree(uint32_t treeId) const {
    if (treeId == 0 || treeId >= trees_.size()) return nullptr;
    const auto& tree = trees_[treeId];
    if (tree.treeId == 0) return nullptr;
    return &tree;
}

void DialogueSystem::setNPCTree(EntityID npc, uint32_t treeId) {
    npcTreeMap_[npc] = treeId;
}

uint32_t DialogueSystem::getNPCTreeId(EntityID npc) const {
    auto it = npcTreeMap_.find(npc);
    return (it != npcTreeMap_.end()) ? it->second : 0;
}

// ============================================================================
// Condition Evaluation
// ============================================================================

bool DialogueSystem::evaluateCondition(const Registry& registry, EntityID player,
                                        const DialogueCondition& condition) const {
    switch (condition.type) {
        case DialogueConditionType::None:
        case DialogueConditionType::NoCondition:
            return true;

        case DialogueConditionType::HasQuest: {
            const QuestLog* log = registry.try_get<QuestLog>(player);
            return log && log->hasActiveQuest(condition.targetId);
        }

        case DialogueConditionType::QuestComplete: {
            const QuestLog* log = registry.try_get<QuestLog>(player);
            return log && log->hasCompletedQuest(condition.targetId);
        }

        case DialogueConditionType::QuestNotStarted: {
            const QuestLog* log = registry.try_get<QuestLog>(player);
            if (!log) return true; // No quest log = hasn't started anything
            return !log->hasActiveQuest(condition.targetId) &&
                   !log->hasCompletedQuest(condition.targetId);
        }

        case DialogueConditionType::MinLevel: {
            const PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
            return prog && prog->level >= condition.targetId;
        }

        case DialogueConditionType::HasItem: {
            // Delegate to ItemSystem if available
            if (itemSystem_) {
                // Check inventory for the item
                const Inventory* inv = registry.try_get<Inventory>(player);
                if (!inv) return false;
                uint32_t count = 0;
                for (uint32_t i = 0; i < inv->slotCount; ++i) {
                    if (inv->slots[i].itemId == condition.targetId) {
                        count += inv->slots[i].quantity;
                    }
                }
                return count >= condition.quantity;
            }
            return false;
        }
    }
    return false;
}

bool DialogueSystem::evaluateConditions(const Registry& registry, EntityID player,
                                         const DialogueCondition* conditions,
                                         uint32_t count) const {
    for (uint32_t i = 0; i < count; ++i) {
        if (!evaluateCondition(registry, player, conditions[i])) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// Action Execution
// ============================================================================

void DialogueSystem::executeAction(Registry& registry, EntityID player,
                                    const DialogueAction& action) {
    switch (action.type) {
        case DialogueActionType::None:
            break;

        case DialogueActionType::GiveQuest: {
            if (questSystem_) {
                questSystem_->acceptQuest(registry, player, action.targetId, 0);
            }
            break;
        }

        case DialogueActionType::CompleteQuest: {
            if (questSystem_) {
                questSystem_->completeQuest(registry, player, action.targetId);
            }
            break;
        }

        case DialogueActionType::GiveItem: {
            if (itemSystem_) {
                itemSystem_->addToInventory(registry, player, action.targetId, action.quantity);
            }
            break;
        }

        case DialogueActionType::TakeItem: {
            if (itemSystem_) {
                itemSystem_->removeFromInventory(registry, player, action.targetId, action.quantity);
            }
            break;
        }

        case DialogueActionType::GiveGold: {
            if (!registry.all_of<Inventory>(player)) {
                registry.emplace<Inventory>(player);
            }
            Inventory& inv = registry.get<Inventory>(player);
            inv.gold += static_cast<float>(action.quantity);
            break;
        }

        case DialogueActionType::CloseDialogue: {
            endConversation(registry, player);
            break;
        }
    }
}

void DialogueSystem::executeActions(Registry& registry, EntityID player,
                                     const DialogueAction* actions, uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        executeAction(registry, player, actions[i]);
    }
}

// ============================================================================
// Conversation Lifecycle
// ============================================================================

const DialogueNode* DialogueSystem::findBestGreeting(const Registry& registry, EntityID player,
                                                       const DialogueTree& tree) const {
    // Try the default greeting first
    if (tree.nodeCount == 0) return nullptr;

    // Find the greeting node
    const DialogueNode* greeting = tree.findNode(tree.greetingNodeId);
    if (!greeting && tree.nodeCount > 0) {
        greeting = &tree.nodes[0]; // Fallback to first node
    }
    return greeting;
}

const char* DialogueSystem::startConversation(Registry& registry, EntityID player,
                                                EntityID npc) {
    // Get tree ID for this NPC (must be explicitly assigned via setNPCTree)
    uint32_t treeId = getNPCTreeId(npc);
    if (treeId == 0) {
        return nullptr;
    }

    const DialogueTree* tree = getTree(treeId);
    if (!tree) return nullptr;

    // Find the appropriate greeting node
    const DialogueNode* greeting = findBestGreeting(registry, player, *tree);
    if (!greeting) return nullptr;

    // Initialize player's dialogue component
    if (!registry.all_of<DialogueComponent>(player)) {
        registry.emplace<DialogueComponent>(player);
    }
    DialogueComponent& dc = registry.get<DialogueComponent>(player);
    dc.startConversation(treeId, greeting->nodeId, npc);

    // Notify via callback
    if (dialogueTextCallback_) {
        dialogueTextCallback_(player, tree->npcName, greeting->text, greeting->isEnd);
    }

    // Send available responses
    auto available = getAvailableResponses(registry, player);
    if (responsesCallback_ && !available.empty()) {
        std::vector<const char*> texts;
        for (uint32_t idx : available) {
            if (idx < greeting->responseCount) {
                texts.push_back(greeting->responses[idx].text);
            }
        }
        responsesCallback_(player, available.data(), texts.data(),
                           static_cast<uint32_t>(available.size()));
    }

    return greeting->text;
}

const char* DialogueSystem::selectResponse(Registry& registry, EntityID player,
                                             uint32_t responseIndex) {
    // Get player's dialogue state
    if (!registry.all_of<DialogueComponent>(player)) return nullptr;

    DialogueComponent& dc = registry.get<DialogueComponent>(player);
    if (!dc.inConversation) return nullptr;

    // Get the current tree and node
    const DialogueTree* tree = getTree(dc.activeTreeId);
    if (!tree) {
        endConversation(registry, player);
        return nullptr;
    }

    const DialogueNode* currentNode = tree->findNode(dc.currentNodeId);
    if (!currentNode || responseIndex >= currentNode->responseCount) {
        endConversation(registry, player);
        return nullptr;
    }

    const DialogueResponse& response = currentNode->responses[responseIndex];

    // Execute actions on the response
    executeActions(registry, player, response.actions, response.actionCount);

    // Check if conversation should end
    if (response.nextNodeId == 0 || dc.inConversation == false) {
        endConversation(registry, player);
        return nullptr;
    }

    // Transition to next node
    const DialogueNode* nextNode = tree->findNode(response.nextNodeId);
    if (!nextNode) {
        endConversation(registry, player);
        return nullptr;
    }

    dc.currentNodeId = nextNode->nodeId;

    // Notify via callback
    if (dialogueTextCallback_) {
        dialogueTextCallback_(player, tree->npcName, nextNode->text, nextNode->isEnd);
    }

    // Send available responses for the new node
    auto available = getAvailableResponses(registry, player);
    if (responsesCallback_ && !available.empty()) {
        std::vector<const char*> texts;
        for (uint32_t idx : available) {
            if (idx < nextNode->responseCount) {
                texts.push_back(nextNode->responses[idx].text);
            }
        }
        responsesCallback_(player, available.data(), texts.data(),
                           static_cast<uint32_t>(available.size()));
    }

    // If this is an end node, close after delivering text
    if (nextNode->isEnd) {
        endConversation(registry, player);
    }

    return nextNode->text;
}

void DialogueSystem::endConversation(Registry& registry, EntityID player) {
    if (registry.all_of<DialogueComponent>(player)) {
        registry.get<DialogueComponent>(player).endConversation();
    }
}

const DialogueNode* DialogueSystem::getCurrentNode(const Registry& registry,
                                                     EntityID player) const {
    if (!registry.all_of<DialogueComponent>(player)) return nullptr;

    const DialogueComponent& dc = registry.get<DialogueComponent>(player);
    if (!dc.inConversation) return nullptr;

    const DialogueTree* tree = getTree(dc.activeTreeId);
    if (!tree) return nullptr;

    return tree->findNode(dc.currentNodeId);
}

std::vector<uint32_t> DialogueSystem::getAvailableResponses(const Registry& registry,
                                                              EntityID player) const {
    std::vector<uint32_t> result;

    const DialogueNode* node = getCurrentNode(registry, player);
    if (!node) return result;

    for (uint32_t i = 0; i < node->responseCount; ++i) {
        if (evaluateConditions(registry, player,
                               node->responses[i].conditions,
                               node->responses[i].conditionCount)) {
            result.push_back(i);
        }
    }

    return result;
}

} // namespace DarkAges
