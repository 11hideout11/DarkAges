// CraftingSystem unit tests
// Tests recipe registration, crafting logic, material consumption, profession XP

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/CraftingSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "ecs/Components.hpp"

using namespace DarkAges;
using Catch::Approx;

// Helper: create a test player with inventory and crafting state
static EntityID createTestPlayer(Registry& registry, uint32_t level = 1) {
    auto entity = registry.create();
    registry.emplace<Position>(entity);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);

    PlayerInfo info{};
    info.playerId = static_cast<uint64_t>(entity);
    info.connectionId = static_cast<uint32_t>(entity) + 1000;
    registry.emplace<PlayerInfo>(entity, info);

    PlayerProgression prog{};
    prog.level = level;
    registry.emplace<PlayerProgression>(entity, prog);

    registry.emplace<Inventory>(entity);
    registry.emplace<CraftingComponent>(entity);

    return entity;
}

// Helper: setup a CraftingSystem with ItemSystem and some recipes
static void setupSystems(CraftingSystem& crafting, ItemSystem& items) {
    items.initializeDefaults();
    crafting.initializeDefaults();
    crafting.setItemSystem(&items);
}

// ============================================================================
// TEST: Crafting Types
// ============================================================================

TEST_CASE("CraftingComponent default construction", "[crafting]") {
    CraftingComponent cc;
    CHECK(cc.currentRecipeId == 0);
    CHECK(cc.craftStartTimeMs == 0);
    CHECK(cc.craftCount == 0);
    CHECK(cc.professionLevel == 0);
    CHECK(cc.professionXP == 0);
    CHECK_FALSE(cc.isCrafting());
}

TEST_CASE("CraftingComponent start/finish/cancel", "[crafting]") {
    CraftingComponent cc;

    cc.startCraft(1, 1000);
    CHECK(cc.isCrafting());
    CHECK(cc.currentRecipeId == 1);
    CHECK(cc.craftStartTimeMs == 1000);

    cc.finishCraft();
    CHECK_FALSE(cc.isCrafting());
    CHECK(cc.currentRecipeId == 0);
    CHECK(cc.craftStartTimeMs == 0);
    CHECK(cc.craftCount == 1);

    cc.startCraft(2, 2000);
    CHECK(cc.isCrafting());
    cc.cancelCraft();
    CHECK_FALSE(cc.isCrafting());
    CHECK(cc.currentRecipeId == 0);
    // cancel doesn't increment craftCount
    CHECK(cc.craftCount == 1);
}

TEST_CASE("CraftingRecipe default construction", "[crafting]") {
    CraftingRecipe recipe;
    CHECK(recipe.recipeId == 0);
    CHECK(recipe.outputItemId == 0);
    CHECK(recipe.outputQuantity == 1);
    CHECK(recipe.ingredientCount == 0);
    CHECK(recipe.requiredLevel == 1);
    CHECK(recipe.requiredProfessionLevel == 0);
    CHECK(recipe.craftTimeMs == 0);
    CHECK(recipe.goldCost == 0);
    CHECK(recipe.discovered == true);
}

TEST_CASE("CraftingIngredient default construction", "[crafting]") {
    CraftingIngredient ing;
    CHECK(ing.itemId == 0);
    CHECK(ing.quantity == 1);
}

// ============================================================================
// TEST: Recipe Registry
// ============================================================================

TEST_CASE("CraftingSystem register and lookup recipes", "[crafting]") {
    CraftingSystem crafting;

    CraftingRecipe recipe{};
    recipe.recipeId = 100;
    std::strncpy(recipe.name, "Test Recipe", sizeof(recipe.name) - 1);
    recipe.outputItemId = 50;
    recipe.outputQuantity = 2;
    recipe.ingredients[0] = {10, 3};
    recipe.ingredientCount = 1;

    crafting.registerRecipe(recipe);

    const CraftingRecipe* found = crafting.getRecipe(100);
    REQUIRE(found != nullptr);
    CHECK(found->recipeId == 100);
    CHECK(std::string(found->name) == "Test Recipe");
    CHECK(found->outputItemId == 50);
    CHECK(found->outputQuantity == 2);
    CHECK(found->ingredients[0].itemId == 10);
    CHECK(found->ingredients[0].quantity == 3);
    CHECK(found->ingredientCount == 1);

    // Non-existent recipe
    CHECK(crafting.getRecipe(999) == nullptr);
}

