#include <catch2/catch_test_macros.hpp>
#include "combat/ItemSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

// ============================================================================
// Inventory Component Tests
// ============================================================================

TEST_CASE("Inventory basic operations", "[gameplay][inventory]") {
    SECTION("Default inventory is empty") {
        Inventory inv;
        REQUIRE(inv.gold == 0.0f);
        REQUIRE(inv.slotCount == INVENTORY_SIZE);
        REQUIRE(inv.findEmptySlot() == 0);
    }

    SECTION("Find empty slot returns first available") {
        Inventory inv;
        inv.slots[0].itemId = 1;
        inv.slots[0].quantity = 5;
        inv.slots[1].itemId = 2;
        inv.slots[1].quantity = 3;

        REQUIRE(inv.findEmptySlot() == 2);
    }

    SECTION("Find empty slot returns -1 when full") {
        Inventory inv;
        for (uint32_t i = 0; i < INVENTORY_SIZE; ++i) {
            inv.slots[i].itemId = i + 1;
            inv.slots[i].quantity = 1;
        }
        REQUIRE(inv.findEmptySlot() == -1);
    }
}

TEST_CASE("Inventory add item", "[gameplay][inventory]") {
    SECTION("Add item to empty inventory") {
        Inventory inv;
        uint32_t overflow = inv.addItem(10, 5, 20);
        REQUIRE(overflow == 0);
        REQUIRE(inv.slots[0].itemId == 10);
        REQUIRE(inv.slots[0].quantity == 5);
    }

    SECTION("Stack items in existing slot") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 10;

        uint32_t overflow = inv.addItem(10, 5, 20);
        REQUIRE(overflow == 0);
        REQUIRE(inv.slots[0].quantity == 15);
        REQUIRE(inv.findEmptySlot() == 1); // Second slot still empty
    }

    SECTION("Overflow when stack is full") {
        Inventory inv;
        // Fill all slots except the last two
        for (uint32_t i = 0; i < INVENTORY_SIZE - 1; ++i) {
            inv.slots[i].itemId = i + 1;
            inv.slots[i].quantity = 10; // Each has 10 items
        }
        // Last slot has 18/20 of item 100
        inv.slots[INVENTORY_SIZE - 1].itemId = 100;
        inv.slots[INVENTORY_SIZE - 1].quantity = 18;

        // Try to add 5 more of item 100: 2 fit in last slot, 3 overflow
        uint32_t overflow = inv.addItem(100, 5, 20);
        REQUIRE(overflow == 3);
        REQUIRE(inv.slots[INVENTORY_SIZE - 1].quantity == 20);
    }

    SECTION("Fill multiple slots") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 19;

        uint32_t overflow = inv.addItem(10, 5, 20);
        REQUIRE(overflow == 0);
        REQUIRE(inv.slots[0].quantity == 20);
        REQUIRE(inv.slots[1].itemId == 10);
        REQUIRE(inv.slots[1].quantity == 4);
    }

    SECTION("Full inventory returns overflow") {
        Inventory inv;
        for (uint32_t i = 0; i < INVENTORY_SIZE; ++i) {
            inv.slots[i].itemId = i + 1;
            inv.slots[i].quantity = 1;
        }
        uint32_t overflow = inv.addItem(100, 5, 20);
        REQUIRE(overflow == 5);
    }
}

TEST_CASE("Inventory remove item", "[gameplay][inventory]") {
    SECTION("Remove existing item") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 10;

        bool success = inv.removeItem(10, 5);
        REQUIRE(success);
        REQUIRE(inv.slots[0].quantity == 5);
    }

    SECTION("Remove all items clears slot") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 5;

        bool success = inv.removeItem(10, 5);
        REQUIRE(success);
        REQUIRE(inv.slots[0].quantity == 0);
        REQUIRE(inv.slots[0].itemId == 0);
    }

    SECTION("Remove from multiple slots") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 10;
        inv.slots[1].itemId = 10;
        inv.slots[1].quantity = 5;

        bool success = inv.removeItem(10, 12);
        REQUIRE(success);
        REQUIRE(inv.slots[0].quantity == 0);
        REQUIRE(inv.slots[1].quantity == 3);
    }

    SECTION("Remove fails when insufficient items") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 3;

        bool success = inv.removeItem(10, 5);
        REQUIRE_FALSE(success);
        REQUIRE(inv.slots[0].quantity == 3); // Unchanged
    }
}

