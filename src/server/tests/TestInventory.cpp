// [COMBAT_AGENT] Unit tests for Inventory and Equipment Systems

#include <catch2/catch_test_macros.hpp>
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <string>
#include <vector>

using namespace DarkAges;

// ============================================================================
// Inventory Component Tests
// ============================================================================

TEST_CASE("InventoryComponent - Basic operations", "[inventory]") {
    SECTION("Empty inventory starts with no items") {
        Inventory inventory;
        REQUIRE(inventory.findEmptySlot() >= 0); // Should find first slot as empty
        
        int32_t emptyCount = 0;
        for (uint32_t i = 0; i < inventory.slotCount; ++i) {
            if (inventory.slots[i].isEmpty()) emptyCount++;
        }
        REQUIRE(emptyCount == INVENTORY_SIZE);
    }

    SECTION("Gold starts at zero") {
        Inventory inventory;
        REQUIRE(inventory.gold == 0.0f);
    }
}

TEST_CASE("InventoryComponent - Add items", "[inventory]") {
    Inventory inventory;

    SECTION("Add single item to empty inventory") {
        uint32_t remaining = inventory.addItem(1001, 1, 99);
        REQUIRE(remaining == 0); // All items added
        
        REQUIRE(inventory.slots[0].itemId == 1001);
        REQUIRE(inventory.slots[0].quantity == 1);
    }

    SECTION("Stack items when possible") {
        inventory.addItem(1001, 50, 99);
        inventory.addItem(1001, 30, 99);
        
        // Should stack to 80 (not exceed max stack of 99)
        REQUIRE(inventory.slots[0].quantity == 80);
    }

    SECTION("Overflow to new slot when stack full") {
        inventory.addItem(1001, 99, 99);
        uint32_t remaining = inventory.addItem(1001, 5, 99);
        
        // 99 in slot 0, 5 in slot 1
        REQUIRE(inventory.slots[0].quantity == 99);
        REQUIRE(inventory.slots[1].quantity == 5);
        REQUIRE(remaining == 0);
    }

    SECTION("Return remaining when inventory full") {
        // Fill inventory with different items
        for (uint32_t i = 0; i < INVENTORY_SIZE; ++i) {
            inventory.addItem(1000 + i, 1, 99);
        }
        
        // Now try to add another - should fail
        uint32_t remaining = inventory.addItem(9999, 1, 99);
        REQUIRE(remaining == 1); // Couldn't add
    }
}

TEST_CASE("InventoryComponent - Remove items", "[inventory]") {
    Inventory inventory;
    inventory.addItem(1001, 50, 99);

    SECTION("Remove partial quantity") {
        bool success = inventory.removeItem(1001, 20);
        REQUIRE(success == true);
        REQUIRE(inventory.slots[0].quantity == 30);
    }

    SECTION("Remove all items") {
        bool success = inventory.removeItem(1001, 50);
        REQUIRE(success == true);
        REQUIRE(inventory.slots[0].isEmpty() == true);
    }

    SECTION("Remove too many fails gracefully") {
        bool success = inventory.removeItem(1001, 100);
        REQUIRE(success == false); // Can't remove more than we have
        REQUIRE(inventory.slots[0].quantity == 50); // Unchanged
    }

    SECTION("Remove item not in inventory") {
        bool success = inventory.removeItem(9999, 1);
        REQUIRE(success == false);
    }
}

TEST_CASE("InventoryComponent - Query operations", "[inventory]") {
    Inventory inventory;
    inventory.addItem(1001, 30, 99);
    inventory.addItem(1002, 15, 99);
    inventory.addItem(1001, 20, 99); // Stack with first

    SECTION("Count item returns total across all stacks") {
        uint32_t count = inventory.countItem(1001);
        REQUIRE(count == 50); // 30 + 20
    }

    SECTION("Count item not present") {
        uint32_t count = inventory.countItem(9999);
        REQUIRE(count == 0);
    }

    SECTION("Find stackable slot") {
        int32_t slot = inventory.findStackableSlot(1001, 99);
        REQUIRE(slot >= 0); // Should find slot 0
    }
}

TEST_CASE("InventoryComponent - Gold operations", "[inventory]") {
    Inventory inventory;

    SECTION("Add gold") {
        inventory.gold = 100.0f;
        REQUIRE(inventory.gold == 100.0f);
    }

    SECTION("Remove gold") {
        inventory.gold = 100.0f;
        inventory.gold -= 50.0f;
        REQUIRE(inventory.gold == 50.0f);
    }
}

// ============================================================================
// Equipment Component Tests
// ============================================================================

