#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <vector>

// Crafting System — manages recipes, material consumption, and item production.
// Supports instant and timed crafting, profession leveling, and recipe discovery.

namespace DarkAges {

class ItemSystem;

class CraftingSystem {
public:
    CraftingSystem() = default;

    // --- Recipe Registry ---

    // Register a crafting recipe
    void registerRecipe(const CraftingRecipe& recipe);

    // Get recipe by ID, nullptr if not found
    const CraftingRecipe* getRecipe(uint32_t recipeId) const;

    // Get all registered recipes
    const std::vector<CraftingRecipe>& getAllRecipes() const { return recipes_; }

    // --- Crafting Actions ---

    // Attempt to start crafting a recipe.
    // Checks: player level, profession level, materials, gold, not already crafting.
    // For instant recipes (craftTimeMs=0), completes immediately.
    // Returns true if crafting started (or completed for instant).
    bool startCraft(Registry& registry, EntityID player, uint32_t recipeId,
                    uint32_t currentTimeMs);

    // Cancel current craft in progress. Materials are NOT refunded.
    // Returns true if a craft was cancelled.
    bool cancelCraft(Registry& registry, EntityID player);

    // Check if a timed craft has completed. Call on tick.
    // Returns true if craft finished this tick.
    bool updateCraft(Registry& registry, EntityID player, uint32_t currentTimeMs);

    // --- Validation ---

    // Check if a player meets all requirements for a recipe
    bool canCraft(const Registry& registry, EntityID player, uint32_t recipeId) const;

    // Check if player has required materials
    bool hasRequiredMaterials(const Registry& registry, EntityID player,
                              uint32_t recipeId) const;

    // Check if player has required gold
    bool hasRequiredGold(const Registry& registry, EntityID player,
                         uint32_t recipeId) const;

    // --- Starter Recipes ---

    // Give new player their initial crafting recipes
    void giveStarterRecipes(Registry& registry, EntityID player);

    // --- Default Recipe Database ---

    // Initialize the default recipe database
    void initializeDefaults();

    // --- Dependencies ---

    void setItemSystem(ItemSystem* is) { itemSystem_ = is; }

    // --- Callbacks ---

    using CraftCompleteCallback = std::function<void(EntityID player, uint32_t recipeId,
                                                      uint32_t outputItemId, uint32_t outputQuantity)>;
    void setCraftCompleteCallback(CraftCompleteCallback cb) { craftCompleteCallback_ = std::move(cb); }

    using CraftStartCallback = std::function<void(EntityID player, uint32_t recipeId,
                                                    uint32_t craftTimeMs)>;
    void setCraftStartCallback(CraftStartCallback cb) { craftStartCallback_ = std::move(cb); }

private:
    // Consume materials and gold from player inventory
    bool consumeMaterials(Registry& registry, EntityID player, uint32_t recipeId);

    // Grant output item to player and award profession XP
    void grantOutput(Registry& registry, EntityID player, const CraftingRecipe& recipe);

    // Recipe registry
    static constexpr uint32_t MAX_RECIPES = 512;
    std::vector<CraftingRecipe> recipes_;

    ItemSystem* itemSystem_{nullptr};
    CraftCompleteCallback craftCompleteCallback_;
    CraftStartCallback craftStartCallback_;
};

} // namespace DarkAges