TEST_CASE("Inventory count item", "[gameplay][inventory]") {
    SECTION("Count items across multiple slots") {
        Inventory inv;
        inv.slots[0].itemId = 10;
        inv.slots[0].quantity = 10;
        inv.slots[1].itemId = 10;
        inv.slots[1].quantity = 5;
        inv.slots[2].itemId = 20;
        inv.slots[2].quantity = 3;

        REQUIRE(inv.countItem(10) == 15);
        REQUIRE(inv.countItem(20) == 3);
        REQUIRE(inv.countItem(99) == 0);
    }
}

// ============================================================================
// Equipment Component Tests
// ============================================================================

TEST_CASE("Equipment equip/unequip", "[gameplay][equipment]") {
    SECTION("Equip to empty slot") {
        Equipment equip;
        uint32_t old = equip.equip(EquipSlot::MainHand, 1);
        REQUIRE(old == 0);
        REQUIRE(equip.mainHand == 1);
    }

    SECTION("Equip replaces existing item") {
        Equipment equip;
        equip.mainHand = 1;
        uint32_t old = equip.equip(EquipSlot::MainHand, 5);
        REQUIRE(old == 1);
        REQUIRE(equip.mainHand == 5);
    }

    SECTION("Equip all slots") {
        Equipment equip;
        equip.equip(EquipSlot::MainHand, 1);
        equip.equip(EquipSlot::OffHand, 2);
        equip.equip(EquipSlot::Head, 3);
        equip.equip(EquipSlot::Chest, 4);
        equip.equip(EquipSlot::Legs, 5);
        equip.equip(EquipSlot::Feet, 6);
        equip.equip(EquipSlot::Ring, 7);
        equip.equip(EquipSlot::Amulet, 8);

        REQUIRE(equip.mainHand == 1);
        REQUIRE(equip.offHand == 2);
        REQUIRE(equip.head == 3);
        REQUIRE(equip.chest == 4);
        REQUIRE(equip.legs == 5);
        REQUIRE(equip.feet == 6);
        REQUIRE(equip.ring == 7);
        REQUIRE(equip.amulet == 8);
    }

    SECTION("Equip to None slot does nothing") {
        Equipment equip;
        uint32_t old = equip.equip(EquipSlot::None, 99);
        REQUIRE(old == 0);
        REQUIRE(equip.mainHand == 0);
    }
}

// ============================================================================
// AbilityLoadout Tests
// ============================================================================

TEST_CASE("AbilityLoadout slot access", "[gameplay][abilities]") {
    SECTION("Get ability from valid slot") {
        AbilityLoadout loadout;
        loadout.abilityIds[0] = 1; // Fireball
        loadout.abilityIds[1] = 2; // Heal
        loadout.abilityIds[2] = 3; // Power Strike
        loadout.abilityIds[3] = 0; // Empty

        REQUIRE(loadout.getAbilityInSlot(1) == 1);
        REQUIRE(loadout.getAbilityInSlot(2) == 2);
        REQUIRE(loadout.getAbilityInSlot(3) == 3);
        REQUIRE(loadout.getAbilityInSlot(4) == 0);
    }

    SECTION("Invalid slot returns 0") {
        AbilityLoadout loadout;
        REQUIRE(loadout.getAbilityInSlot(0) == 0);
        REQUIRE(loadout.getAbilityInSlot(5) == 0);
    }
}

// ============================================================================
// ItemSystem Tests
// ============================================================================

