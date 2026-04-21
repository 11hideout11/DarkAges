#include <catch2/catch_test_macros.hpp>
#include "combat/DialogueSystem.hpp"
#include "combat/QuestSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <cstring>

using namespace DarkAges;

// Helper to build a simple dialogue tree
static DialogueTree buildSimpleTree() {
    DialogueTree tree;
    tree.treeId = 1;
    std::strncpy(tree.npcName, "Elder Oakheart", NPC_NAME_MAX_LEN);
    tree.greetingNodeId = 1;

    // Node 1: Greeting
    tree.nodes[0].nodeId = 1;
    std::strncpy(tree.nodes[0].text, "Greetings, adventurer. The forest is dangerous these days.", DIALOGUE_TEXT_MAX_LEN);
    tree.nodes[0].responseCount = 2;

    // Response 1: Ask about quest
    std::strncpy(tree.nodes[0].responses[0].text, "What can I do to help?", 128);
    tree.nodes[0].responses[0].nextNodeId = 2;

    // Response 2: Leave
    std::strncpy(tree.nodes[0].responses[1].text, "I must go now.", 128);
    tree.nodes[0].responses[1].nextNodeId = 0; // End conversation

    // Node 2: Quest offer
    tree.nodes[1].nodeId = 2;
    std::strncpy(tree.nodes[1].text, "The wolves have been attacking travelers. Can you thin their numbers?", DIALOGUE_TEXT_MAX_LEN);
    tree.nodes[1].responseCount = 2;

    std::strncpy(tree.nodes[1].responses[0].text, "I accept!", 128);
    tree.nodes[1].responses[0].nextNodeId = 3;
    tree.nodes[1].responses[0].actions[0].type = DialogueActionType::GiveQuest;
    tree.nodes[1].responses[0].actions[0].targetId = 100;
    tree.nodes[1].responses[0].actionCount = 1;

    std::strncpy(tree.nodes[1].responses[1].text, "Not right now.", 128);
    tree.nodes[1].responses[1].nextNodeId = 1;

    // Node 3: Quest accepted confirmation
    tree.nodes[2].nodeId = 3;
    std::strncpy(tree.nodes[2].text, "Thank you! Kill 5 wolves and return to me.", DIALOGUE_TEXT_MAX_LEN);
    tree.nodes[2].isEnd = true;
    tree.nodeCount = 3;

    return tree;
}

// Helper to build a conditional dialogue tree
static DialogueTree buildConditionalTree() {
    DialogueTree tree;
    tree.treeId = 2;
    std::strncpy(tree.npcName, "Guard Captain", NPC_NAME_MAX_LEN);
    tree.greetingNodeId = 1;

    // Node 1: Greeting (shows different text based on quest state)
    tree.nodes[0].nodeId = 1;
    std::strncpy(tree.nodes[0].text, "Halt! The town is on high alert.", DIALOGUE_TEXT_MAX_LEN);
    tree.nodes[0].responseCount = 2;

    // Response: "I have news about the wolves" (only if quest 100 is complete)
    std::strncpy(tree.nodes[0].responses[0].text, "I've dealt with the wolves.", 128);
    tree.nodes[0].responses[0].nextNodeId = 2;
    tree.nodes[0].responses[0].conditions[0].type = DialogueConditionType::QuestComplete;
    tree.nodes[0].responses[0].conditions[0].targetId = 100;
    tree.nodes[0].responses[0].conditionCount = 1;

    // Response: "Goodbye" (always available)
    std::strncpy(tree.nodes[0].responses[1].text, "Goodbye.", 128);
    tree.nodes[0].responses[1].nextNodeId = 0;

    // Node 2: Quest completion dialogue
    tree.nodes[1].nodeId = 2;
    std::strncpy(tree.nodes[1].text, "Excellent work! Here is your reward.", DIALOGUE_TEXT_MAX_LEN);
    tree.nodes[1].responseCount = 1;
    std::strncpy(tree.nodes[1].responses[0].text, "Thank you.", 128);
    tree.nodes[1].responses[0].nextNodeId = 0;
    tree.nodes[1].responses[0].actions[0].type = DialogueActionType::CompleteQuest;
    tree.nodes[1].responses[0].actions[0].targetId = 100;
    tree.nodes[1].responses[0].actionCount = 1;
    tree.nodes[1].isEnd = true;

    tree.nodeCount = 2;
    return tree;
}

// ============================================================================
// Type Construction Tests
// ============================================================================

