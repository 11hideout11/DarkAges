#include <catch2/catch_test_macros.hpp>
#include "combat/QuestSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

// ============================================================================
// Quest Log Component Tests
// ============================================================================

TEST_CASE("QuestLog basic operations", "[gameplay][quest]") {
    SECTION("Default quest log is empty") {
        QuestLog log;
        REQUIRE(log.activeCount == 0);
        REQUIRE(log.completedCount == 0);
        REQUIRE_FALSE(log.hasActiveQuest(1));
        REQUIRE_FALSE(log.hasCompletedQuest(1));
    }

    SECTION("Add active quest") {
        QuestLog log;
        REQUIRE(log.addActiveQuest(1, 1000));
        REQUIRE(log.activeCount == 1);
        REQUIRE(log.hasActiveQuest(1));
        REQUIRE(log.activeQuests[0].questId == 1);
        REQUIRE(log.activeQuests[0].accepted);
    }

    SECTION("Cannot add duplicate active quest") {
        QuestLog log;
        REQUIRE(log.addActiveQuest(1, 1000));
        REQUIRE_FALSE(log.addActiveQuest(1, 2000));
        REQUIRE(log.activeCount == 1);
    }

    SECTION("Complete quest moves to completed") {
        QuestLog log;
        log.addActiveQuest(1, 1000);
        REQUIRE(log.completeQuest(1));
        REQUIRE(log.activeCount == 0);
        REQUIRE(log.completedCount == 1);
        REQUIRE(log.hasCompletedQuest(1));
        REQUIRE_FALSE(log.hasActiveQuest(1));
    }

    SECTION("Get active quest returns pointer") {
        QuestLog log;
        log.addActiveQuest(42, 1000);
        QuestProgress* progress = log.getActiveQuest(42);
        REQUIRE(progress != nullptr);
        REQUIRE(progress->questId == 42);

        QuestProgress* missing = log.getActiveQuest(99);
        REQUIRE(missing == nullptr);
    }
}

// ============================================================================
// Quest System Tests
// ============================================================================

TEST_CASE("QuestSystem registration and lookup", "[gameplay][quest]") {
    QuestSystem qs;
    qs.initializeDefaults();

    SECTION("Can get registered quest") {
        const QuestDefinition* quest = qs.getQuest(1);
        REQUIRE(quest != nullptr);
        REQUIRE(quest->questId == 1);
        REQUIRE(std::string(quest->name) == "Proving Grounds");
    }

    SECTION("Returns nullptr for unknown quest") {
        REQUIRE(qs.getQuest(0) == nullptr);
        REQUIRE(qs.getQuest(9999) == nullptr);
    }

    SECTION("Quest 1 has correct objectives") {
        const QuestDefinition* quest = qs.getQuest(1);
        REQUIRE(quest->objectiveCount == 1);
        REQUIRE(quest->objectives[0].type == QuestObjectiveType::KillNPC);
        REQUIRE(quest->objectives[0].targetId == static_cast<uint32_t>(NPCArchetype::Melee));
        REQUIRE(quest->objectives[0].requiredCount == 5);
    }
}

