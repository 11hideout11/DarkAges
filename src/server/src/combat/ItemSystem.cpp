#include "combat/ItemSystem.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace DarkAges {

// ============================================================================
// Item Registry
// ============================================================================

void ItemSystem::registerItem(const ItemDefinition& definition) {
    if (definition.itemId == 0) return;

    // Ensure vector is large enough
    if (definition.itemId >= items_.size()) {
        items_.resize(definition.itemId + 1);
    }

    items_[definition.itemId] = definition;
}

const ItemDefinition* ItemSystem::getItem(uint32_t itemId) const {
    if (itemId == 0 || itemId >= items_.size()) return nullptr;
    const auto& item = items_[itemId];
    if (item.itemId == 0) return nullptr;
    return &item;
}

const ItemDefinition* ItemSystem::getItemByName(const char* name) const {
    if (!name || name[0] == '\0') return nullptr;
    for (const auto& item : items_) {
        if (item.itemId != 0 && std::strncmp(item.name, name, sizeof(item.name)) == 0) {
            return &item;
        }
    }
    return nullptr;
}

// ============================================================================
// Inventory Operations
// ============================================================================

uint32_t ItemSystem::addToInventory(Registry& registry, EntityID entity,
                                    uint32_t itemId, uint32_t quantity) {
    const ItemDefinition* def = getItem(itemId);
    if (!def || quantity == 0) return quantity;

    // Ensure entity has inventory
    if (!registry.all_of<Inventory>(entity)) {
        registry.emplace<Inventory>(entity);
    }

    Inventory& inv = registry.get<Inventory>(entity);
    uint32_t overflow = inv.addItem(itemId, quantity, def->maxStackSize);

    uint32_t added = quantity - overflow;
    if (added > 0 && inventoryChangeCallback_) {
        inventoryChangeCallback_(entity, itemId, added, true);
    }

    return overflow;
}

bool ItemSystem::removeFromInventory(Registry& registry, EntityID entity,
                                     uint32_t itemId, uint32_t quantity) {
    if (quantity == 0) return true;

    Inventory* inv = registry.try_get<Inventory>(entity);
    if (!inv) return false;

    bool success = inv->removeItem(itemId, quantity);
    if (success && inventoryChangeCallback_) {
        inventoryChangeCallback_(entity, itemId, quantity, false);
    }
    return success;
}

bool ItemSystem::hasInInventory(const Registry& registry, EntityID entity,
                                uint32_t itemId, uint32_t quantity) const {
    const Inventory* inv = registry.try_get<Inventory>(entity);
    if (!inv) return false;
    return inv->countItem(itemId) >= quantity;
}

uint32_t ItemSystem::countInInventory(const Registry& registry, EntityID entity,
                                      uint32_t itemId) const {
    const Inventory* inv = registry.try_get<Inventory>(entity);
    if (!inv) return 0;
    return inv->countItem(itemId);
}

// ============================================================================
// Equipment
// ============================================================================

bool ItemSystem::equipItem(Registry& registry, EntityID entity, uint32_t itemId) {
    const ItemDefinition* def = getItem(itemId);
    if (!def) return false;

    // Must have an equip slot
    if (def->equipSlot == EquipSlot::None) return false;

    // Must be in inventory
    if (!hasInInventory(registry, entity, itemId, 1)) return false;

    // Ensure equipment component
    if (!registry.all_of<Equipment>(entity)) {
        registry.emplace<Equipment>(entity);
    }

    Equipment& equip = registry.get<Equipment>(entity);

    // Unequip current item in that slot (put back in inventory)
    uint32_t oldItem = equip.equip(def->equipSlot, itemId);
    if (oldItem != 0) {
        addToInventory(registry, entity, oldItem, 1);
    }

    // Remove from inventory
    removeFromInventory(registry, entity, itemId, 1);

    return true;
}