TEST_CASE("DialogueCondition default construction", "[dialogue]") {
    DialogueCondition cond;
    CHECK(cond.type == DialogueConditionType::None);
    CHECK(cond.targetId == 0);
    CHECK(cond.quantity == 1);
}

TEST_CASE("DialogueAction default construction", "[dialogue]") {
    DialogueAction action;
    CHECK(action.type == DialogueActionType::None);
    CHECK(action.targetId == 0);
    CHECK(action.quantity == 1);
}

TEST_CASE("DialogueResponse default construction", "[dialogue]") {
    DialogueResponse resp;
    CHECK(resp.text[0] == '\0');
    CHECK(resp.nextNodeId == 0);
    CHECK(resp.conditionCount == 0);
    CHECK(resp.actionCount == 0);
}

TEST_CASE("DialogueNode default construction", "[dialogue]") {
    DialogueNode node;
    CHECK(node.nodeId == 0);
    CHECK(node.text[0] == '\0');
    CHECK(node.responseCount == 0);
    CHECK(node.conditionCount == 0);
    CHECK(node.isEnd == false);
}

TEST_CASE("DialogueTree default construction", "[dialogue]") {
    DialogueTree tree;
    CHECK(tree.treeId == 0);
    CHECK(tree.npcName[0] == '\0');
    CHECK(tree.nodeCount == 0);
    CHECK(tree.greetingNodeId == 0);
}

TEST_CASE("DialogueComponent default construction", "[dialogue]") {
    DialogueComponent dc;
    CHECK(dc.activeTreeId == 0);
    CHECK(dc.currentNodeId == 0);
    CHECK(dc.npcEntity == static_cast<EntityID>(entt::null));
    CHECK(dc.inConversation == false);
}

TEST_CASE("DialogueComponent start and end conversation", "[dialogue]") {
    DialogueComponent dc;
    dc.startConversation(5, 10, entt::entity{42});
    CHECK(dc.activeTreeId == 5);
    CHECK(dc.currentNodeId == 10);
    CHECK(dc.npcEntity == entt::entity{42});
    CHECK(dc.inConversation == true);

    dc.endConversation();
    CHECK(dc.activeTreeId == 0);
    CHECK(dc.currentNodeId == 0);
    CHECK(dc.npcEntity == static_cast<EntityID>(entt::null));
    CHECK(dc.inConversation == false);
}

// ============================================================================
// Enum Tests
// ============================================================================

TEST_CASE("DialogueConditionType enum values", "[dialogue]") {
    CHECK(static_cast<uint8_t>(DialogueConditionType::None) == 0);
    CHECK(static_cast<uint8_t>(DialogueConditionType::HasQuest) == 1);
    CHECK(static_cast<uint8_t>(DialogueConditionType::QuestComplete) == 2);
    CHECK(static_cast<uint8_t>(DialogueConditionType::QuestNotStarted) == 3);
    CHECK(static_cast<uint8_t>(DialogueConditionType::MinLevel) == 4);
    CHECK(static_cast<uint8_t>(DialogueConditionType::HasItem) == 5);
    CHECK(static_cast<uint8_t>(DialogueConditionType::NoCondition) == 6);
}

TEST_CASE("DialogueActionType enum values", "[dialogue]") {
    CHECK(static_cast<uint8_t>(DialogueActionType::None) == 0);
    CHECK(static_cast<uint8_t>(DialogueActionType::GiveQuest) == 1);
    CHECK(static_cast<uint8_t>(DialogueActionType::CompleteQuest) == 2);
    CHECK(static_cast<uint8_t>(DialogueActionType::GiveItem) == 3);
    CHECK(static_cast<uint8_t>(DialogueActionType::TakeItem) == 4);
    CHECK(static_cast<uint8_t>(DialogueActionType::GiveGold) == 5);
    CHECK(static_cast<uint8_t>(DialogueActionType::CloseDialogue) == 6);
}

// ============================================================================
// Tree Registry Tests
// ============================================================================

TEST_CASE("DialogueSystem register and lookup tree", "[dialogue]") {
    DialogueSystem ds;
    DialogueTree tree = buildSimpleTree();

    ds.registerTree(tree);

    const DialogueTree* found = ds.getTree(1);
    REQUIRE(found != nullptr);
    CHECK(found->treeId == 1);
    CHECK(std::strcmp(found->npcName, "Elder Oakheart") == 0);
    CHECK(found->nodeCount == 3);
}

TEST_CASE("DialogueSystem getTree returns null for unregistered", "[dialogue]") {
    DialogueSystem ds;
    CHECK(ds.getTree(0) == nullptr);
    CHECK(ds.getTree(999) == nullptr);
}