TEST_CASE("CraftingSystem duplicate recipe ID rejected", "[crafting]") {
    CraftingSystem crafting;

    CraftingRecipe r1{};
    r1.recipeId = 1;
    crafting.registerRecipe(r1);

    CraftingRecipe r2{};
    r2.recipeId = 1; // Same ID
    r2.outputItemId = 99;
    crafting.registerRecipe(r2);

    // Should only have the first one
    const CraftingRecipe* found = crafting.getRecipe(1);
    REQUIRE(found != nullptr);
    CHECK(found->outputItemId == 0); // First recipe, not second
}

TEST_CASE("CraftingSystem getAllRecipes", "[crafting]") {
    CraftingSystem crafting;

    CraftingRecipe r1{};
    r1.recipeId = 1;
    crafting.registerRecipe(r1);

    CraftingRecipe r2{};
    r2.recipeId = 2;
    crafting.registerRecipe(r2);

    const auto& all = crafting.getAllRecipes();
    CHECK(all.size() == 2);
}

TEST_CASE("CraftingSystem initializeDefaults creates recipes", "[crafting]") {
    CraftingSystem crafting;
    crafting.initializeDefaults();

    // Should have at least 5 default recipes
    CHECK(crafting.getAllRecipes().size() >= 5);

    // Check specific recipes exist
    CHECK(crafting.getRecipe(1) != nullptr); // Iron Sword
    CHECK(crafting.getRecipe(2) != nullptr); // Healing Potion
    CHECK(crafting.getRecipe(3) != nullptr); // Iron Chestplate
    CHECK(crafting.getRecipe(4) != nullptr); // Mana Potion
    CHECK(crafting.getRecipe(5) != nullptr); // Relic Blade
}

// ============================================================================
// TEST: Validation
// ============================================================================

TEST_CASE("CraftingSystem canCraft with sufficient materials", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    // Recipe 4 (Mana Potion): 1x Iron Ore, 3 gold, instant
    items.addToInventory(registry, player, 21, 1);
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    CHECK(crafting.canCraft(registry, player, 4));
}

TEST_CASE("CraftingSystem canCraft fails without materials", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    // No materials in inventory

    CHECK_FALSE(crafting.canCraft(registry, player, 1));
}

TEST_CASE("CraftingSystem canCraft fails without gold", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 2); // Has materials
    // But no gold (default 0)

    CHECK_FALSE(crafting.canCraft(registry, player, 1)); // Needs 10 gold
}

TEST_CASE("CraftingSystem canCraft fails below level", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry, 1); // Level 1
    items.addToInventory(registry, player, 21, 3); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    cc->professionLevel = 2;

    // Recipe 3 requires level 5
    CHECK_FALSE(crafting.canCraft(registry, player, 3));
    // Recipe 4 requires level 1, 1x Iron Ore, 3 gold
    CHECK(crafting.canCraft(registry, player, 4));
}

TEST_CASE("CraftingSystem canCraft fails when already crafting", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 1); // Iron Ore
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    // Start a craft (timed recipe 1 needs 2x Iron Ore + 1x Wolf Pelt + 10 gold)
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    items.addToInventory(registry, player, 21, 1); // More Iron Ore
    crafting.startCraft(registry, player, 1, 1000);

    // Now try another craft - should fail (already crafting)
    items.addToInventory(registry, player, 21, 1); // Give materials for recipe 4
    CHECK_FALSE(crafting.canCraft(registry, player, 4));
}

TEST_CASE("CraftingSystem canCraft fails below profession level", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry, 10); // High enough player level
    items.addToInventory(registry, player, 21, 3); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    // Recipe 3 requires profession level 2
    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    cc->professionLevel = 0;
    CHECK_FALSE(crafting.canCraft(registry, player, 3));

    cc->professionLevel = 2;
    CHECK(crafting.canCraft(registry, player, 3));
}

