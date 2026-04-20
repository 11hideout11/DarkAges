// Crafting System implementation
// Manages recipe registry, material consumption, and item production

#include "combat/CraftingSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "ecs/Components.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace DarkAges {

void CraftingSystem::registerRecipe(const CraftingRecipe& recipe) {
    if (recipes_.size() >= MAX_RECIPES) {
        std::cerr << "[CRAFTING] Recipe registry full, cannot register recipe "
                  << recipe.recipeId << std::endl;
        return;
    }

    // Check for duplicate IDs
    for (const auto& r : recipes_) {
        if (r.recipeId == recipe.recipeId) {
            std::cerr << "[CRAFTING] Duplicate recipe ID: " << recipe.recipeId << std::endl;
            return;
        }
    }

    recipes_.push_back(recipe);
}

const CraftingRecipe* CraftingSystem::getRecipe(uint32_t recipeId) const {
    for (const auto& r : recipes_) {
        if (r.recipeId == recipeId) return &r;
    }
    return nullptr;
}

bool CraftingSystem::canCraft(const Registry& registry, EntityID player,
                               uint32_t recipeId) const {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;

    // Check player level
    const PlayerProgression* prog = registry.try_get<PlayerProgression>(player);
    if (!prog) return false;
    if (prog->level < recipe->requiredLevel) return false;

    // Check profession level
    const CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    if (recipe->requiredProfessionLevel > 0) {
        if (!cc) return false;
        if (cc->professionLevel < static_cast<uint8_t>(recipe->requiredProfessionLevel)) {
            return false;
        }
    }

    // Check materials and gold
    if (!hasRequiredMaterials(registry, player, recipeId)) return false;
    if (!hasRequiredGold(registry, player, recipeId)) return false;

    // Check not already crafting
    if (cc && cc->isCrafting()) return false;

    return true;
}

bool CraftingSystem::hasRequiredMaterials(const Registry& registry, EntityID player,
                                           uint32_t recipeId) const {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;

    if (!itemSystem_) return false;

    for (uint32_t i = 0; i < recipe->ingredientCount; ++i) {
        const auto& ingredient = recipe->ingredients[i];
        uint32_t has = itemSystem_->countInInventory(registry, player, ingredient.itemId);
        if (has < ingredient.quantity) return false;
    }

    return true;
}

bool CraftingSystem::hasRequiredGold(const Registry& registry, EntityID player,
                                      uint32_t recipeId) const {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;
    if (recipe->goldCost == 0) return true;

    const Inventory* inv = registry.try_get<Inventory>(player);
    if (!inv) return false;

    return inv->gold >= static_cast<float>(recipe->goldCost);
}

bool CraftingSystem::startCraft(Registry& registry, EntityID player,
                                 uint32_t recipeId, uint32_t currentTimeMs) {
    if (!canCraft(registry, player, recipeId)) return false;

    const CraftingRecipe* recipe = getRecipe(recipeId);

    // Consume materials
    if (!consumeMaterials(registry, player, recipeId)) return false;

    // Ensure CraftingComponent exists BEFORE grantOutput (which may invalidate pointers)
    if (!registry.all_of<CraftingComponent>(player)) {
        registry.emplace<CraftingComponent>(player);
    }

    // Instant craft — complete immediately
    if (recipe->craftTimeMs == 0) {
        grantOutput(registry, player, *recipe);

        // Re-fetch component after grantOutput (pointer may be stale)
        CraftingComponent& crafting = registry.get<CraftingComponent>(player);
        crafting.finishCraft();

        std::cout << "[CRAFTING] Player " << static_cast<uint32_t>(player)
                  << " crafted " << recipe->outputQuantity << "x '" << recipe->name
                  << "' (instant)" << std::endl;

        if (craftCompleteCallback_) {
            craftCompleteCallback_(player, recipeId, recipe->outputItemId,
                                    recipe->outputQuantity);
        }
        return true;
    }

    // Timed craft — start progress (component already exists from above)
    CraftingComponent& crafting = registry.get<CraftingComponent>(player);
    crafting.startCraft(recipeId, currentTimeMs);

    std::cout << "[CRAFTING] Player " << static_cast<uint32_t>(player)
              << " started crafting '" << recipe->name << "' ("
              << recipe->craftTimeMs << "ms)" << std::endl;

    if (craftStartCallback_) {
        craftStartCallback_(player, recipeId, recipe->craftTimeMs);
    }

    return true;
}

