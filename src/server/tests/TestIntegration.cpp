// [INTEGRATION_AGENT] Full gameplay loop integration test
// Combat + Loot + XP + Inventory
// Tests the complete flow: player kills NPC → XP awarded → loot drops → player picks up → inventory updated

#include <catch2/catch_test_macros.hpp>
#include "combat/CombatSystem.hpp"
#include "combat/ExperienceSystem.hpp"  // includes LootSystem
#include "combat/ItemSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

namespace {

// Helper: sets up a complete player entity with all required components
EntityID createPlayer(Registry& registry, ItemSystem& itemSys) {
    EntityID player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<Position>(player);
    registry.emplace<CombatState>(player);  // health=10000
    registry.emplace<PlayerProgression>(player);  // level 1, 0 XP
    registry.emplace<Inventory>(player);
    // Give some starting gold for gold-drop tests
    auto& inv = registry.get<Inventory>(player);
    inv.gold = 0.0f;
    // Register basic items and give starter gear
    itemSys.initializeDefaults();
    return player;
}

// Helper: creates an NPC entity with loot table and XP reward
EntityID createNPC(Registry& registry, ItemSystem& itemSys, NPCArchetype archetype = NPCArchetype::Melee) {
    EntityID npc = registry.create();
    registry.emplace<NPCTag>(npc);
    registry.emplace<Position>(npc);
    // Default combat state - full health
    CombatState& cs = registry.emplace<CombatState>(npc);
    cs.health = 10000;
    cs.maxHealth = 10000;
    cs.isDead = false;

    // NPC stats - determine XP reward
    NPCStats& stats = registry.emplace<NPCStats>(npc);
    stats.level = 1;
    stats.xpReward = 100;  // flat 100 XP
    stats.archetype = archetype;

    // Build loot table
    LootTable& table = registry.emplace<LootTable>(npc);
    table.goldDropMin = 15.0f;
    table.goldDropMax = 30.0f;
    table.count = 1;
    // Guaranteed drop: Health Potion (itemId=10)
    table.entries[0] = LootEntry{10, 1, 1, 1.0f};  // 100% chance, 1 quantity
    // Item 10 exists: Health Potion (registered by ItemSystem::initializeDefaults())

    // Also ensure item definitions are registered (already done by player setup)
    // We called initializeDefaults() in createPlayer already

    return npc;
}

// Helper: create a melee attack input
AttackInput makeMeleeAttack(uint32_t seq, uint32_t time) {
    AttackInput input;
    input.type = AttackInput::MELEE;
    input.sequence = seq;
    input.timestamp = time;
    input.targetEntity = 0;  // no specific target (will auto-select)
    input.aimDirection = glm::vec3(0, 0, 1);
    return input;
}

// Helper: directly apply massive damage to kill NPC quickly
void killNPC(Registry& registry, CombatSystem& combat, EntityID npc) {
    // Find attacker entity (player). For this test we just want NPC dead.
    // We'll apply damage to set health to 0 directly.
    CombatState& cs = registry.get<CombatState>(npc);
    cs.health = 0;
    cs.isDead = true;
}


TEST_CASE("Integration: Full combat → loot → XP → inventory cycle", "[integration][combat][loot][xp][inventory]") {
    Registry registry;
    CombatConfig combatConfig;
    combatConfig.baseMeleeDamage = 50000;  // huge to guarantee kill
    CombatSystem combat(combatConfig);
    ExperienceSystem xpSystem;
    LootSystem lootSystem;
    ItemSystem itemSys;

    // Initialize item database
    itemSys.initializeDefaults();

    // Wire combat death to XP and loot systems (integration point)
    combat.setOnDeath([&](EntityID victim, EntityID killer) {
        xpSystem.awardKillXP(registry, killer, victim);
        lootSystem.generateLoot(registry, victim, killer);
    });

    // Track state changes via callbacks
    bool xpAwarded = false;
    uint64_t xpAmountGained = 0;
    xpSystem.setXPGainCallback([&](EntityID player, uint64_t amount) {
        xpAwarded = true;
        xpAmountGained = amount;
    });

    bool lootCreated = false;
    std::vector<EntityID> createdLootEntities;
    lootSystem.setLootDropCallback([&](EntityID lootEntity, uint32_t itemId, uint32_t quantity, float gold) {
        (void)itemId; (void)quantity; (void)gold;
        lootCreated = true;
        createdLootEntities.push_back(lootEntity);
    });

    bool itemPickedUp = false;
    uint32_t pickedItemId = 0;
    uint32_t pickedQuantity = 0;
    float pickedGold = 0.0f;
    lootSystem.setLootPickupCallback([&](EntityID player, uint32_t itemId, uint32_t quantity, float gold) {
        itemPickedUp = true;
        pickedItemId = itemId;
        pickedQuantity = quantity;
        pickedGold = gold;
        // Integrate with inventory: add item/gold on pickup
        if (itemId != 0) {
            itemSys.addToInventory(registry, player, itemId, quantity);
        }
        if (gold > 0.0f) {
            if (Inventory* inv = registry.try_get<Inventory>(player)) {
                inv->gold += gold;
            }
        }
    });

    // Create player and NPC
    EntityID player = createPlayer(registry, itemSys);
    EntityID npc = createNPC(registry, itemSys);

    // Position: put both at same spot for easy target
    auto& playerPos = registry.get<Position>(player);
    auto& npcPos = registry.get<Position>(npc);
    playerPos.x = playerPos.y = playerPos.z = 0;
    npcPos.x = npcPos.y = npcPos.z = 0;

    // Get initial XP
    PlayerProgression& prog = registry.get<PlayerProgression>(player);
    uint64_t initialXP = prog.currentXP;
    uint32_t initialLevel = prog.level;
    REQUIRE(initialLevel == 1);
    REQUIRE(initialXP == 0);

    // Get initial inventory state
    Inventory& inv = registry.get<Inventory>(player);
    REQUIRE(inv.countItem(10) == 0);  // No health potions yet
    float initialGold = inv.gold;

    // Simulate death of NPC via combat system's killEntity
    // This is the entry point when an NPC dies
    combat.killEntity(registry, npc, player);  // player is killer

    // Verify NPC is dead
    CombatState& npcCs = registry.get<CombatState>(npc);
    REQUIRE(npcCs.isDead == true);

    // Verify XP awarded to killer (player)
    REQUIRE(xpAwarded == true);
    REQUIRE(xpAmountGained == 100);
    // Awarding exactly 100 XP triggers level-up (xpToNextLevel starts at 100)
    // currentXP += 100 => currentXP = 100
    // while(currentXP >= xpToNextLevel): true (100>=100)
    //   currentXP -= xpToNextLevel => 0
    //   level++ => 2
    //   xpToNextLevel = xpForLevel(2) ≈ 283
    REQUIRE(prog.level == 2);
    REQUIRE(prog.currentXP == 0);
    REQUIRE(prog.xpToNextLevel == ExperienceSystem::xpForLevel(2));

    // Verify loot generated
    REQUIRE(lootCreated == true);
    REQUIRE(createdLootEntities.size() >= 1);  // At least 1 item entry dropped

    // Count loot entities in registry that have LootDropTag
    auto lootView = registry.view<LootDropTag>();
    REQUIRE(lootView.size() >= 1);
    // The createdLootEntities from callback should match
    for (auto e : createdLootEntities) {
        REQUIRE(registry.valid(e));
        REQUIRE(registry.all_of<LootDropTag>(e));
        REQUIRE(registry.all_of<LootDropData>(e));
    }

    // Verify there is at least one item loot (itemId 10) and one gold loot
    bool foundHealthPotionLoot = false;
    bool foundGoldLoot = false;
    for (auto e : lootView) {
        const LootDropData& data = registry.get<LootDropData>(e);
        if (data.itemId == 10) {
            foundHealthPotionLoot = true;
            REQUIRE(data.quantity == 1);
            REQUIRE(data.goldAmount == 0.0f);
            // Verify owner is the killer
            REQUIRE(data.ownerPlayer == player);
            // Verify position approx near NPC (x,y,z same NPC pos with small offset)
            const Position& lp = registry.get<Position>(e);
            // offset within ±2 fixed units
            // We can't easily compare due to offset randomness, but it should exist
        } else if (data.itemId == 0) {
            foundGoldLoot = true;
            REQUIRE(data.quantity == 0);
            REQUIRE(data.goldAmount >= 15.0f);
            REQUIRE(data.goldAmount <= 30.0f);
        }
    }
    REQUIRE(foundHealthPotionLoot);
    REQUIRE(foundGoldLoot);

    // --- Pick up loot ---
    // Find the health potion loot entity
    EntityID healthLootEntity = entt::null;
    for (auto e : lootView) {
        const LootDropData& data = registry.get<LootDropData>(e);
        if (data.itemId == 10) {
            healthLootEntity = e;
            break;
        }
    }
    REQUIRE(registry.valid(healthLootEntity));

    // Pickup
    REQUIRE_FALSE(itemPickedUp);  // not yet
    bool pickedUp = lootSystem.pickupLoot(registry, player, healthLootEntity);
    REQUIRE(pickedUp == true);
    REQUIRE(itemPickedUp == true);
    REQUIRE(pickedItemId == 10);
    REQUIRE(pickedQuantity == 1);

    // Loot entity should be destroyed
    REQUIRE_FALSE(registry.valid(healthLootEntity));

    // Verify inventory updated
    REQUIRE(inv.countItem(10) == 1);
    // Should have exactly one Health Potion now
    REQUIRE(inv.slots[0].itemId == 10);  // slot 0 is first empty slot
    REQUIRE(inv.slots[0].quantity == 1);

    // Pick up gold loot
    EntityID goldLootEntity = entt::null;
    for (auto e : registry.view<LootDropTag>()) {
        const LootDropData& data = registry.get<LootDropData>(e);
        if (data.itemId == 0) {
            goldLootEntity = e;
            break;
        }
    }
    REQUIRE(registry.valid(goldLootEntity));

    float goldBefore = inv.gold;
    pickedUp = lootSystem.pickupLoot(registry, player, goldLootEntity);
    REQUIRE(pickedUp);
    REQUIRE(inv.gold > goldBefore);
    REQUIRE_FALSE(registry.valid(goldLootEntity));

    // loot view should now be empty
    REQUIRE(registry.view<LootDropTag>().size() == 0);
}

TEST_CASE("Integration: Loot despawn timing", "[integration][loot]") {
    Registry registry;
    LootSystem lootSys;
    uint32_t now = 1000;

    // Create player and entity
    EntityID player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<Position>(player);

    EntityID loot = registry.create();
    registry.emplace<LootDropTag>(loot);
    LootDropData data;
    data.itemId = 10;
    data.quantity = 1;
    data.goldAmount = 0.0f;
    data.ownerPlayer = player;
    data.despawnTimeMs = now + 5000;  // expires in 5 seconds
    registry.emplace<LootDropData>(loot, data);

    REQUIRE(registry.valid(loot));
    REQUIRE(registry.view<LootDropTag>().size() == 1);

    // Update before expiry — nothing happens
    lootSys.update(registry, now + 1000);
    REQUIRE(registry.valid(loot));

    // Update exactly at expiry — entity destroyed
    lootSys.update(registry, now + 5000);
    REQUIRE_FALSE(registry.valid(loot));
    REQUIRE(registry.view<LootDropTag>().size() == 0);
}

TEST_CASE("Integration: XP level-up progression with multiple levels", "[integration][xp]") {
    Registry registry;
    ExperienceSystem xpSys;
    ItemSystem itemSys;
    itemSys.initializeDefaults();

    EntityID player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<PlayerProgression>(player);
    auto& prog = registry.get<PlayerProgression>(player);
    prog.level = 1;
    prog.currentXP = 0;
    prog.xpToNextLevel = ExperienceSystem::xpForLevel(1);  // 100

    // Award XP = xpForLevel(1) + xpForLevel(2) + 50
    // Should level up twice, end at level 3 with some XP
    uint64_t largeXP = 100 + ExperienceSystem::xpForLevel(2) + 50;
    bool leveled = xpSys.awardXP(registry, player, largeXP);
    REQUIRE(leveled);
    REQUIRE(prog.level == 3);
    REQUIRE(prog.currentXP == 50);
    REQUIRE(prog.statPoints == 6);  // 3 per level × 2 levels gained
}

TEST_CASE("Integration: Inventory stack limit and overflow", "[integration][inventory]") {
    Registry registry;
    ItemSystem itemSys;
    itemSys.initializeDefaults();

    EntityID player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<Inventory>(player);
    Inventory& inv = registry.get<Inventory>(player);

    // Item 10: Health Potion, maxStackSize = 20
    const ItemDefinition* def = itemSys.getItem(10);
    REQUIRE(def != nullptr);
    REQUIRE(def->maxStackSize == 20);

    // Add 25 potions → distributes across slots (20 in slot 0, 5 in slot 1)
    uint32_t overflow = itemSys.addToInventory(registry, player, 10, 25);
    REQUIRE(overflow == 0);
    REQUIRE(inv.countItem(10) == 25);

    // Add another 10 → fills slot 1 to 15, total 35
    overflow = itemSys.addToInventory(registry, player, 10, 10);
    REQUIRE(overflow == 0);
    REQUIRE(inv.countItem(10) == 35);  // total of 35 across slots

    // Remove 5
    bool removed = itemSys.removeFromInventory(registry, player, 10, 5);
    REQUIRE(removed);
    REQUIRE(inv.countItem(10) == 30);

    // Remove 40 (more than exists) → false, no change
    removed = itemSys.removeFromInventory(registry, player, 10, 40);
    REQUIRE_FALSE(removed);
    REQUIRE(inv.countItem(10) == 30);
}

TEST_CASE("Integration: Equip item and stat calculation", "[integration][item]") {
    Registry registry;
    ItemSystem itemSys;
    itemSys.initializeDefaults();

    EntityID player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<Inventory>(player);
    itemSys.addToInventory(registry, player, 3, 1);   // Iron Longsword: +12 damage
    itemSys.addToInventory(registry, player, 2, 1);   // Leather Tunic: +50 health

    // Equip main hand
    REQUIRE(itemSys.equipItem(registry, player, 3));
    const Equipment& equip = registry.get<Equipment>(player);
    REQUIRE(equip.mainHand == 3);

    // Equip chest
    REQUIRE(itemSys.equipItem(registry, player, 2));
    REQUIRE(equip.chest == 2);

    // Check combined stats
    ItemStats stats = itemSys.getEquippedStats(registry, player);
    REQUIRE(stats.damageBonus == 12);
    REQUIRE(stats.healthBonus == 50);
}

TEST_CASE("Integration: Loot ownership and pickup permissions", "[integration][loot]") {
    Registry registry;
    LootSystem lootSys;
    ItemSystem itemSys;
    itemSys.initializeDefaults();

    EntityID player1 = registry.create();
    registry.emplace<PlayerTag>(player1);
    registry.emplace<Position>(player1);
    registry.emplace<Inventory>(player1);

    EntityID player2 = registry.create();
    registry.emplace<PlayerTag>(player2);
    registry.emplace<Position>(player2);
    registry.emplace<Inventory>(player2);

    // Create a loot entity with specific owner
    EntityID loot = registry.create();
    registry.emplace<LootDropTag>(loot);
    LootDropData data;
    data.itemId = 10;
    data.quantity = 1;
    data.goldAmount = 0.0f;
    data.ownerPlayer = player1;  // only player1 can pick up
    data.despawnTimeMs = 0;
    registry.emplace<LootDropData>(loot, data);

    // player1 can pick up
    REQUIRE(lootSys.pickupLoot(registry, player1, loot));
    REQUIRE_FALSE(registry.valid(loot));

    // Reset: recreate loot
    loot = registry.create();
    registry.emplace<LootDropTag>(loot);
    data.ownerPlayer = player1;
    registry.emplace<LootDropData>(loot, data);
    REQUIRE(registry.valid(loot));

    // player2 cannot pick up
    REQUIRE_FALSE(lootSys.pickupLoot(registry, player2, loot));
    REQUIRE(registry.valid(loot));  // still exists

    // player1 can still pick up
    REQUIRE(lootSys.pickupLoot(registry, player1, loot));
    REQUIRE_FALSE(registry.valid(loot));
}

TEST_CASE("Integration: ItemSystem useItem consumable heals and consumes", "[integration][item][combat]") {
    Registry registry;
    ItemSystem itemSys;
    itemSys.initializeDefaults();

    EntityID player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<CombatState>(player);
    auto& cs = registry.get<CombatState>(player);
    cs.health = 5000;  // 50% health
    cs.maxHealth = 10000;
    registry.emplace<Inventory>(player);
    itemSys.addToInventory(registry, player, 10, 3);  // 3 Health Potions

    // Use health potion (slot 1)
    REQUIRE(itemSys.useItem(registry, player, 1));
    REQUIRE(cs.health == 5050);  // +50
    REQUIRE(registry.get<Inventory>(player).countItem(10) == 2);  // consumed one

    // Use again
    REQUIRE(itemSys.useItem(registry, player, 1));
    REQUIRE(cs.health == 5100);
    REQUIRE(registry.get<Inventory>(player).countItem(10) == 1);

    // Consume last potion
    REQUIRE(itemSys.useItem(registry, player, 1));
    REQUIRE(cs.health == 5150);
    REQUIRE(registry.get<Inventory>(player).countItem(10) == 0);
}

// TODO: More integration tests — PartyXP sharing, Quest kill tracking with ZoneEvent
// These require wiring additional systems (PartySystem, QuestSystem, ZoneEventSystem)

} // namespace