TEST_CASE("CraftingSystem canCraft fails with non-existent recipe", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    CHECK_FALSE(crafting.canCraft(registry, player, 9999));
}

TEST_CASE("CraftingSystem hasRequiredMaterials", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    // Recipe 4 needs 1x Iron Ore (item 21)
    CHECK_FALSE(crafting.hasRequiredMaterials(registry, player, 4));

    items.addToInventory(registry, player, 21, 1);
    CHECK(crafting.hasRequiredMaterials(registry, player, 4)); // Have 1, need 1

    // Recipe 1 needs 2x Iron Ore + 1x Wolf Pelt
    CHECK_FALSE(crafting.hasRequiredMaterials(registry, player, 1));
    items.addToInventory(registry, player, 21, 1); // Total 2x Iron Ore
    CHECK_FALSE(crafting.hasRequiredMaterials(registry, player, 1)); // Still need Wolf Pelt
    items.addToInventory(registry, player, 20, 1); // 1x Wolf Pelt
    CHECK(crafting.hasRequiredMaterials(registry, player, 1)); // Now complete
}

TEST_CASE("CraftingSystem hasRequiredGold", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    Inventory* inv = registry.try_get<Inventory>(player);

    // Recipe 4 costs 3 gold
    CHECK_FALSE(crafting.hasRequiredGold(registry, player, 4));

    inv->gold = 2.0f;
    CHECK_FALSE(crafting.hasRequiredGold(registry, player, 4));

    inv->gold = 3.0f;
    CHECK(crafting.hasRequiredGold(registry, player, 4));

    inv->gold = 100.0f;
    CHECK(crafting.hasRequiredGold(registry, player, 4));

    // Recipe 1 costs 10 gold
    CHECK(crafting.hasRequiredGold(registry, player, 1));

    // Recipe with 0 gold cost
    inv->gold = 0.0f;
    // Recipe 2 costs 5 gold, need to check
    CHECK_FALSE(crafting.hasRequiredGold(registry, player, 2));
}

// ============================================================================
// TEST: Instant Crafting
// ============================================================================

TEST_CASE("CraftingSystem instant craft produces item", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    // Recipe 4: Mana Potion (instant, 1x Iron Ore, 3 gold)
    items.addToInventory(registry, player, 21, 1);
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 10.0f;

    // Check initial mana potion count
    uint32_t initialManaPotions = items.countInInventory(registry, player, 2);

    bool success = crafting.startCraft(registry, player, 4, 1000);
    CHECK(success);

    // Materials consumed
    CHECK(items.countInInventory(registry, player, 21) == 0);
    CHECK(inv->gold == Approx(7.0f));

    // Output item produced
    CHECK(items.countInInventory(registry, player, 2) == initialManaPotions + 1);

    // Player should not be in crafting state (instant)
    const CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    CHECK_FALSE(cc->isCrafting());
    CHECK(cc->craftCount == 1);
}

TEST_CASE("CraftingSystem instant craft fires callback", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 1);
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 10.0f;

    bool callbackFired = false;
    uint32_t cbRecipeId = 0;
    uint32_t cbOutputItem = 0;
    uint32_t cbOutputQty = 0;

    crafting.setCraftCompleteCallback([&](EntityID, uint32_t recipeId,
                                           uint32_t outputItemId, uint32_t outputQuantity) {
        callbackFired = true;
        cbRecipeId = recipeId;
        cbOutputItem = outputItemId;
        cbOutputQty = outputQuantity;
    });

    crafting.startCraft(registry, player, 4, 1000);

    CHECK(callbackFired);
    CHECK(cbRecipeId == 4);
    CHECK(cbOutputItem == 2);  // Mana Potion
    CHECK(cbOutputQty == 1);
}

// ============================================================================
// TEST: Timed Crafting
// ============================================================================

