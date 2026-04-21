#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// NPC Dialogue System — manages conversation trees for NPCs.
// Handles dialogue flow, condition checking, action execution, and quest integration.
// Players interact with NPCs to start conversations, choose responses, and trigger actions.

namespace DarkAges {

class QuestSystem;
class ItemSystem;
class ChatSystem;

class DialogueSystem {
public:
    DialogueSystem() = default;

    // --- Dialogue Tree Registry ---

    // Register a dialogue tree for an NPC
    void registerTree(const DialogueTree& tree);

    // Get a dialogue tree by ID
    const DialogueTree* getTree(uint32_t treeId) const;

    // Set the dialogue tree ID for a specific NPC entity
    void setNPCTree(EntityID npc, uint32_t treeId);

    // Get the dialogue tree ID for an NPC entity (0 if none)
    uint32_t getNPCTreeId(EntityID npc) const;

    // --- Conversation Lifecycle ---

    // Start a conversation between a player and an NPC.
    // Returns the greeting node text, or empty string if conversation can't start.
    const char* startConversation(Registry& registry, EntityID player, EntityID npc);

    // Select a response in an active conversation.
    // Returns the next node's text, or empty string if conversation ended.
    const char* selectResponse(Registry& registry, EntityID player, uint32_t responseIndex);

    // End a conversation
    void endConversation(Registry& registry, EntityID player);

    // Get the current node for a player's active conversation
    const DialogueNode* getCurrentNode(const Registry& registry, EntityID player) const;

    // Get available response indices for current node (filtered by conditions)
    std::vector<uint32_t> getAvailableResponses(const Registry& registry, EntityID player) const;

    // --- Condition Evaluation ---

    // Check if a dialogue condition is met for a player
    bool evaluateCondition(const Registry& registry, EntityID player,
                           const DialogueCondition& condition) const;

    // Check if all conditions in an array are met
    bool evaluateConditions(const Registry& registry, EntityID player,
                            const DialogueCondition* conditions, uint32_t count) const;

    // --- Action Execution ---

    // Execute a dialogue action for a player
    void executeAction(Registry& registry, EntityID player, const DialogueAction& action);

    // Execute all actions in an array
    void executeActions(Registry& registry, EntityID player,
                        const DialogueAction* actions, uint32_t count);

    // --- Callbacks ---

    // Callback for dialogue text delivery (to send to client)
    using DialogueTextCallback = std::function<void(EntityID player, const char* npcName,
                                                     const char* text, bool isEnd)>;
    void setDialogueTextCallback(DialogueTextCallback cb) { dialogueTextCallback_ = std::move(cb); }

    // Callback for response delivery (to send available options to client)
    using DialogueResponsesCallback = std::function<void(EntityID player,
                                                          const uint32_t* indices,
                                                          const char* const* texts,
                                                          uint32_t count)>;
    void setResponsesCallback(DialogueResponsesCallback cb) { responsesCallback_ = std::move(cb); }

    // --- System Dependencies ---

    void setQuestSystem(QuestSystem* qs) { questSystem_ = qs; }
    void setItemSystem(ItemSystem* is) { itemSystem_ = is; }
    void setChatSystem(ChatSystem* cs) { chatSystem_ = cs; }

private:
    // Tree registry — indexed by treeId
    std::vector<DialogueTree> trees_;

    // NPC entity -> tree ID mapping
    std::unordered_map<EntityID, uint32_t> npcTreeMap_;

    // System references (not owned)
    QuestSystem* questSystem_{nullptr};
    ItemSystem* itemSystem_{nullptr};
    ChatSystem* chatSystem_{nullptr};

    // Callbacks
    DialogueTextCallback dialogueTextCallback_;
    DialogueResponsesCallback responsesCallback_;

    // Find the best starting node for a player based on conditions
    const DialogueNode* findBestGreeting(const Registry& registry, EntityID player,
                                         const DialogueTree& tree) const;
};

} // namespace DarkAges