TEST_CASE("QuestSystem accept and complete flow", "[gameplay][quest]") {
    entt::registry registry;
    QuestSystem qs;
    ItemSystem is;
    qs.initializeDefaults();
    is.initializeDefaults();
    qs.setItemSystem(&is);

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);
    registry.emplace<CombatState>(player);
    registry.emplace<Inventory>(player);

    SECTION("Can accept starter quest") {
        REQUIRE(qs.acceptQuest(registry, player, 1, 1000));
        REQUIRE(registry.get<QuestLog>(player).hasActiveQuest(1));
    }

    SECTION("Cannot accept same quest twice") {
        qs.acceptQuest(registry, player, 1, 1000);
        REQUIRE_FALSE(qs.acceptQuest(registry, player, 1, 2000));
    }

    SECTION("Cannot accept quest without meeting level requirement") {
        registry.get<PlayerProgression>(player).level = 1;
        // Quest 2 requires level 3
        REQUIRE_FALSE(qs.canAcceptQuest(registry, player, 2));
    }

    SECTION("Kill tracking updates objectives") {
        qs.acceptQuest(registry, player, 1, 1000);

        // Kill 4 melee NPCs — not enough
        for (int i = 0; i < 4; ++i) {
            qs.onNPCKilled(registry, player, static_cast<uint32_t>(NPCArchetype::Melee));
        }
        REQUIRE_FALSE(qs.areObjectivesComplete(registry, player, 1));

        // Kill the 5th
        qs.onNPCKilled(registry, player, static_cast<uint32_t>(NPCArchetype::Melee));
        REQUIRE(qs.areObjectivesComplete(registry, player, 1));
    }

    SECTION("Kill tracking does not count wrong archetype") {
        qs.acceptQuest(registry, player, 1, 1000);

        // Kill ranged NPCs — quest wants melee
        for (int i = 0; i < 5; ++i) {
            qs.onNPCKilled(registry, player, static_cast<uint32_t>(NPCArchetype::Ranged));
        }
        REQUIRE_FALSE(qs.areObjectivesComplete(registry, player, 1));
    }

    SECTION("Complete quest grants rewards") {
        qs.acceptQuest(registry, player, 1, 1000);

        // Complete kill objectives
        for (int i = 0; i < 5; ++i) {
            qs.onNPCKilled(registry, player, static_cast<uint32_t>(NPCArchetype::Melee));
        }

        auto& progBefore = registry.get<PlayerProgression>(player);
        uint64_t xpBefore = progBefore.currentXP;
        float goldBefore = registry.get<Inventory>(player).gold;

        REQUIRE(qs.completeQuest(registry, player, 1));

        // Check XP reward (100 XP)
        REQUIRE(registry.get<PlayerProgression>(player).currentXP == xpBefore + 100);
        // Check gold reward (50 gold)
        REQUIRE(registry.get<Inventory>(player).gold == goldBefore + 50.0f);
        // Check quest is completed
        REQUIRE(registry.get<QuestLog>(player).hasCompletedQuest(1));
    }
}

TEST_CASE("QuestSystem collect objectives", "[gameplay][quest]") {
    entt::registry registry;
    QuestSystem qs;
    ItemSystem is;
    qs.initializeDefaults();
    is.initializeDefaults();
    qs.setItemSystem(&is);

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);
    registry.emplace<Inventory>(player);

    SECTION("Item collection tracks progress") {
        // Quest 5: Collect 10 Wolf Pelts (item 20)
        qs.acceptQuest(registry, player, 5, 1000);

        qs.onItemCollected(registry, player, 20, 5);
        REQUIRE_FALSE(qs.areObjectivesComplete(registry, player, 5));

        qs.onItemCollected(registry, player, 20, 5);
        REQUIRE(qs.areObjectivesComplete(registry, player, 5));
    }

    SECTION("Wrong item does not count") {
        qs.acceptQuest(registry, player, 5, 1000);

        // Collect Iron Ore (item 21) — quest wants Wolf Pelts (item 20)
        qs.onItemCollected(registry, player, 21, 20);
        REQUIRE_FALSE(qs.areObjectivesComplete(registry, player, 5));
    }
}

TEST_CASE("QuestSystem level objectives", "[gameplay][quest]") {
    entt::registry registry;
    QuestSystem qs;
    qs.initializeDefaults();

    auto player = registry.create();
    auto& prog = registry.emplace<PlayerProgression>(player);
    prog.level = 1;
    registry.emplace<Inventory>(player);

    SECTION("Level up triggers completion") {
        // Quest 6: Reach level 5
        qs.acceptQuest(registry, player, 6, 1000);

        qs.onLevelUp(registry, player, 3);
        REQUIRE_FALSE(qs.areObjectivesComplete(registry, player, 6));

        qs.onLevelUp(registry, player, 5);
        REQUIRE(qs.areObjectivesComplete(registry, player, 6));
    }
}