TEST_CASE("CraftingSystem timed craft starts and completes", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    // Recipe 1: Iron Sword (3s craft time, 2x Iron Ore + 1x Wolf Pelt, 10 gold)
    items.addToInventory(registry, player, 21, 2); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    uint32_t initialSwords = items.countInInventory(registry, player, 10);

    bool started = crafting.startCraft(registry, player, 1, 1000);
    CHECK(started);

    // Materials should be consumed immediately
    CHECK(items.countInInventory(registry, player, 21) == 0);
    CHECK(inv->gold == Approx(90.0f));

    // Player should be in crafting state
    const CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    CHECK(cc->isCrafting());
    CHECK(cc->currentRecipeId == 1);

    // Not complete yet
    CHECK_FALSE(crafting.updateCraft(registry, player, 2000));
    CHECK(cc->isCrafting());

    // Not complete at 2.9s
    CHECK_FALSE(crafting.updateCraft(registry, player, 3999));
    CHECK(cc->isCrafting());

    // Complete at 3s+
    bool completed = crafting.updateCraft(registry, player, 4000);
    CHECK(completed);
    CHECK_FALSE(cc->isCrafting());
    CHECK(cc->craftCount == 1);

    // Output item produced
    CHECK(items.countInInventory(registry, player, 10) == initialSwords + 1);
}

TEST_CASE("CraftingSystem timed craft fires start callback", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 2); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    bool startFired = false;
    uint32_t startRecipeId = 0;
    uint32_t startCraftTime = 0;

    crafting.setCraftStartCallback([&](EntityID, uint32_t recipeId, uint32_t craftTimeMs) {
        startFired = true;
        startRecipeId = recipeId;
        startCraftTime = craftTimeMs;
    });

    crafting.startCraft(registry, player, 1, 1000);

    CHECK(startFired);
    CHECK(startRecipeId == 1);
    CHECK(startCraftTime == 3000);
}

TEST_CASE("CraftingSystem timed craft fires complete callback", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 2); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    bool completeFired = false;
    crafting.setCraftCompleteCallback([&](EntityID, uint32_t, uint32_t, uint32_t) {
        completeFired = true;
    });

    crafting.startCraft(registry, player, 1, 1000);
    crafting.updateCraft(registry, player, 4000);

    CHECK(completeFired);
}

// ============================================================================
// TEST: Craft Cancellation
// ============================================================================

TEST_CASE("CraftingSystem cancelCraft", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 2); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    crafting.startCraft(registry, player, 1, 1000);

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    CHECK(cc->isCrafting());

    bool cancelled = crafting.cancelCraft(registry, player);
    CHECK(cancelled);
    CHECK_FALSE(cc->isCrafting());
    CHECK(cc->currentRecipeId == 0);

    // Materials are NOT refunded
    CHECK(items.countInInventory(registry, player, 21) == 0);
    CHECK(inv->gold == Approx(90.0f));
}

TEST_CASE("CraftingSystem cancelCraft when not crafting", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    CHECK_FALSE(crafting.cancelCraft(registry, player));
}

// ============================================================================
// TEST: Profession XP and Leveling
// ============================================================================

TEST_CASE("CraftingSystem craft awards profession XP", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 1);
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 10.0f;

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    CHECK(cc->professionXP == 0);

    // Craft instant recipe 4 (1 ingredient: 10 + 5*1 = 15 XP)
    crafting.startCraft(registry, player, 4, 1000);

    CHECK(cc->professionXP == 15);
    CHECK(cc->professionLevel == 0); // Not enough to level up
}

TEST_CASE("CraftingSystem profession levels up", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);

    // Set XP near level-up threshold (100 XP for level 0->1)
    cc->professionXP = 90;

    items.addToInventory(registry, player, 21, 1);
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 10.0f;

    // Craft instant recipe (15 XP) — 90 + 15 = 105 >= 100
    crafting.startCraft(registry, player, 4, 1000);

    CHECK(cc->professionLevel == 1);
    CHECK(cc->professionXP == 5); // 105 - 100 = 5
}

// ============================================================================
// TEST: Starter Recipes
// ============================================================================

TEST_CASE("CraftingSystem giveStarterRecipes", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;

    auto player = registry.create();
    // Player has no CraftingComponent
    CHECK_FALSE(registry.try_get<CraftingComponent>(player));

    crafting.giveStarterRecipes(registry, player);

    // Should now have CraftingComponent
    const CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    REQUIRE(cc != nullptr);
    CHECK_FALSE(cc->isCrafting());
    CHECK(cc->professionLevel == 0);
}

