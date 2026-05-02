// [COMBAT_AGENT] Unit tests for Quest System

#include <catch2/catch_test_macros.hpp>
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <string>
#include <vector>

using namespace DarkAges;

// ============================================================================
// Quest Objective Tests
// ============================================================================

TEST_CASE("QuestObjectiveType enum values", "[quest]") {
    REQUIRE(static_cast<uint8_t>(QuestObjectiveType::KillNPC) == 0);
    REQUIRE(static_cast<uint8_t>(QuestObjectiveType::CollectItem) == 1);
    REQUIRE(static_cast<uint8_t>(QuestObjectiveType::TalkToNPC) == 2);
    REQUIRE(static_cast<uint8_t>(QuestObjectiveType::ReachLevel) == 3);
    REQUIRE(static_cast<uint8_t>(QuestObjectiveType::ExploreZone) == 4);
}

TEST_CASE("QuestObjective validation", "[quest]") {
    SECTION("Valid kill objective") {
        QuestObjective obj;
        obj.type = QuestObjectiveType::KillNPC;
        obj.targetId = 2001;
        obj.requiredCount = 10;
        // Basic validation - must have target ID and count > 0
        REQUIRE(obj.targetId == 2001);
        REQUIRE(obj.requiredCount == 10);
    }

    SECTION("Valid collect objective") {
        QuestObjective obj;
        obj.type = QuestObjectiveType::CollectItem;
        obj.targetId = 5001;  // Iron Ore
        obj.requiredCount = 5;
        REQUIRE(obj.targetId == 5001);
        REQUIRE(obj.requiredCount == 5);
    }
}

// ============================================================================
// Quest Definition Tests
// ============================================================================

TEST_CASE("QuestDefinition basic fields", "[quest]") {
    QuestDefinition quest;
    quest.questId = 10001;
    std::strncpy(quest.name, "Test Quest", 48);
    std::strncpy(quest.description, "A test quest description", 128);
    quest.requiredLevel = 5;
    quest.prerequisiteQuestId = 0;

    REQUIRE(quest.questId == 10001);
    REQUIRE(quest.requiredLevel == 5);
    REQUIRE(quest.prerequisiteQuestId == 0);
}

TEST_CASE("QuestDefinition with objectives", "[quest]") {
    QuestDefinition quest;
    quest.questId = 10002;
    quest.objectiveCount = 2;
    
    // Add kill objective
    quest.objectives[0].type = QuestObjectiveType::KillNPC;
    quest.objectives[0].targetId = 2001;
    quest.objectives[0].requiredCount = 10;
    
    // Add collect objective
    quest.objectives[1].type = QuestObjectiveType::CollectItem;
    quest.objectives[1].targetId = 5001;
    quest.objectives[1].requiredCount = 5;

    REQUIRE(quest.objectiveCount == 2);
    REQUIRE(quest.objectives[0].type == QuestObjectiveType::KillNPC);
    REQUIRE(quest.objectives[1].type == QuestObjectiveType::CollectItem);
}

TEST_CASE("QuestDefinition with reward", "[quest]") {
    QuestDefinition quest;
    quest.questId = 10003;
    quest.reward.xpReward = 500;
    quest.reward.goldReward = 250;
    quest.reward.itemId = 1001;
    quest.reward.itemQuantity = 1;

    REQUIRE(quest.reward.xpReward == 500);
    REQUIRE(quest.reward.goldReward == 250);
    REQUIRE(quest.reward.itemId == 1001);
    REQUIRE(quest.reward.itemQuantity == 1);
}

// ============================================================================
// Quest Progress Tests
// ============================================================================

TEST_CASE("QuestProgress initialization", "[quest]") {
    QuestProgress progress;
    progress.questId = 10001;
    progress.accepted = false;
    progress.completed = false;

    REQUIRE(progress.questId == 10001);
    REQUIRE(progress.accepted == false);
    REQUIRE(progress.completed == false);
}

TEST_CASE("ObjectiveProgress tracking", "[quest]") {
    SECTION("Initial state is not complete") {
        ObjectiveProgress progress;
        REQUIRE(progress.completed == false);
        REQUIRE(progress.currentCount == 0);
    }

    SECTION("Update progress") {
        ObjectiveProgress progress;
        progress.currentCount = 5;
        REQUIRE(progress.currentCount == 5);
        REQUIRE(progress.completed == false);
    }

    SECTION("Mark complete") {
        ObjectiveProgress progress;
        progress.currentCount = 10;
        progress.completed = true;
        REQUIRE(progress.completed == true);
    }
}

// ============================================================================
// Quest Log Tests
// ============================================================================

TEST_CASE("QuestLog initialization", "[quest]") {
    QuestLog log;
    REQUIRE(log.activeCount == 0);
    REQUIRE(log.completedCount == 0);
}

TEST_CASE("QuestLog - hasActiveQuest", "[quest]") {
    QuestLog log;
    
    SECTION("Returns false for empty log") {
        REQUIRE(log.hasActiveQuest(10001) == false);
    }

    SECTION("Returns true when quest is active") {
        log.activeQuests[0].questId = 10001;
        log.activeQuests[0].accepted = true;
        log.activeCount = 1;
        
        REQUIRE(log.hasActiveQuest(10001) == true);
    }

    SECTION("Returns false for different quest") {
        log.activeQuests[0].questId = 10001;
        log.activeQuests[0].accepted = true;
        log.activeCount = 1;
        
        REQUIRE(log.hasActiveQuest(10002) == false);
    }
}