TEST_CASE("QuestSystem prerequisite checking", "[gameplay][quest]") {
    entt::registry registry;
    QuestSystem qs;
    qs.initializeDefaults();

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);
    registry.emplace<CombatState>(player);
    registry.emplace<Inventory>(player);

    SECTION("Cannot accept quest with unmet prerequisite") {
        // Quest 2 requires quest 1 to be completed
        REQUIRE_FALSE(qs.canAcceptQuest(registry, player, 2));
    }

    SECTION("Can accept quest after completing prerequisite") {
        qs.acceptQuest(registry, player, 1, 1000);
        for (int i = 0; i < 5; ++i) {
            qs.onNPCKilled(registry, player, static_cast<uint32_t>(NPCArchetype::Melee));
        }
        qs.completeQuest(registry, player, 1);

        // Now can accept quest 2
        registry.get<PlayerProgression>(player).level = 3;
        REQUIRE(qs.canAcceptQuest(registry, player, 2));
    }
}

TEST_CASE("QuestSystem repeatable quests", "[gameplay][quest]") {
    entt::registry registry;
    QuestSystem qs;
    qs.initializeDefaults();

    auto player = registry.create();
    registry.emplace<PlayerProgression>(player);
    registry.emplace<Inventory>(player);

    SECTION("Repeatable quest can be re-accepted") {
        // Quest 5 is repeatable
        qs.acceptQuest(registry, player, 5, 1000);
        for (int i = 0; i < 10; ++i) {
            qs.onItemCollected(registry, player, 20, 1);
        }
        qs.completeQuest(registry, player, 5);

        // Can accept again
        REQUIRE(qs.canAcceptQuest(registry, player, 5));
        REQUIRE(qs.acceptQuest(registry, player, 5, 5000));
    }
}

// ============================================================================
// Item Consumable Use Tests
// ============================================================================

TEST_CASE("ItemSystem useItem consumables", "[gameplay][items]") {
    entt::registry registry;
    ItemSystem is;
    is.initializeDefaults();

    auto player = registry.create();
    auto& combat = registry.emplace<CombatState>(player);
    combat.health = 5000; // Half health
    combat.maxHealth = 10000;
    auto& mana = registry.emplace<Mana>(player);
    mana.current = 50.0f;
    mana.max = 100.0f;

    // Give player health potions
    is.addToInventory(registry, player, 10, 5); // Health Potion

    SECTION("Use health potion restores HP") {
        REQUIRE(is.useItem(registry, player, 1)); // Use slot 1
        // Health Potion restores 50 HP (not fixed-point, just 50)
        REQUIRE(combat.health == 5050); // Was 5000, +50
        // One potion consumed
        REQUIRE(is.countInInventory(registry, player, 10) == 4);
    }

    SECTION("Cannot use potion at full health if no mana effect") {
        combat.health = 10000; // Full health
        REQUIRE_FALSE(is.useItem(registry, player, 1));
    }

    SECTION("Cannot use empty slot") {
        REQUIRE_FALSE(is.useItem(registry, player, 5)); // Empty slot
    }

    SECTION("Cannot use non-consumable item") {
        is.addToInventory(registry, player, 1, 1); // Rusty Sword (weapon)
        // Find the slot with the sword
        // It should be in slot 2 since potions are in slot 1
        REQUIRE_FALSE(is.useItem(registry, player, 2));
    }
}

TEST_CASE("ItemSystem useItem mana potion", "[gameplay][items]") {
    entt::registry registry;
    ItemSystem is;
    is.initializeDefaults();

    auto player = registry.create();
    registry.emplace<CombatState>(player);
    auto& mana = registry.emplace<Mana>(player);
    mana.current = 50.0f;
    mana.max = 100.0f;

    is.addToInventory(registry, player, 11, 3); // Mana Potion

    SECTION("Use mana potion restores mana") {
        REQUIRE(is.useItem(registry, player, 1));
        REQUIRE(mana.current == 80.0f); // 50 + 30
        REQUIRE(is.countInInventory(registry, player, 11) == 2);
    }

    SECTION("Mana potion does not exceed max") {
        mana.current = 90.0f;
        REQUIRE(is.useItem(registry, player, 1));
        REQUIRE(mana.current == 100.0f); // Capped at max
    }
}