TEST_CASE("EquipmentComponent - Equip items", "[equipment]") {
    Equipment equipment;

    SECTION("Equip main hand") {
        uint32_t previous = equipment.equip(EquipSlot::MainHand, 1001);
        REQUIRE(previous == 0); // Was empty
        REQUIRE(equipment.mainHand == 1001);
    }

    SECTION("Equip off hand") {
        uint32_t previous = equipment.equip(EquipSlot::OffHand, 1002);
        REQUIRE(previous == 0);
        REQUIRE(equipment.offHand == 1002);
    }

    SECTION("Equip head slot") {
        uint32_t previous = equipment.equip(EquipSlot::Head, 2001);
        REQUIRE(previous == 0);
        REQUIRE(equipment.head == 2001);
    }

    SECTION("Replace equipped item returns old") {
        equipment.equip(EquipSlot::MainHand, 1001);
        uint32_t previous = equipment.equip(EquipSlot::MainHand, 1010);
        REQUIRE(previous == 1001); // Returns old item
        REQUIRE(equipment.mainHand == 1010);
    }
}

TEST_CASE("EquipmentComponent - All slots", "[equipment]") {
    Equipment equipment;

    SECTION("All eight slots function") {
        REQUIRE(equipment.equip(EquipSlot::MainHand, 1001) == 0);
        REQUIRE(equipment.equip(EquipSlot::OffHand, 1002) == 0);
        REQUIRE(equipment.equip(EquipSlot::Head, 2001) == 0);
        REQUIRE(equipment.equip(EquipSlot::Chest, 2002) == 0);
        REQUIRE(equipment.equip(EquipSlot::Legs, 2003) == 0);
        REQUIRE(equipment.equip(EquipSlot::Feet, 2004) == 0);
        REQUIRE(equipment.equip(EquipSlot::Ring, 3001) == 0);
        REQUIRE(equipment.equip(EquipSlot::Amulet, 3003) == 0);
    }

    SECTION("All slots have correct items") {
        equipment.equip(EquipSlot::MainHand, 1001);
        equipment.equip(EquipSlot::OffHand, 1002);
        equipment.equip(EquipSlot::Head, 2001);
        equipment.equip(EquipSlot::Chest, 2002);
        equipment.equip(EquipSlot::Legs, 2003);
        equipment.equip(EquipSlot::Feet, 2004);
        equipment.equip(EquipSlot::Ring, 3001);
        equipment.equip(EquipSlot::Amulet, 3003);

        REQUIRE(equipment.mainHand == 1001);
        REQUIRE(equipment.offHand == 1002);
        REQUIRE(equipment.head == 2001);
        REQUIRE(equipment.chest == 2002);
        REQUIRE(equipment.legs == 2003);
        REQUIRE(equipment.feet == 2004);
        REQUIRE(equipment.ring == 3001);
        REQUIRE(equipment.amulet == 3003);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("Inventory - Full workflow", "[inventory][integration]") {
    // Set up inventory with items
    Inventory inventory;
    inventory.addItem(1001, 5, 99);
    inventory.gold = 1000.0f;

    // Add more items
    inventory.addItem(2001, 1, 99);
    inventory.addItem(4001, 3, 5);

    // Verify counts
    REQUIRE(inventory.countItem(1001) == 5);
    REQUIRE(inventory.countItem(2001) == 1);
    REQUIRE(inventory.countItem(4001) == 3);
    REQUIRE(inventory.gold == 1000.0f);

    // Remove some
    inventory.removeItem(4001, 2);
    REQUIRE(inventory.countItem(4001) == 1);

    // Add gold reward
    inventory.gold += 500.0f;
    REQUIRE(inventory.gold == 1500.0f);
}

TEST_CASE("Equipment - Equip workflow", "[equipment][integration]") {
    Equipment equipment;

    // Player starts with basic gear
    equipment.equip(EquipSlot::MainHand, 1001); // Iron Sword
    equipment.equip(EquipSlot::Head, 2001);    // Leather Cap
    
    // Upgrade helmet
    uint32_t oldHelm = equipment.equip(EquipSlot::Head, 2010); // Iron Helm
    REQUIRE(oldHelm == 2001); // Returned old helmet
    REQUIRE(equipment.head == 2010);

    // Verify sword unchanged
    REQUIRE(equipment.mainHand == 1001);
}

TEST_CASE("Inventory - Stack limits by item type", "[inventory]") {
    Inventory inventory;

    SECTION("Weapons stack to 99") {
        uint32_t remaining = inventory.addItem(1001, 150, 99);
        // Maximum 99 in first slot
        REQUIRE(inventory.slots[0].quantity == 99);
        // 51 should overflow to second slot
        REQUIRE(inventory.slots[1].quantity == 51);
        REQUIRE(remaining == 0);
    }

    SECTION("Consumables stack to 5") {
        uint32_t remaining = inventory.addItem(4001, 10, 5);
        REQUIRE(inventory.slots[0].quantity == 5);
        REQUIRE(inventory.slots[1].quantity == 5);
        REQUIRE(remaining == 0);
    }

    SECTION("Materials stack to 20") {
        uint32_t remaining = inventory.addItem(5001, 30, 20);
        REQUIRE(inventory.slots[0].quantity == 20);
        REQUIRE(inventory.slots[1].quantity == 10);
        REQUIRE(remaining == 0);
    }
}