TEST_CASE("DialogueSystem registerTree ignores invalid ID", "[dialogue]") {
    DialogueSystem ds;
    DialogueTree tree;
    tree.treeId = 0;
    ds.registerTree(tree);
    CHECK(ds.getTree(0) == nullptr);
}

TEST_CASE("DialogueSystem NPC tree mapping", "[dialogue]") {
    DialogueSystem ds;
    ds.setNPCTree(entt::entity{10}, 5);
    ds.setNPCTree(entt::entity{20}, 7);

    CHECK(ds.getNPCTreeId(entt::entity{10}) == 5);
    CHECK(ds.getNPCTreeId(entt::entity{20}) == 7);
    CHECK(ds.getNPCTreeId(entt::entity{30}) == 0);
}

// ============================================================================
// Condition Evaluation Tests
// ============================================================================

TEST_CASE("DialogueSystem evaluate None condition", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueCondition cond;
    cond.type = DialogueConditionType::None;

    auto player = registry.create();
    CHECK(ds.evaluateCondition(registry, player, cond));
}

TEST_CASE("DialogueSystem evaluate NoCondition", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueCondition cond;
    cond.type = DialogueConditionType::NoCondition;

    auto player = registry.create();
    CHECK(ds.evaluateCondition(registry, player, cond));
}

TEST_CASE("DialogueSystem evaluate HasQuest condition", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;
    QuestSystem qs;
    ds.setQuestSystem(&qs);

    auto player = registry.create();
    registry.emplace<QuestLog>(player);

    DialogueCondition cond;
    cond.type = DialogueConditionType::HasQuest;
    cond.targetId = 100;

    // No active quest yet
    CHECK_FALSE(ds.evaluateCondition(registry, player, cond));

    // Add quest to log manually
    QuestLog& log = registry.get<QuestLog>(player);
    log.addActiveQuest(100, 0);
    CHECK(ds.evaluateCondition(registry, player, cond));
}

TEST_CASE("DialogueSystem evaluate QuestNotStarted condition", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();

    DialogueCondition cond;
    cond.type = DialogueConditionType::QuestNotStarted;
    cond.targetId = 100;

    // No quest log = not started
    CHECK(ds.evaluateCondition(registry, player, cond));

    // Add quest log with active quest
    registry.emplace<QuestLog>(player);
    QuestLog& log = registry.get<QuestLog>(player);
    log.addActiveQuest(100, 0);
    CHECK_FALSE(ds.evaluateCondition(registry, player, cond));
}

TEST_CASE("DialogueSystem evaluate MinLevel condition", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();

    DialogueCondition cond;
    cond.type = DialogueConditionType::MinLevel;
    cond.targetId = 5;

    // No progression = fails
    CHECK_FALSE(ds.evaluateCondition(registry, player, cond));

    // Level 3 = fails (need 5)
    registry.emplace<PlayerProgression>(player);
    registry.get<PlayerProgression>(player).level = 3;
    CHECK_FALSE(ds.evaluateCondition(registry, player, cond));

    // Level 5 = passes
    registry.get<PlayerProgression>(player).level = 5;
    CHECK(ds.evaluateCondition(registry, player, cond));

    // Level 10 = passes
    registry.get<PlayerProgression>(player).level = 10;
    CHECK(ds.evaluateCondition(registry, player, cond));
}

TEST_CASE("DialogueSystem evaluate multiple conditions (AND logic)", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);
    registry.get<PlayerProgression>(player).level = 10;
    registry.emplace<QuestLog>(player);

    DialogueCondition conditions[2];
    conditions[0].type = DialogueConditionType::MinLevel;
    conditions[0].targetId = 5;
    conditions[1].type = DialogueConditionType::HasQuest;
    conditions[1].targetId = 200;

    // Level passes, but no quest
    CHECK_FALSE(ds.evaluateConditions(registry, player, conditions, 2));

    // Add quest
    registry.get<QuestLog>(player).addActiveQuest(200, 0);
    CHECK(ds.evaluateConditions(registry, player, conditions, 2));

    // Zero conditions = always true
    CHECK(ds.evaluateConditions(registry, player, conditions, 0));
}

// ============================================================================
// Conversation Lifecycle Tests
// ============================================================================

TEST_CASE("DialogueSystem startConversation basic", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    const char* text = ds.startConversation(registry, player, npc);
    REQUIRE(text != nullptr);
    CHECK(std::strcmp(text, "Greetings, adventurer. The forest is dangerous these days.") == 0);

    // Player should have dialogue component
    REQUIRE(registry.all_of<DialogueComponent>(player));
    const DialogueComponent& dc = registry.get<DialogueComponent>(player);
    CHECK(dc.inConversation == true);
    CHECK(dc.activeTreeId == 1);
    CHECK(dc.currentNodeId == 1);
    CHECK(dc.npcEntity == npc);
}