bool CraftingSystem::cancelCraft(Registry& registry, EntityID player) {
    CraftingComponent* crafting = registry.try_get<CraftingComponent>(player);
    if (!crafting || !crafting->isCrafting()) return false;

    uint32_t recipeId = crafting->currentRecipeId;
    crafting->cancelCraft();

    std::cout << "[CRAFTING] Player " << static_cast<uint32_t>(player)
              << " cancelled craft (recipe " << recipeId << ") — materials lost"
              << std::endl;

    return true;
}

bool CraftingSystem::updateCraft(Registry& registry, EntityID player,
                                  uint32_t currentTimeMs) {
    CraftingComponent* crafting = registry.try_get<CraftingComponent>(player);
    if (!crafting || !crafting->isCrafting()) return false;

    const CraftingRecipe* recipe = getRecipe(crafting->currentRecipeId);
    if (!recipe) {
        // Recipe was removed — cancel
        crafting->cancelCraft();
        return false;
    }

    uint32_t elapsed = currentTimeMs - crafting->craftStartTimeMs;
    if (elapsed < recipe->craftTimeMs) return false; // Still crafting

    // Craft complete!
    uint32_t recipeId = crafting->currentRecipeId;
    crafting->finishCraft();
    grantOutput(registry, player, *recipe);

    std::cout << "[CRAFTING] Player " << static_cast<uint32_t>(player)
              << " completed crafting " << recipe->outputQuantity << "x '"
              << recipe->name << "'" << std::endl;

    if (craftCompleteCallback_) {
        craftCompleteCallback_(player, recipeId, recipe->outputItemId,
                                recipe->outputQuantity);
    }

    return true;
}

bool CraftingSystem::consumeMaterials(Registry& registry, EntityID player,
                                       uint32_t recipeId) {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe || !itemSystem_) return false;

    // Consume ingredients
    for (uint32_t i = 0; i < recipe->ingredientCount; ++i) {
        const auto& ingredient = recipe->ingredients[i];
        if (!itemSystem_->removeFromInventory(registry, player,
                                               ingredient.itemId, ingredient.quantity)) {
            std::cerr << "[CRAFTING] Failed to consume material item "
                      << ingredient.itemId << " x" << ingredient.quantity << std::endl;
            return false;
        }
    }

    // Consume gold
    if (recipe->goldCost > 0) {
        Inventory* inv = registry.try_get<Inventory>(player);
        if (!inv || inv->gold < static_cast<float>(recipe->goldCost)) return false;
        inv->gold -= static_cast<float>(recipe->goldCost);
    }

    return true;
}

void CraftingSystem::grantOutput(Registry& registry, EntityID player,
                                  const CraftingRecipe& recipe) {
    if (itemSystem_) {
        itemSystem_->addToInventory(registry, player,
                                     recipe.outputItemId, recipe.outputQuantity);
    }

    // Award profession XP (10 XP per craft + 5 per ingredient)
    // Use registry.get (not try_get) since component must exist by now
    if (registry.all_of<CraftingComponent>(player)) {
        CraftingComponent& crafting = registry.get<CraftingComponent>(player);
        uint64_t xpGain = 10 + (recipe.ingredientCount * 5);
        crafting.professionXP += xpGain;

        // Level up: every 100 XP * current level
        uint64_t xpNeeded = 100 * (static_cast<uint64_t>(crafting.professionLevel) + 1);
        if (crafting.professionXP >= xpNeeded && crafting.professionLevel < 255) {
            crafting.professionLevel++;
            crafting.professionXP -= xpNeeded;
            std::cout << "[CRAFTING] Player " << static_cast<uint32_t>(player)
                      << " profession leveled up to " << static_cast<int>(crafting.professionLevel)
                      << std::endl;
        }
    }
}

void CraftingSystem::giveStarterRecipes(Registry& registry, EntityID player) {
    // Ensure player has a CraftingComponent
    if (!registry.try_get<CraftingComponent>(player)) {
        registry.emplace<CraftingComponent>(player);
    }
}