TEST_CASE("ItemSystem registry", "[gameplay][items]") {
    ItemSystem items;
    items.initializeDefaults();

    SECTION("Default items are registered") {
        REQUIRE(items.hasItem(1));  // Rusty Sword
        REQUIRE(items.hasItem(2));  // Leather Tunic
        REQUIRE(items.hasItem(10)); // Health Potion
        REQUIRE(items.hasItem(11)); // Mana Potion
    }

    SECTION("Get item by ID") {
        const ItemDefinition* sword = items.getItem(1);
        REQUIRE(sword != nullptr);
        REQUIRE(std::string(sword->name) == "Rusty Sword");
        REQUIRE(sword->type == ItemType::Weapon);
        REQUIRE(sword->equipSlot == EquipSlot::MainHand);
        REQUIRE(sword->stats.damageBonus == 5);
    }

    SECTION("Get item by name") {
        const ItemDefinition* potion = items.getItemByName("Health Potion");
        REQUIRE(potion != nullptr);
        REQUIRE(potion->itemId == 10);
        REQUIRE(potion->type == ItemType::Consumable);
        REQUIRE(potion->maxStackSize == 20);
    }

    SECTION("Non-existent item returns nullptr") {
        REQUIRE(items.getItem(9999) == nullptr);
        REQUIRE(items.getItem(0) == nullptr);
        REQUIRE(items.getItemByName("Nonexistent") == nullptr);
    }
}

TEST_CASE("ItemSystem inventory operations", "[gameplay][items]") {
    entt::registry registry;
    ItemSystem items;
    items.initializeDefaults();

    auto entity = registry.create();

    SECTION("Add items to inventory") {
        uint32_t overflow = items.addToInventory(registry, entity, 10, 5);
        REQUIRE(overflow == 0);
        REQUIRE(items.countInInventory(registry, entity, 10) == 5);
    }

    SECTION("Remove items from inventory") {
        items.addToInventory(registry, entity, 10, 10);
        bool success = items.removeFromInventory(registry, entity, 10, 5);
        REQUIRE(success);
        REQUIRE(items.countInInventory(registry, entity, 10) == 5);
    }

    SECTION("Has in inventory check") {
        items.addToInventory(registry, entity, 10, 5);
        REQUIRE(items.hasInInventory(registry, entity, 10, 5));
        REQUIRE_FALSE(items.hasInInventory(registry, entity, 10, 6));
        REQUIRE_FALSE(items.hasInInventory(registry, entity, 99, 1));
    }
}

TEST_CASE("ItemSystem equipment operations", "[gameplay][items]") {
    entt::registry registry;
    ItemSystem items;
    items.initializeDefaults();

    auto entity = registry.create();

    SECTION("Equip item from inventory") {
        items.addToInventory(registry, entity, 1, 1); // Rusty Sword
        bool success = items.equipItem(registry, entity, 1);
        REQUIRE(success);
        REQUIRE(items.countInInventory(registry, entity, 1) == 0); // Moved from inventory

        const Equipment* equip = registry.try_get<Equipment>(entity);
        REQUIRE(equip != nullptr);
        REQUIRE(equip->mainHand == 1);
    }

    SECTION("Equip replaces existing item") {
        items.addToInventory(registry, entity, 1, 1); // Rusty Sword
        items.equipItem(registry, entity, 1);

        items.addToInventory(registry, entity, 3, 1); // Iron Longsword
        bool success = items.equipItem(registry, entity, 3);
        REQUIRE(success);

        const Equipment* equip = registry.try_get<Equipment>(entity);
        REQUIRE(equip->mainHand == 3);
        // Old sword should be back in inventory
        REQUIRE(items.countInInventory(registry, entity, 1) == 1);
    }

    SECTION("Cannot equip non-equipable item") {
        items.addToInventory(registry, entity, 10, 1); // Health Potion
        bool success = items.equipItem(registry, entity, 10);
        REQUIRE_FALSE(success); // Consumable cannot be equipped
    }

    SECTION("Cannot equip item not in inventory") {
        bool success = items.equipItem(registry, entity, 1);
        REQUIRE_FALSE(success);
    }

    SECTION("Unequip item to inventory") {
        items.addToInventory(registry, entity, 1, 1);
        items.equipItem(registry, entity, 1);

        bool success = items.unequipItem(registry, entity, EquipSlot::MainHand);
        REQUIRE(success);

        const Equipment* equip = registry.try_get<Equipment>(entity);
        REQUIRE(equip->mainHand == 0);
        REQUIRE(items.countInInventory(registry, entity, 1) == 1);
    }

    SECTION("Get equipped stats") {
        items.addToInventory(registry, entity, 1, 1); // Rusty Sword (+5 dmg)
        items.equipItem(registry, entity, 1);
        items.addToInventory(registry, entity, 2, 1); // Leather Tunic (+50 HP)
        items.equipItem(registry, entity, 2);

        ItemStats stats = items.getEquippedStats(registry, entity);
        REQUIRE(stats.damageBonus == 5);
        REQUIRE(stats.healthBonus == 50);
    }
}