TEST_CASE("DialogueSystem startConversation fails with no tree", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();
    auto npc = registry.create();

    const char* text = ds.startConversation(registry, player, npc);
    CHECK(text == nullptr);
}

TEST_CASE("DialogueSystem selectResponse navigates tree", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);

    // Select "What can I do to help?" (index 0)
    const char* text = ds.selectResponse(registry, player, 0);
    REQUIRE(text != nullptr);
    CHECK(std::strcmp(text, "The wolves have been attacking travelers. Can you thin their numbers?") == 0);

    // Current node should now be node 2
    const DialogueComponent& dc = registry.get<DialogueComponent>(player);
    CHECK(dc.currentNodeId == 2);
}

TEST_CASE("DialogueSystem selectResponse ends conversation", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);

    // Select "I must go now." (index 1) — should end conversation
    const char* text = ds.selectResponse(registry, player, 1);
    CHECK(text == nullptr);

    // Conversation should be ended
    const DialogueComponent& dc = registry.get<DialogueComponent>(player);
    CHECK(dc.inConversation == false);
}

TEST_CASE("DialogueSystem selectResponse handles quest action", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;
    QuestSystem qs;

    // Register a quest
    QuestDefinition questDef;
    questDef.questId = 100;
    std::strncpy(questDef.name, "Wolf Hunt", 48);
    questDef.objectiveCount = 1;
    questDef.objectives[0].type = QuestObjectiveType::KillNPC;
    questDef.objectives[0].targetId = 1;
    questDef.objectives[0].requiredCount = 5;
    qs.registerQuest(questDef);
    ds.setQuestSystem(&qs);

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    registry.emplace<QuestLog>(player);
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);

    // Navigate to quest offer: "What can I do to help?"
    ds.selectResponse(registry, player, 0);

    // Now select "I accept!" (index 0 on node 2)
    const char* text = ds.selectResponse(registry, player, 0);
    REQUIRE(text != nullptr);
    CHECK(std::strcmp(text, "Thank you! Kill 5 wolves and return to me.") == 0);

    // Quest should be active
    QuestLog& log = registry.get<QuestLog>(player);
    CHECK(log.hasActiveQuest(100));
}

TEST_CASE("DialogueSystem endConversation cleanup", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);
    REQUIRE(registry.get<DialogueComponent>(player).inConversation == true);

    ds.endConversation(registry, player);
    CHECK(registry.get<DialogueComponent>(player).inConversation == false);
}

// ============================================================================
// Response Filtering Tests
// ============================================================================

TEST_CASE("DialogueSystem getAvailableResponses filters by condition", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;
    QuestSystem qs;
    ds.setQuestSystem(&qs);

    DialogueTree tree = buildConditionalTree();
    ds.registerTree(tree);

    auto player = registry.create();
    registry.emplace<QuestLog>(player);
    auto npc = registry.create();
    ds.setNPCTree(npc, 2);

    ds.startConversation(registry, player, npc);

    // Without quest 100 complete, only "Goodbye" should be available
    auto responses = ds.getAvailableResponses(registry, player);
    CHECK(responses.size() == 1);
    CHECK(responses[0] == 1); // Index 1 is "Goodbye"

    // Complete quest 100
    registry.get<QuestLog>(player).addActiveQuest(100, 0);
    // Simulate completion by removing from active and adding to completed
    registry.get<QuestLog>(player).completeQuest(100);

    // Now both responses should be available
    responses = ds.getAvailableResponses(registry, player);
    CHECK(responses.size() == 2);
}

// ============================================================================
// Action Execution Tests
// ============================================================================

TEST_CASE("DialogueSystem executeAction GiveGold", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();

    DialogueAction action;
    action.type = DialogueActionType::GiveGold;
    action.quantity = 100;

    ds.executeAction(registry, player, action);

    REQUIRE(registry.all_of<Inventory>(player));
    CHECK(registry.get<Inventory>(player).gold == 100.0f);
}

TEST_CASE("DialogueSystem executeAction CloseDialogue", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);
    REQUIRE(registry.get<DialogueComponent>(player).inConversation == true);

    DialogueAction action;
    action.type = DialogueActionType::CloseDialogue;
    ds.executeAction(registry, player, action);

    CHECK(registry.get<DialogueComponent>(player).inConversation == false);
}