TEST_CASE("QuestLog - hasCompletedQuest", "[quest]") {
    QuestLog log;
    
    SECTION("Returns false for empty log") {
        REQUIRE(log.hasCompletedQuest(10001) == false);
    }

    SECTION("Returns true when quest completed") {
        log.completedQuests[0] = 10001;
        log.completedCount = 1;
        
        REQUIRE(log.hasCompletedQuest(10001) == true);
    }

    SECTION("Returns false for different quest") {
        log.completedQuests[0] = 10001;
        log.completedCount = 1;
        
        REQUIRE(log.hasCompletedQuest(10002) == false);
    }
}

TEST_CASE("QuestLog - complete quest workflow", "[quest][integration]") {
    QuestLog log;
    
    // Accept quest
    log.activeQuests[0].questId = 10001;
    log.activeQuests[0].accepted = true;
    log.activeQuests[0].acceptTimeMs = 1000;
    log.activeCount = 1;
    REQUIRE(log.hasActiveQuest(10001) == true);
    
    // Complete objective
    log.activeQuests[0].objectives[0].currentCount = 1;
    log.activeQuests[0].objectives[0].completed = true;
    log.activeQuests[0].completed = true;
    
    // Move to completed (simulated)
    log.completedQuests[0] = 10001;
    log.completedCount = 1;
    
    // Remove from active (simulated - shifts array)
    log.activeCount = 0;
    
    REQUIRE(log.hasCompletedQuest(10001) == true);
    REQUIRE(log.hasActiveQuest(10001) == false);
}

// ============================================================================
// Quest Reward Calculation Tests
// ============================================================================

TEST_CASE("Quest - gold reward", "[quest]") {
    QuestDefinition quest;
    quest.reward.goldReward = 100;
    
    // Should give gold
    float expectedGold = 100.0f;
    REQUIRE(quest.reward.goldReward == static_cast<uint32_t>(expectedGold));
}

TEST_CASE("Quest - XP reward", "[quest]") {
    QuestDefinition quest;
    quest.reward.xpReward = 500;
    
    REQUIRE(quest.reward.xpReward == 500);
}

TEST_CASE("Quest - item reward", "[quest]") {
    QuestDefinition quest;
    quest.reward.itemId = 1001;  // Iron Sword
    quest.reward.itemQuantity = 1;
    
    REQUIRE(quest.reward.itemId == 1001);
    REQUIRE(quest.reward.itemQuantity == 1);
}

TEST_CASE("Quest - no item reward", "[quest]") {
    QuestDefinition quest;
    quest.reward.itemId = 0;
    quest.reward.itemQuantity = 0;
    
    REQUIRE(quest.reward.itemId == 0);
    REQUIRE(quest.reward.itemQuantity == 0);
}

// ============================================================================
// Quest Prerequisite Tests
// ============================================================================

TEST_CASE("Quest - no prerequisite", "[quest]") {
    QuestDefinition quest;
    quest.prerequisiteQuestId = 0;
    
    REQUIRE(quest.prerequisiteQuestId == 0);
}

TEST_CASE("Quest - with prerequisite", "[quest]") {
    QuestDefinition quest;
    quest.prerequisiteQuestId = 10001;
    
    REQUIRE(quest.prerequisiteQuestId == 10001);
}

// ============================================================================
// Quest Repeatability Tests
// ============================================================================

TEST_CASE("Quest - non-repeatable", "[quest]") {
    QuestDefinition quest;
    quest.repeatable = false;
    
    REQUIRE(quest.repeatable == false);
}

TEST_CASE("Quest - repeatable", "[quest]") {
    QuestDefinition quest;
    quest.repeatable = true;
    
    REQUIRE(quest.repeatable == true);
}

// ============================================================================
// Multiple Quest Progress Tests
// ============================================================================

TEST_CASE("QuestLog - multiple active quests", "[quest]") {
    QuestLog log;
    
    log.activeQuests[0].questId = 10001;
    log.activeQuests[0].accepted = true;
    log.activeQuests[1].questId = 10002;
    log.activeQuests[1].accepted = true;
    log.activeCount = 2;
    
    REQUIRE(log.activeCount == 2);
    REQUIRE(log.hasActiveQuest(10001) == true);
    REQUIRE(log.hasActiveQuest(10002) == true);
    REQUIRE(log.hasActiveQuest(10003) == false);
}

TEST_CASE("QuestLog - multiple completed quests", "[quest]") {
    QuestLog log;
    
    log.completedQuests[0] = 10001;
    log.completedQuests[1] = 10002;
    log.completedQuests[2] = 10003;
    log.completedCount = 3;
    
    REQUIRE(log.completedCount == 3);
    REQUIRE(log.hasCompletedQuest(10001) == true);
    REQUIRE(log.hasCompletedQuest(10002) == true);
    REQUIRE(log.hasCompletedQuest(10003) == true);
}