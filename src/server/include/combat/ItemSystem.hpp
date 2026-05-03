#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <vector>

// Item System — manages item definitions, inventory, and equipment
// Provides a registry of all items in the game and handles
// inventory management (add/remove/equip/unequip)

namespace DarkAges {

class ItemSystem {
public:
    ItemSystem() = default;

    // --- Item Registry ---

    // Register an item definition
    void registerItem(const ItemDefinition& definition);

    // Get item definition by ID, nullptr if not found
    const ItemDefinition* getItem(uint32_t itemId) const;

    // Get item definition by name, nullptr if not found
    const ItemDefinition* getItemByName(const char* name) const;

    // Check if an item exists
    bool hasItem(uint32_t itemId) const { return getItem(itemId) != nullptr; }

    // --- Inventory Operations ---

    // Add item to entity's inventory (creates Inventory component if missing)
    // Returns number of items that couldn't fit (0 = all added)
    uint32_t addToInventory(Registry& registry, EntityID entity,
                           uint32_t itemId, uint32_t quantity);

    // Remove item from entity's inventory
    // Returns true if successful (items were removed)
    bool removeFromInventory(Registry& registry, EntityID entity,
                            uint32_t itemId, uint32_t quantity);

    // Check if entity has enough of an item
    bool hasInInventory(const Registry& registry, EntityID entity,
                       uint32_t itemId, uint32_t quantity) const;

    // Count items in inventory
    uint32_t countInInventory(const Registry& registry, EntityID entity,
                             uint32_t itemId) const;

    // --- Equipment ---

    // Equip item from inventory slot
    // Returns true if successful
    bool equipItem(Registry& registry, EntityID entity, uint32_t itemId);

    // Unequip item to inventory
    // Returns true if successful
    bool unequipItem(Registry& registry, EntityID entity, EquipSlot slot);

    // Get total stat bonuses from equipped items
    ItemStats getEquippedStats(const Registry& registry, EntityID entity) const;

    // --- Consumable Use ---

    // Use a consumable item from inventory (potions, food, etc.)
    // Applies instant effects (health/mana restoration), removes 1 from stack.
    // Returns true if item was used successfully.
    bool useItem(Registry& registry, EntityID entity, uint32_t inventorySlot);

    // --- Starter Kit ---

    // Give a new player their starting gear and abilities
    void giveStarterKit(Registry& registry, EntityID entity);

    // --- Predefined Items ---

    // Initialize the default item database
    void initializeDefaults();

    // --- Callbacks ---

    using InventoryChangeCallback = std::function<void(EntityID entity, uint32_t itemId,
                                                        uint32_t quantity, bool added)>;
    void setInventoryChangeCallback(InventoryChangeCallback cb) {
        inventoryChangeCallback_ = std::move(cb);
    }

    // Set the progression calculator to recalculate stats on equipment changes
    void setProgressionCalculator(class ProgressionCalculator* calc) {
        progressionCalculator_ = calc;
    }

private:
    // Item registry — flat array indexed by item ID
    // Max 4096 items in the game
    static constexpr uint32_t MAX_ITEMS = 4096;
    std::vector<ItemDefinition> items_;

    InventoryChangeCallback inventoryChangeCallback_;
    ProgressionCalculator* progressionCalculator_ = nullptr;
};

} // namespace DarkAges