void CraftingSystem::initializeDefaults() {
    recipes_.clear();

    // --- Starter Recipes ---

    // Recipe 1: Iron Sword (2x Iron Ore + 1x Wolf Pelt -> Iron Sword)
    {
        CraftingRecipe recipe{};
        recipe.recipeId = 1;
        std::strncpy(recipe.name, "Forge Iron Sword", sizeof(recipe.name) - 1);
        std::strncpy(recipe.description, "Smelt iron ore into a sturdy blade.",
                      sizeof(recipe.description) - 1);
        recipe.outputItemId = 10;     // Iron Sword (from ItemSystem defaults)
        recipe.outputQuantity = 1;
        recipe.ingredients[0] = {21, 2}; // 2x Iron Ore
        recipe.ingredients[1] = {20, 1}; // 1x Wolf Pelt (leather grip)
        recipe.ingredientCount = 2;
        recipe.requiredLevel = 1;
        recipe.craftTimeMs = 3000;    // 3 seconds
        recipe.goldCost = 10;
        registerRecipe(recipe);
    }

    // Recipe 2: Healing Potion (1x Wolf Pelt + 1x Iron Ore -> Healing Potion x2)
    {
        CraftingRecipe recipe{};
        recipe.recipeId = 2;
        std::strncpy(recipe.name, "Brew Healing Potion", sizeof(recipe.name) - 1);
        std::strncpy(recipe.description, "Combine natural ingredients into a restorative brew.",
                      sizeof(recipe.description) - 1);
        recipe.outputItemId = 1;      // Healing Potion (from ItemSystem defaults)
        recipe.outputQuantity = 2;    // Produces 2 potions
        recipe.ingredients[0] = {20, 1}; // 1x Wolf Pelt
        recipe.ingredients[1] = {21, 1}; // 1x Iron Ore (as alchemical base)
        recipe.ingredientCount = 2;
        recipe.requiredLevel = 1;
        recipe.craftTimeMs = 2000;    // 2 seconds
        recipe.goldCost = 5;
        registerRecipe(recipe);
    }

    // Recipe 3: Reinforced Armor (3x Iron Ore + 1x Wolf Pelt -> Iron Chestplate)
    {
        CraftingRecipe recipe{};
        recipe.recipeId = 3;
        std::strncpy(recipe.name, "Forge Iron Chestplate", sizeof(recipe.name) - 1);
        std::strncpy(recipe.description, "Hammer iron plates into protective armor.",
                      sizeof(recipe.description) - 1);
        recipe.outputItemId = 11;     // Iron Chestplate (from ItemSystem defaults)
        recipe.outputQuantity = 1;
        recipe.ingredients[0] = {21, 3}; // 3x Iron Ore
        recipe.ingredients[1] = {20, 1}; // 1x Wolf Pelt (leather straps)
        recipe.ingredientCount = 2;
        recipe.requiredLevel = 5;
        recipe.requiredProfessionLevel = 2;
        recipe.craftTimeMs = 5000;    // 5 seconds
        recipe.goldCost = 25;
        registerRecipe(recipe);
    }

    // Recipe 4: Mana Potion (instant, cheap)
    {
        CraftingRecipe recipe{};
        recipe.recipeId = 4;
        std::strncpy(recipe.name, "Brew Mana Potion", sizeof(recipe.name) - 1);
        std::strncpy(recipe.description, "Distill raw materials into a mana-restoring elixir.",
                      sizeof(recipe.description) - 1);
        recipe.outputItemId = 2;      // Mana Potion (from ItemSystem defaults)
        recipe.outputQuantity = 1;
        recipe.ingredients[0] = {21, 1}; // 1x Iron Ore
        recipe.ingredientCount = 1;
        recipe.requiredLevel = 1;
        recipe.craftTimeMs = 0;       // Instant
        recipe.goldCost = 3;
        registerRecipe(recipe);
    }

    // Recipe 5: Legendary Forge (Ancient Relic + 5x Iron Ore -> Relic Blade)
    {
        CraftingRecipe recipe{};
        recipe.recipeId = 5;
        std::strncpy(recipe.name, "Forge Relic Blade", sizeof(recipe.name) - 1);
        std::strncpy(recipe.description, "Infuse ancient power into a masterwork weapon.",
                      sizeof(recipe.description) - 1);
        recipe.outputItemId = 30;     // Vorpal Blade (legendary)
        recipe.outputQuantity = 1;
        recipe.ingredients[0] = {22, 1}; // 1x Ancient Relic Shard
        recipe.ingredients[1] = {21, 5}; // 5x Iron Ore
        recipe.ingredientCount = 2;
        recipe.requiredLevel = 20;
        recipe.requiredProfessionLevel = 5;
        recipe.craftTimeMs = 10000;   // 10 seconds
        recipe.goldCost = 500;
        recipe.discovered = false;    // Hidden until learned
        registerRecipe(recipe);
    }

    std::cout << "[CRAFTING] Initialized " << recipes_.size() << " crafting recipes" << std::endl;
}

} // namespace DarkAges