// ============================================================================
// TEST: Multi-Ingredient Recipes
// ============================================================================

TEST_CASE("CraftingSystem multi-ingredient recipe consumes all", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry, 5);

    // Recipe 3: Iron Chestplate (3x Iron Ore + 1x Wolf Pelt, 25 gold)
    items.addToInventory(registry, player, 21, 3); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 50.0f;

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    cc->professionLevel = 2; // Required for recipe 3

    bool success = crafting.startCraft(registry, player, 3, 1000);
    CHECK(success);

    // All materials consumed
    CHECK(items.countInInventory(registry, player, 21) == 0);
    CHECK(items.countInInventory(registry, player, 20) == 0);
    CHECK(inv->gold == Approx(25.0f));
}

TEST_CASE("CraftingSystem multi-ingredient recipe fails if any missing", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry, 5);

    // Recipe 3 needs 3x Iron Ore + 1x Wolf Pelt
    items.addToInventory(registry, player, 21, 3); // Iron Ore (enough)
    // Missing Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 50.0f;

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    cc->professionLevel = 2;

    CHECK_FALSE(crafting.canCraft(registry, player, 3));
}

// ============================================================================
// TEST: Edge Cases
// ============================================================================

TEST_CASE("CraftingSystem startCraft with non-existent recipe", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    CHECK_FALSE(crafting.startCraft(registry, player, 9999, 1000));
}

TEST_CASE("CraftingSystem updateCraft when not crafting", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);

    CHECK_FALSE(crafting.updateCraft(registry, player, 1000));
}

TEST_CASE("CraftingSystem updateCraft with removed recipe", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    items.addToInventory(registry, player, 21, 2); // Iron Ore
    items.addToInventory(registry, player, 20, 1); // Wolf Pelt
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 100.0f;

    crafting.startCraft(registry, player, 1, 1000);

    // Force a non-existent recipe ID (simulates recipe removal)
    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    cc->currentRecipeId = 9999;

    // updateCraft should cancel since recipe doesn't exist
    bool result = crafting.updateCraft(registry, player, 5000);
    CHECK_FALSE(result);
    CHECK_FALSE(cc->isCrafting());
}

TEST_CASE("CraftingSystem without ItemSystem cannot craft", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    // No ItemSystem set
    crafting.initializeDefaults();

    auto player = createTestPlayer(registry);

    // hasRequiredMaterials returns false without ItemSystem
    CHECK_FALSE(crafting.hasRequiredMaterials(registry, player, 1));
    // canCraft returns false
    CHECK_FALSE(crafting.canCraft(registry, player, 1));
}

TEST_CASE("CraftingSystem multiple crafts increment count", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry);
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 1000.0f;

    // Craft recipe 4 (instant, 1x Iron Ore + 3 gold) 3 times
    for (int i = 0; i < 3; ++i) {
        items.addToInventory(registry, player, 21, 1); // Replenish Iron Ore
        bool success = crafting.startCraft(registry, player, 4, static_cast<uint32_t>(1000 + i * 100));
        REQUIRE(success);
    }

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    CHECK(cc->craftCount == 3);
}

TEST_CASE("CraftingSystem hidden recipe still craftable", "[crafting]") {
    Registry registry;
    CraftingSystem crafting;
    ItemSystem items;
    setupSystems(crafting, items);

    auto player = createTestPlayer(registry, 20);

    // Recipe 5 (Relic Blade) is discovered=false but still craftable
    items.addToInventory(registry, player, 22, 1); // Ancient Relic Shard
    items.addToInventory(registry, player, 21, 5); // Iron Ore
    Inventory* inv = registry.try_get<Inventory>(player);
    inv->gold = 1000.0f;

    CraftingComponent* cc = registry.try_get<CraftingComponent>(player);
    cc->professionLevel = 5;

    const CraftingRecipe* recipe = crafting.getRecipe(5);
    REQUIRE(recipe != nullptr);
    CHECK_FALSE(recipe->discovered);

    // canCraft doesn't check discovered flag (that's UI concern)
    CHECK(crafting.canCraft(registry, player, 5));
}