bool ItemSystem::unequipItem(Registry& registry, EntityID entity, EquipSlot slot) {
    Equipment* equip = registry.try_get<Equipment>(entity);
    if (!equip) return false;

    uint32_t itemId = 0;
    switch (slot) {
        case EquipSlot::MainHand: itemId = equip->mainHand; equip->mainHand = 0; break;
        case EquipSlot::OffHand:  itemId = equip->offHand; equip->offHand = 0; break;
        case EquipSlot::Head:     itemId = equip->head; equip->head = 0; break;
        case EquipSlot::Chest:    itemId = equip->chest; equip->chest = 0; break;
        case EquipSlot::Legs:     itemId = equip->legs; equip->legs = 0; break;
        case EquipSlot::Feet:     itemId = equip->feet; equip->feet = 0; break;
        case EquipSlot::Ring:     itemId = equip->ring; equip->ring = 0; break;
        case EquipSlot::Amulet:   itemId = equip->amulet; equip->amulet = 0; break;
        default: return false;
    }

    if (itemId == 0) return false; // Nothing equipped

    // Put back in inventory
    addToInventory(registry, entity, itemId, 1);
    return true;
}

ItemStats ItemSystem::getEquippedStats(const Registry& registry, EntityID entity) const {
    ItemStats total;
    const Equipment* equip = registry.try_get<Equipment>(entity);
    if (!equip) return total;

    // Sum stats from all equipped items
    uint32_t slots[] = {
        equip->mainHand, equip->offHand, equip->head, equip->chest,
        equip->legs, equip->feet, equip->ring, equip->amulet
    };

    for (uint32_t itemId : slots) {
        if (itemId == 0) continue;
        const ItemDefinition* def = getItem(itemId);
        if (!def) continue;

        total.damageBonus += def->stats.damageBonus;
        total.healthBonus += def->stats.healthBonus;
        total.manaBonus += def->stats.manaBonus;
        total.speedBonus += def->stats.speedBonus;
        total.critChanceBonus += def->stats.critChanceBonus;
    }

    return total;
}

// ============================================================================
// Consumable Use
// ============================================================================

bool ItemSystem::useItem(Registry& registry, EntityID entity, uint32_t inventorySlot) {
    // Validate inventory slot (1-indexed to match client input)
    if (inventorySlot == 0 || inventorySlot > INVENTORY_SIZE) return false;

    // Get inventory
    Inventory* inv = registry.try_get<Inventory>(entity);
    if (!inv) return false;

    // Get the slot (convert to 0-indexed)
    uint32_t slotIdx = inventorySlot - 1;
    InventorySlot& slot = inv->slots[slotIdx];
    if (slot.isEmpty()) return false;

    // Get item definition
    const ItemDefinition* def = getItem(slot.itemId);
    if (!def) return false;

    // Only consumables can be used this way
    if (def->type != ItemType::Consumable) return false;

    // Check level requirement
    const PlayerProgression* prog = registry.try_get<PlayerProgression>(entity);
    if (prog && def->requiredLevel > prog->level) return false;

    // Apply consumable effects
    bool applied = false;

    // Health restoration
    if (def->stats.healthBonus > 0) {
        CombatState* combat = registry.try_get<CombatState>(entity);
        if (combat && !combat->isDead) {
            int16_t healAmount = def->stats.healthBonus;
            int16_t oldHealth = combat->health;
            combat->health = std::min(static_cast<int16_t>(combat->health + healAmount),
                                      combat->maxHealth);
            int16_t actualHeal = combat->health - oldHealth;
            if (actualHeal > 0) {
                applied = true;
            }
        }
    }

    // Mana restoration
    if (def->stats.manaBonus > 0) {
        Mana* mana = registry.try_get<Mana>(entity);
        if (mana) {
            float oldMana = mana->current;
            mana->current = std::min(mana->current + static_cast<float>(def->stats.manaBonus),
                                     mana->max);
            float actualRestore = mana->current - oldMana;
            if (actualRestore > 0.0f) {
                applied = true;
            }
        }
    }

    // If nothing was applied (full health/mana), still allow use but don't consume
    // Actually, let's consume it anyway — the player chose to use it
    if (!applied) {
        // Player is already at full health AND mana — don't waste the potion
        return false;
    }

    // Remove one from the stack
    slot.quantity--;
    if (slot.quantity == 0) {
        slot.itemId = 0;
    }

    // Fire inventory change callback
    if (inventoryChangeCallback_) {
        inventoryChangeCallback_(entity, def->itemId, 1, false);
    }

    return true;
}

// ============================================================================
// Starter Kit
// ============================================================================