TEST_CASE("DialogueSystem executeActions multiple", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();

    DialogueAction actions[2];
    actions[0].type = DialogueActionType::GiveGold;
    actions[0].quantity = 50;
    actions[1].type = DialogueActionType::GiveGold;
    actions[1].quantity = 25;

    ds.executeActions(registry, player, actions, 2);

    CHECK(registry.get<Inventory>(player).gold == 75.0f);
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_CASE("DialogueSystem dialogue text callback fires", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    bool callbackFired = false;
    std::string receivedName;
    std::string receivedText;
    bool receivedIsEnd = false;

    ds.setDialogueTextCallback([&](EntityID player, const char* npcName,
                                    const char* text, bool isEnd) {
        callbackFired = true;
        receivedName = npcName;
        receivedText = text;
        receivedIsEnd = isEnd;
    });

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);

    CHECK(callbackFired);
    CHECK(receivedName == "Elder Oakheart");
    CHECK(receivedText == "Greetings, adventurer. The forest is dangerous these days.");
    CHECK_FALSE(receivedIsEnd);
}

TEST_CASE("DialogueSystem end node fires callback with isEnd", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    bool endFired = false;
    ds.setDialogueTextCallback([&](EntityID, const char*, const char*, bool isEnd) {
        if (isEnd) endFired = true;
    });

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);
    // Navigate to quest offer
    ds.selectResponse(registry, player, 0);
    // Accept quest -> node 3 (isEnd=true)
    ds.selectResponse(registry, player, 0);

    CHECK(endFired);
}

// ============================================================================
// GetCurrentNode Tests
// ============================================================================

TEST_CASE("DialogueSystem getCurrentNode returns correct node", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    // Before conversation
    CHECK(ds.getCurrentNode(registry, player) == nullptr);

    // Start conversation
    ds.startConversation(registry, player, npc);
    const DialogueNode* node = ds.getCurrentNode(registry, player);
    REQUIRE(node != nullptr);
    CHECK(node->nodeId == 1);

    // Navigate
    ds.selectResponse(registry, player, 0);
    node = ds.getCurrentNode(registry, player);
    REQUIRE(node != nullptr);
    CHECK(node->nodeId == 2);

    // End conversation
    ds.endConversation(registry, player);
    CHECK(ds.getCurrentNode(registry, player) == nullptr);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("DialogueSystem selectResponse with no active conversation", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    auto player = registry.create();
    const char* text = ds.selectResponse(registry, player, 0);
    CHECK(text == nullptr);
}

TEST_CASE("DialogueSystem selectResponse with invalid index", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player, npc);

    // Out of range response index — should end conversation
    const char* text = ds.selectResponse(registry, player, 99);
    CHECK(text == nullptr);
    CHECK_FALSE(registry.get<DialogueComponent>(player).inConversation);
}

TEST_CASE("DialogueSystem multiple players independent conversations", "[dialogue]") {
    entt::registry registry;
    DialogueSystem ds;

    DialogueTree tree = buildSimpleTree();
    ds.registerTree(tree);

    auto player1 = registry.create();
    auto player2 = registry.create();
    auto npc = registry.create();
    ds.setNPCTree(npc, 1);

    ds.startConversation(registry, player1, npc);
    ds.startConversation(registry, player2, npc);

    // Both should be at node 1
    CHECK(registry.get<DialogueComponent>(player1).currentNodeId == 1);
    CHECK(registry.get<DialogueComponent>(player2).currentNodeId == 1);

    // Player 1 navigates
    ds.selectResponse(registry, player1, 0);

    // Player 1 at node 2, player 2 still at node 1
    CHECK(registry.get<DialogueComponent>(player1).currentNodeId == 2);
    CHECK(registry.get<DialogueComponent>(player2).currentNodeId == 1);

    // Player 2 leaves
    ds.selectResponse(registry, player2, 1);
    CHECK_FALSE(registry.get<DialogueComponent>(player2).inConversation);
    CHECK(registry.get<DialogueComponent>(player1).inConversation == true);
}

TEST_CASE("DialogueTree findNode lookup", "[dialogue]") {
    DialogueTree tree = buildSimpleTree();

    const DialogueNode* node1 = tree.findNode(1);
    REQUIRE(node1 != nullptr);
    CHECK(node1->nodeId == 1);

    const DialogueNode* node3 = tree.findNode(3);
    REQUIRE(node3 != nullptr);
    CHECK(node3->nodeId == 3);

    CHECK(tree.findNode(0) == nullptr);
    CHECK(tree.findNode(999) == nullptr);
}