TEST_CASE("ItemSystem starter kit", "[gameplay][items]") {
    entt::registry registry;
    ItemSystem items;
    items.initializeDefaults();

    auto entity = registry.create();

    SECTION("Starter kit gives items and abilities") {
        items.giveStarterKit(registry, entity);

        // Should have inventory
        REQUIRE(registry.all_of<Inventory>(entity));

        // Should have equipment
        const Equipment* equip = registry.try_get<Equipment>(entity);
        REQUIRE(equip != nullptr);
        REQUIRE(equip->mainHand == 1);  // Rusty Sword equipped
        REQUIRE(equip->chest == 2);    // Leather Tunic equipped

        // Should have abilities
        const AbilityLoadout* loadout = registry.try_get<AbilityLoadout>(entity);
        REQUIRE(loadout != nullptr);
        REQUIRE(loadout->getAbilityInSlot(1) == 1); // Fireball
        REQUIRE(loadout->getAbilityInSlot(2) == 2); // Heal
        REQUIRE(loadout->getAbilityInSlot(3) == 3); // Power Strike

        // Should have potions in inventory
        REQUIRE(items.countInInventory(registry, entity, 10) == 5); // Health Potions
        REQUIRE(items.countInInventory(registry, entity, 11) == 3); // Mana Potions

        // Should have gold
        const Inventory* inv = registry.try_get<Inventory>(entity);
        REQUIRE(inv->gold == 100.0f);
    }
}

// ============================================================================
// NPC Archetype Tests
// ============================================================================

TEST_CASE("NPCArchetype enum values", "[gameplay][npc]") {
    SECTION("Archetype enum has correct values") {
        REQUIRE(static_cast<uint8_t>(NPCArchetype::Melee) == 0);
        REQUIRE(static_cast<uint8_t>(NPCArchetype::Ranged) == 1);
        REQUIRE(static_cast<uint8_t>(NPCArchetype::Caster) == 2);
        REQUIRE(static_cast<uint8_t>(NPCArchetype::Boss) == 3);
    }
}

TEST_CASE("NPCArchetypeConfig defaults", "[gameplay][npc]") {
    SECTION("Default config is melee") {
        NPCArchetypeConfig config;
        REQUIRE(config.archetype == NPCArchetype::Melee);
        REQUIRE(config.preferredRange == 2.0f);
        REQUIRE(config.retreatRange == 1.5f);
        REQUIRE(config.healthScalePercent == 100);
        REQUIRE(config.damageScalePercent == 100);
    }
}

// ============================================================================
// Item Definition Validation
// ============================================================================

TEST_CASE("Item definition properties", "[gameplay][items]") {
    SECTION("ItemRarity enum values") {
        REQUIRE(static_cast<uint8_t>(ItemRarity::Common) == 0);
        REQUIRE(static_cast<uint8_t>(ItemRarity::Uncommon) == 1);
        REQUIRE(static_cast<uint8_t>(ItemRarity::Rare) == 2);
        REQUIRE(static_cast<uint8_t>(ItemRarity::Epic) == 3);
        REQUIRE(static_cast<uint8_t>(ItemRarity::Legendary) == 4);
    }

    SECTION("ItemType enum values") {
        REQUIRE(static_cast<uint8_t>(ItemType::Weapon) == 0);
        REQUIRE(static_cast<uint8_t>(ItemType::Armor) == 1);
        REQUIRE(static_cast<uint8_t>(ItemType::Accessory) == 2);
        REQUIRE(static_cast<uint8_t>(ItemType::Consumable) == 3);
        REQUIRE(static_cast<uint8_t>(ItemType::Material) == 4);
        REQUIRE(static_cast<uint8_t>(ItemType::Quest) == 5);
    }

    SECTION("EquipSlot enum values") {
        REQUIRE(static_cast<uint8_t>(EquipSlot::None) == 0);
        REQUIRE(static_cast<uint8_t>(EquipSlot::MainHand) == 1);
        REQUIRE(static_cast<uint8_t>(EquipSlot::Chest) == 4);
        REQUIRE(static_cast<uint8_t>(EquipSlot::Amulet) == 8);
    }
}

