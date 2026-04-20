#include "combat/CraftingSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "ecs/CoreTypes.hpp"

namespace DarkAges {

// ============================================================================
// Recipe Registry
// ============================================================================

void CraftingSystem::registerRecipe(const CraftingRecipe& recipe) {
    if (recipes_.size() >= MAX_RECIPES) return;
    recipes_.push_back(recipe);
}

const CraftingRecipe* CraftingSystem::getRecipe(uint32_t recipeId) const {
    for (const auto& recipe : recipes_) {
        if (recipe.recipeId == recipeId) return &recipe;
    }
    return nullptr;
}

// ============================================================================
// Crafting Actions
// ============================================================================

bool CraftingSystem::startCraft(Registry& registry, EntityID player,
                                uint32_t recipeId, uint32_t currentTimeMs) {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;

    if (!canCraft(registry, player, recipeId)) return false;

    // Consume materials and gold
    if (!consumeMaterials(registry, player, recipeId)) return false;

    // Grant craft start callback
    if (craftStartCallback_) {
        craftStartCallback_(player, recipeId, recipe->craftTimeMs);
    }

    // Get or create crafting component
    CraftingComponent& cc = registry.emplace_or_replace<CraftingComponent>(player);
    cc.startCraft(recipeId, currentTimeMs);

    // Instant craft: complete immediately
    if (recipe->craftTimeMs == 0) {
        grantOutput(registry, player, *recipe);
        cc.finishCraft();
        if (craftCompleteCallback_) {
            craftCompleteCallback_(player, recipeId,
                                    recipe->outputItemId, recipe->outputQuantity);
        }
        return true;
    }

    return true;
}

bool CraftingSystem::cancelCraft(Registry& registry, EntityID player) {
    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    if (!cc || !cc->isCrafting()) return false;
    cc->cancelCraft();
    return true;
}

bool CraftingSystem::updateCraft(Registry& registry, EntityID player,
                                 uint32_t currentTimeMs) {
    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    if (!cc || !cc->isCrafting()) return false;

    const CraftingRecipe* recipe = getRecipe(cc->currentRecipeId);
    if (!recipe) return false;

    // Check if craft time has elapsed
    uint32_t elapsed = currentTimeMs - cc->craftStartTimeMs;
    if (elapsed < recipe->craftTimeMs) return false;

    // Craft complete
    grantOutput(registry, player, *recipe);
    cc->finishCraft();

    if (craftCompleteCallback_) {
        craftCompleteCallback_(player, recipe->recipeId,
                                recipe->outputItemId, recipe->outputQuantity);
    }
    return true;
}

// ============================================================================
// Validation
// ============================================================================

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

bool CraftingSystem::hasRequiredMaterials(const Registry& registry,
                                           EntityID player,
                                           uint32_t recipeId) const {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;

    if (!itemSystem_) return false;

    for (uint32_t i = 0; i < recipe->ingredientCount; ++i) {
        const CraftingIngredient& ing = recipe->ingredients[i];
        if (!itemSystem_->hasInInventory(registry, player, ing.itemId, ing.quantity)) {
            return false;
        }
    }
    return true;
}

bool CraftingSystem::hasRequiredGold(const Registry& registry,
                                      EntityID player,
                                      uint32_t recipeId) const {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;

    if (recipe->goldCost == 0) return true;

    const Inventory* inv = registry.try_get<Inventory>(player);
    if (!inv) return false;
    return inv->gold >= static_cast<float>(recipe->goldCost);
}

// ============================================================================
// Starter Recipes
// ============================================================================

void CraftingSystem::giveStarterRecipes(Registry& registry, EntityID player) {
    // Ensure crafting component exists
    CraftingComponent& cc = registry.emplace_or_replace<CraftingComponent>(player);
    (void)cc; // Mark as used (component auto-initialized)
}

// ============================================================================
// Default Recipe Database
// ============================================================================

void CraftingSystem::initializeDefaults() {
    recipes_.clear();

    // Basic Weapon recipe: Iron Ingot + Oak Wood -> Iron Sword
    {
        CraftingRecipe r;
        r.recipeId = 1;
        snprintf(r.name, sizeof(r.name), "Iron Sword");
        snprintf(r.description, sizeof(r.description), "Forge an iron sword from raw materials.");
        r.outputItemId = 101;
        r.outputQuantity = 1;
        r.ingredients[0] = {1001, 2}; // Iron Ingot x2
        r.ingredients[1] = {1002, 1}; // Oak Wood x1
        r.ingredientCount = 2;
        r.requiredLevel = 1;
        r.requiredProfessionLevel = 1;
        r.craftTimeMs = 5000;
        r.goldCost = 50;
        registerRecipe(r);
    }

    // Health Potion recipe: Herb + Empty Vial -> Health Potion
    {
        CraftingRecipe r;
        r.recipeId = 2;
        snprintf(r.name, sizeof(r.name), "Health Potion");
        snprintf(r.description, sizeof(r.description), "Brew a health potion from herbs.");
        r.outputItemId = 201;
        r.outputQuantity = 1;
        r.ingredients[0] = {1003, 1}; // Herb x1
        r.ingredients[1] = {1004, 1}; // Empty Vial x1
        r.ingredientCount = 2;
        r.requiredLevel = 1;
        r.requiredProfessionLevel = 1;
        r.craftTimeMs = 0; // Instant
        r.goldCost = 10;
        registerRecipe(r);
    }

    // Mana Potion recipe: Blue Mushroom + Empty Vial -> Mana Potion
    {
        CraftingRecipe r;
        r.recipeId = 3;
        snprintf(r.name, sizeof(r.name), "Mana Potion");
        snprintf(r.description, sizeof(r.description), "Brew a mana potion from mystical ingredients.");
        r.outputItemId = 202;
        r.outputQuantity = 1;
        r.ingredients[0] = {1005, 1}; // Blue Mushroom x1
        r.ingredients[1] = {1004, 1}; // Empty Vial x1
        r.ingredientCount = 2;
        r.requiredLevel = 1;
        r.requiredProfessionLevel = 1;
        r.craftTimeMs = 0; // Instant
        r.goldCost = 15;
        registerRecipe(r);
    }

    // Iron Ingot: Iron Ore x2 -> Iron Ingot
    {
        CraftingRecipe r;
        r.recipeId = 4;
        snprintf(r.name, sizeof(r.name), "Iron Ingot");
        snprintf(r.description, sizeof(r.description), "Smelt iron ore into a usable ingot.");
        r.outputItemId = 1001;
        r.outputQuantity = 1;
        r.ingredients[0] = {1006, 2}; // Iron Ore x2
        r.ingredientCount = 1;
        r.requiredLevel = 1;
        r.requiredProfessionLevel = 0;
        r.craftTimeMs = 3000;
        r.goldCost = 5;
        registerRecipe(r);
    }

    // Leather Armor: Raw Hide x3 -> Leather Armor
    {
        CraftingRecipe r;
        r.recipeId = 5;
        snprintf(r.name, sizeof(r.name), "Leather Armor");
        snprintf(r.description, sizeof(r.description), "Craft leather armor from raw hides.");
        r.outputItemId = 102;
        r.outputQuantity = 1;
        r.ingredients[0] = {1007, 3}; // Raw Hide x3
        r.ingredientCount = 1;
        r.requiredLevel = 3;
        r.requiredProfessionLevel = 2;
        r.craftTimeMs = 8000;
        r.goldCost = 100;
        registerRecipe(r);
    }

    // Magic Scroll: Parchment + Blue Mushroom -> Magic Scroll
    {
        CraftingRecipe r;
        r.recipeId = 6;
        snprintf(r.name, sizeof(r.name), "Magic Scroll");
        snprintf(r.description, sizeof(r.description), "Inscribe a magic scroll with mystical power.");
        r.outputItemId = 203;
        r.outputQuantity = 1;
        r.ingredients[0] = {1008, 1}; // Parchment x1
        r.ingredients[1] = {1005, 1}; // Blue Mushroom x1
        r.ingredientCount = 2;
        r.requiredLevel = 5;
        r.requiredProfessionLevel = 3;
        r.craftTimeMs = 10000;
        r.goldCost = 75;
        registerRecipe(r);
    }

    // Enchanted Ring: Silver Ring + Gemstone -> Enchanted Ring
    {
        CraftingRecipe r;
        r.recipeId = 7;
        snprintf(r.name, sizeof(r.name), "Enchanted Ring");
        snprintf(r.description, sizeof(r.description), "Forge an enchanted ring with magical properties.");
        r.outputItemId = 103;
        r.outputQuantity = 1;
        r.ingredients[0] = {1009, 1}; // Silver Ring x1
        r.ingredients[1] = {1010, 1}; // Gemstone x1
        r.ingredientCount = 2;
        r.requiredLevel = 10;
        r.requiredProfessionLevel = 5;
        r.craftTimeMs = 15000;
        r.goldCost = 200;
        registerRecipe(r);
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

bool CraftingSystem::consumeMaterials(Registry& registry, EntityID player,
                                       uint32_t recipeId) {
    const CraftingRecipe* recipe = getRecipe(recipeId);
    if (!recipe) return false;

    if (!itemSystem_) return false;

    // Consume each ingredient
    for (uint32_t i = 0; i < recipe->ingredientCount; ++i) {
        const CraftingIngredient& ing = recipe->ingredients[i];
        if (!itemSystem_->removeFromInventory(registry, player, ing.itemId, ing.quantity)) {
            return false;
        }
    }

    // Consume gold if needed
    if (recipe->goldCost > 0) {
        Inventory* inv = registry.try_get<Inventory>(player);
        if (!inv) return false;
        if (inv->gold < static_cast<float>(recipe->goldCost)) return false;
        inv->gold -= static_cast<float>(recipe->goldCost);
    }

    return true;
}

void CraftingSystem::grantOutput(Registry& registry, EntityID player,
                                  const CraftingRecipe& recipe) {
    if (!itemSystem_) return;

    // Add crafted item to inventory
    itemSystem_->addToInventory(registry, player,
                                  recipe.outputItemId, recipe.outputQuantity);

    // Award profession XP (simplified: 10 XP per craft)
    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    if (cc) {
        cc->professionXP += 10;

        // Level up profession every 100 XP
        constexpr uint64_t XP_PER_LEVEL = 100;
        while (cc->professionXP >= XP_PER_LEVEL) {
            cc->professionXP -= XP_PER_LEVEL;
            if (cc->professionLevel < 255) {
                cc->professionLevel++;
            }
        }
    }
}

} // namespace DarkAges