void ItemSystem::giveStarterKit(Registry& registry, EntityID entity) {
    // Give basic starting gear
    addToInventory(registry, entity, 1, 1);   // Rusty Sword
    addToInventory(registry, entity, 2, 1);   // Leather Tunic
    addToInventory(registry, entity, 10, 5);  // Health Potion x5
    addToInventory(registry, entity, 11, 3);  // Mana Potion x3

    // Give starter abilities
    AbilityLoadout loadout;
    loadout.abilityIds[0] = 1;  // Fireball
    loadout.abilityIds[1] = 2;  // Heal
    loadout.abilityIds[2] = 3;  // Power Strike
    loadout.abilityIds[3] = 0;  // Empty slot
    registry.emplace_or_replace<AbilityLoadout>(entity, loadout);

    // Give starting gold
    if (Inventory* inv = registry.try_get<Inventory>(entity)) {
        inv->gold = 100.0f;
    }

    // Equip starter weapon
    equipItem(registry, entity, 1);
    equipItem(registry, entity, 2);
}

// ============================================================================
// Default Item Database
// ============================================================================

void ItemSystem::initializeDefaults() {
    items_.clear();
    items_.resize(100); // Reserve space for starter items

    // --- Weapons ---
    {
        ItemDefinition item{};
        item.itemId = 1;
        std::strncpy(item.name, "Rusty Sword", sizeof(item.name) - 1);
        std::strncpy(item.description, "A worn but serviceable blade.", sizeof(item.description) - 1);
        item.type = ItemType::Weapon;
        item.rarity = ItemRarity::Common;
        item.equipSlot = EquipSlot::MainHand;
        item.stats.damageBonus = 5;
        item.maxStackSize = 1;
        item.buyPrice = 50;
        item.sellPrice = 10;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 3;
        std::strncpy(item.name, "Iron Longsword", sizeof(item.name) - 1);
        std::strncpy(item.description, "A well-forged iron blade.", sizeof(item.description) - 1);
        item.type = ItemType::Weapon;
        item.rarity = ItemRarity::Uncommon;
        item.equipSlot = EquipSlot::MainHand;
        item.stats.damageBonus = 12;
        item.maxStackSize = 1;
        item.buyPrice = 200;
        item.sellPrice = 50;
        item.requiredLevel = 5;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 4;
        std::strncpy(item.name, "Flame Staff", sizeof(item.name) - 1);
        std::strncpy(item.description, "A staff imbued with fire magic.", sizeof(item.description) - 1);
        item.type = ItemType::Weapon;
        item.rarity = ItemRarity::Rare;
        item.equipSlot = EquipSlot::MainHand;
        item.stats.damageBonus = 8;
        item.stats.manaBonus = 30;
        item.maxStackSize = 1;
        item.buyPrice = 500;
        item.sellPrice = 125;
        item.requiredLevel = 10;
        registerItem(item);
    }

    // --- Armor ---
    {
        ItemDefinition item{};
        item.itemId = 2;
        std::strncpy(item.name, "Leather Tunic", sizeof(item.name) - 1);
        std::strncpy(item.description, "Basic leather protection.", sizeof(item.description) - 1);
        item.type = ItemType::Armor;
        item.rarity = ItemRarity::Common;
        item.equipSlot = EquipSlot::Chest;
        item.stats.healthBonus = 50;
        item.maxStackSize = 1;
        item.buyPrice = 30;
        item.sellPrice = 5;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 5;
        std::strncpy(item.name, "Iron Helmet", sizeof(item.name) - 1);
        std::strncpy(item.description, "A sturdy iron helm.", sizeof(item.description) - 1);
        item.type = ItemType::Armor;
        item.rarity = ItemRarity::Uncommon;
        item.equipSlot = EquipSlot::Head;
        item.stats.healthBonus = 30;
        item.maxStackSize = 1;
        item.buyPrice = 150;
        item.sellPrice = 35;
        item.requiredLevel = 3;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 6;
        std::strncpy(item.name, "Chain Boots", sizeof(item.name) - 1);
        std::strncpy(item.description, "Linked chainmail boots.", sizeof(item.description) - 1);
        item.type = ItemType::Armor;
        item.rarity = ItemRarity::Common;
        item.equipSlot = EquipSlot::Feet;
        item.stats.healthBonus = 20;
        item.stats.speedBonus = 0.05f;
        item.maxStackSize = 1;
        item.buyPrice = 80;
        item.sellPrice = 15;
        registerItem(item);
    }

    // --- Accessories ---
    {
        ItemDefinition item{};
        item.itemId = 7;
        std::strncpy(item.name, "Silver Ring", sizeof(item.name) - 1);
        std::strncpy(item.description, "A simple silver band.", sizeof(item.description) - 1);
        item.type = ItemType::Accessory;
        item.rarity = ItemRarity::Uncommon;
        item.equipSlot = EquipSlot::Ring;
        item.stats.manaBonus = 15;
        item.maxStackSize = 1;
        item.buyPrice = 100;
        item.sellPrice = 25;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 8;
        std::strncpy(item.name, "Lucky Amulet", sizeof(item.name) - 1);
        std::strncpy(item.description, "An amulet said to bring fortune.", sizeof(item.description) - 1);
        item.type = ItemType::Accessory;
        item.rarity = ItemRarity::Rare;
        item.equipSlot = EquipSlot::Amulet;
        item.stats.critChanceBonus = 0.05f;
        item.maxStackSize = 1;
        item.buyPrice = 300;
        item.sellPrice = 75;
        item.requiredLevel = 5;
        registerItem(item);
    }

    // --- Consumables ---
    {
        ItemDefinition item{};
        item.itemId = 10;
        std::strncpy(item.name, "Health Potion", sizeof(item.name) - 1);
        std::strncpy(item.description, "Restores 50 health.", sizeof(item.description) - 1);
        item.type = ItemType::Consumable;
        item.rarity = ItemRarity::Common;
        item.stats.healthBonus = 50;
        item.maxStackSize = 20;
        item.buyPrice = 25;
        item.sellPrice = 5;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 11;
        std::strncpy(item.name, "Mana Potion", sizeof(item.name) - 1);
        std::strncpy(item.description, "Restores 30 mana.", sizeof(item.description) - 1);
        item.type = ItemType::Consumable;
        item.rarity = ItemRarity::Common;
        item.stats.manaBonus = 30;
        item.maxStackSize = 20;
        item.buyPrice = 20;
        item.sellPrice = 4;
        registerItem(item);
    }

    // --- Materials ---
    {
        ItemDefinition item{};
        item.itemId = 20;
        std::strncpy(item.name, "Wolf Pelt", sizeof(item.name) - 1);
        std::strncpy(item.description, "A thick wolf hide.", sizeof(item.description) - 1);
        item.type = ItemType::Material;
        item.rarity = ItemRarity::Common;
        item.maxStackSize = 50;
        item.buyPrice = 5;
        item.sellPrice = 2;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 21;
        std::strncpy(item.name, "Iron Ore", sizeof(item.name) - 1);
        std::strncpy(item.description, "Raw iron ore for smelting.", sizeof(item.description) - 1);
        item.type = ItemType::Material;
        item.rarity = ItemRarity::Common;
        item.maxStackSize = 100;
        item.buyPrice = 3;
        item.sellPrice = 1;
        registerItem(item);
    }
    {
        ItemDefinition item{};
        item.itemId = 22;
        std::strncpy(item.name, "Ancient Relic Shard", sizeof(item.name) - 1);
        std::strncpy(item.description, "A fragment of a powerful artifact.", sizeof(item.description) - 1);
        item.type = ItemType::Material;
        item.rarity = ItemRarity::Epic;
        item.maxStackSize = 10;
        item.buyPrice = 1000;
        item.sellPrice = 250;
        item.requiredLevel = 15;
        item.tradable = false;
        registerItem(item);
    }

    // --- Legendary ---
    {
        ItemDefinition item{};
        item.itemId = 30;
        std::strncpy(item.name, "Vorpal Blade", sizeof(item.name) - 1);
        std::strncpy(item.description, "A legendary sword that cuts through anything.", sizeof(item.description) - 1);
        item.type = ItemType::Weapon;
        item.rarity = ItemRarity::Legendary;
        item.equipSlot = EquipSlot::MainHand;
        item.stats.damageBonus = 50;
        item.stats.critChanceBonus = 0.15f;
        item.maxStackSize = 1;
        item.buyPrice = 10000;
        item.sellPrice = 2500;
        item.requiredLevel = 25;
        item.tradable = false;
        registerItem(item);
    }

    std::cout << "[ITEMS] Initialized " << items_.size() << " item definitions" << std::endl;
}

} // namespace DarkAges