TEST_CASE("Default item properties", "[gameplay][items]") {
    ItemSystem items;
    items.initializeDefaults();

    SECTION("Rusty Sword") {
        const ItemDefinition* sword = items.getItem(1);
        REQUIRE(sword->type == ItemType::Weapon);
        REQUIRE(sword->rarity == ItemRarity::Common);
        REQUIRE(sword->equipSlot == EquipSlot::MainHand);
        REQUIRE(sword->stats.damageBonus == 5);
        REQUIRE(sword->maxStackSize == 1);
        REQUIRE(sword->requiredLevel == 1);
    }

    SECTION("Health Potion") {
        const ItemDefinition* potion = items.getItem(10);
        REQUIRE(potion->type == ItemType::Consumable);
        REQUIRE(potion->rarity == ItemRarity::Common);
        REQUIRE(potion->equipSlot == EquipSlot::None);
        REQUIRE(potion->stats.healthBonus == 50);
        REQUIRE(potion->maxStackSize == 20);
    }

    SECTION("Vorpal Blade (Legendary)") {
        const ItemDefinition* blade = items.getItem(30);
        REQUIRE(blade->rarity == ItemRarity::Legendary);
        REQUIRE(blade->stats.damageBonus == 50);
        REQUIRE(blade->stats.critChanceBonus == 0.15f);
        REQUIRE(blade->requiredLevel == 25);
        REQUIRE_FALSE(blade->tradable);
    }

    SECTION("Ancient Relic Shard") {
        const ItemDefinition* shard = items.getItem(22);
        REQUIRE(shard->rarity == ItemRarity::Epic);
        REQUIRE(shard->type == ItemType::Material);
        REQUIRE(shard->maxStackSize == 10);
        REQUIRE_FALSE(shard->tradable);
    }
}

// ============================================================================
// Inventory Slot Component
// ============================================================================

TEST_CASE("InventorySlot", "[gameplay][inventory]") {
    SECTION("Empty slot") {
        InventorySlot slot;
        REQUIRE(slot.isEmpty());
        REQUIRE(slot.itemId == 0);
        REQUIRE(slot.quantity == 0);
    }

    SECTION("Non-empty slot") {
        InventorySlot slot;
        slot.itemId = 10;
        slot.quantity = 5;
        REQUIRE_FALSE(slot.isEmpty());
    }

    SECTION("Zero quantity is empty") {
        InventorySlot slot;
        slot.itemId = 10;
        slot.quantity = 0;
        REQUIRE(slot.isEmpty());
    }
}

// ============================================================================
// Integration: Inventory with Items
// ============================================================================

TEST_CASE("Full item lifecycle", "[gameplay][integration]") {
    entt::registry registry;
    ItemSystem items;
    items.initializeDefaults();

    auto player = registry.create();

    SECTION("Pick up, equip, use, drop") {
        // Pick up a sword
        items.addToInventory(registry, player, 1, 1);
        REQUIRE(items.hasInInventory(registry, player, 1, 1));

        // Equip it
        items.equipItem(registry, player, 1);
        const Equipment* equip = registry.try_get<Equipment>(player);
        REQUIRE(equip->mainHand == 1);

        // Check stat bonus
        ItemStats stats = items.getEquippedStats(registry, player);
        REQUIRE(stats.damageBonus == 5);

        // Pick up a health potion
        items.addToInventory(registry, player, 10, 10);
        REQUIRE(items.countInInventory(registry, player, 10) == 10);

        // Use (remove) some potions
        items.removeFromInventory(registry, player, 10, 3);
        REQUIRE(items.countInInventory(registry, player, 10) == 7);

        // Unequip sword
        items.unequipItem(registry, player, EquipSlot::MainHand);
        REQUIRE(equip->mainHand == 0);
        REQUIRE(items.hasInInventory(registry, player, 1, 1));
    }
}
