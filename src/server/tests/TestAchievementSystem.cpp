#include <catch2/catch_test_macros.hpp>
#include "combat/AchievementSystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/CoreTypes.hpp"

using namespace DarkAges;

TEST_CASE("AchievementSystem basics", "[achievements]") {
    Registry registry;
    AchievementSystem system;
    
    // Initialize with test achievements
    std::vector<AchievementDef> achievements = {
        {1, "First Blood", "Kill your first enemy", 
         AchievementCategory::Combat, 1, 50, 0, nullptr, "Bloodied"},
        {2, "Monster Slayer", "Kill 10 enemies",
         AchievementCategory::Combat, 10, 100, 10, nullptr, "Slayer"},
        {3, "Craftsman", "Craft 5 items",
         AchievementCategory::Crafting, 5, 75, 0, nullptr, nullptr},
        {4, "Explorer", "Visit 3 zones",
         AchievementCategory::Exploration, 3, 50, 0, nullptr, nullptr},
    };
    system.initialize(achievements);
    
    REQUIRE(system.getAchievementCount() == 4);
    
    SECTION("attach to player") {
        EntityID player = registry.create();
        registry.emplace<PlayerComponent>(player);
        
        system.attachToPlayer(registry, player);
        
        auto* comp = registry.try_get<AchievementComponent>(player);
        REQUIRE(comp != nullptr);
        REQUIRE(comp->count == 4);
        REQUIRE(comp->totalPoints == 0);
    }
    
    SECTION("update progress") {
        EntityID player = registry.create();
        registry.emplace<PlayerComponent>(player);
        
        system.attachToPlayer(registry, player);
        
        auto* comp = registry.try_get<AchievementComponent>(player);
        REQUIRE(comp != nullptr);
        REQUIRE(comp->count == 4);  // All 4 achievements tracked
        
        system.updateProgress(registry, player, AchievementCategory::Combat, 1);
        
        // Check partial progress
        comp = registry.try_get<AchievementComponent>(player);
        REQUIRE(comp != nullptr);
        
        // First achievement should be done (criteria 1 met with 1 kill)
        bool found = false;
        for (uint32_t i = 0; i < comp->count; ++i) {
            if (comp->achievements[i].achievementId == 1) {
                REQUIRE(comp->achievements[i].completed == true);
                found = true;
            }
        }
        REQUIRE(found == true);
    }
    
    SECTION("check achievements") {
        EntityID player = registry.create();
        registry.emplace<PlayerComponent>(player);
        
        auto& comp = registry.emplace<AchievementComponent>(player);
        comp.count = 1;
        comp.achievements[0].player = player;
        comp.achievements[0].achievementId = 1;
        comp.achievements[0].current = 1;  // Criteria is 1
        
        bool anyUnlocked = system.checkAchievements(registry, player);
        REQUIRE(anyUnlocked == true);
        REQUIRE(comp.achievements[0].completed == true);
    }
    
    SECTION("has achievement") {
        EntityID player = registry.create();
        registry.emplace<PlayerComponent>(player);
        
        auto& comp = registry.emplace<AchievementComponent>(player);
        comp.count = 1;
        comp.achievements[0].player = player;
        comp.achievements[0].achievementId = 1;
        
        REQUIRE(system.hasAchievement(registry, player, 1) == false);
        
        comp.achievements[0].completed = true;
        
        REQUIRE(system.hasAchievement(registry, player, 1) == true);
    }
    
    SECTION("completion percentage") {
        EntityID player = registry.create();
        registry.emplace<PlayerComponent>(player);
        
        auto& comp = registry.emplace<AchievementComponent>(player);
        comp.count = 4;
        comp.achievements[0].completed = true;
        comp.achievements[1].completed = true;
        comp.achievements[2].completed = false;
        comp.achievements[3].completed = false;
        
        float pct = system.getCompletionPercentage(registry, player);
        REQUIRE(pct == 0.5f);  // 2/4 completed
    }
    
    SECTION("get player points") {
        EntityID player = registry.create();
        registry.emplace<PlayerComponent>(player);
        
        auto& comp = registry.emplace<AchievementComponent>(player);
        // Points: 10 + (criteria/10) = 10 + 0 = 10 per achievement
        comp.totalPoints = 20;
        
        REQUIRE(system.getPlayerPoints(registry, player) == 20);
    }
}

TEST_CASE("AchievementSystem callback", "[achievements]") {
    Registry registry;
    AchievementSystem system;
    
    std::vector<AchievementDef> achievements = {
        {1, "First Blood", "Kill your first enemy",
         AchievementCategory::Combat, 1, 50, 10, nullptr, "Bloodied"},
    };
    system.initialize(achievements);
    
    EntityID player = registry.create();
    registry.emplace<PlayerComponent>(player);
    system.attachToPlayer(registry, player);
    
    bool callbackFired = false;
    AchievementDef caughtDef;
    uint32_t caughtPoints = 0;
    
    system.setAchievementUnlockedCallback(
        [&callbackFired, &caughtDef, &caughtPoints](
            EntityID p, const AchievementDef& def, uint32_t points) {
            callbackFired = true;
            caughtDef = def;
            caughtPoints = points;
        });
    
    // Update progress - achievement should unlock
    system.updateProgress(registry, player, AchievementCategory::Combat, 1);
    
    // Verify callback was fired with reward info
    REQUIRE(callbackFired == true);
    REQUIRE(caughtDef.xpReward == 50);
    REQUIRE(caughtDef.goldReward == 10);
    REQUIRE(caughtPoints == 10);  // 10 base + 0 criteria = 10 points
}

TEST_CASE("AchievementSystem get definition", "[achievements]") {
    Registry registry;
    AchievementSystem system;
    
    std::vector<AchievementDef> achievements = {
        {1, "First Blood", "Kill your first enemy",
         AchievementCategory::Combat, 1, 50, 0, nullptr, "Bloodied"},
        {2, "Craftsman", "Craft items",
         AchievementCategory::Crafting, 5, 75, 0, nullptr, nullptr},
    };
    system.initialize(achievements);
    
    const AchievementDef* def = system.getAchievementDef(1);
    REQUIRE(def != nullptr);
    REQUIRE(std::strcmp(def->name, "First Blood") == 0);
    
    def = system.getAchievementDef(999);
    REQUIRE(def == nullptr);
}
TEST_CASE("AchievementSystem detach from player", "[achievements]") {
    Registry registry;
    AchievementSystem system;

    std::vector<AchievementDef> achievements = {
        {1, "First Blood", "Kill your first enemy",
         AchievementCategory::Combat, 1, 50, 0, nullptr, "Bloodied"},
    };
    system.initialize(achievements);

    EntityID player = registry.create();
    registry.emplace<PlayerComponent>(player);
    system.attachToPlayer(registry, player);

    auto* comp = registry.try_get<AchievementComponent>(player);
    REQUIRE(comp != nullptr);

    system.detachFromPlayer(registry, player);
    comp = registry.try_get<AchievementComponent>(player);
    REQUIRE(comp == nullptr);
}

TEST_CASE("AchievementSystem multi-category progress", "[achievements]") {
    Registry registry;
    AchievementSystem system;

    std::vector<AchievementDef> achievements = {
        {1, "First Blood", "Kill",
         AchievementCategory::Combat, 1, 50, 0, nullptr, nullptr},
        {2, "Craftsman", "Craft 5",
         AchievementCategory::Crafting, 5, 75, 0, nullptr, nullptr},
        {3, "Explorer", "Visit 3",
         AchievementCategory::Exploration, 3, 50, 0, nullptr, nullptr},
    };
    system.initialize(achievements);

    EntityID player = registry.create();
    registry.emplace<PlayerComponent>(player);
    system.attachToPlayer(registry, player);

    system.updateProgress(registry, player, AchievementCategory::Combat, 1);
    system.updateProgress(registry, player, AchievementCategory::Crafting, 5);
    system.updateProgress(registry, player, AchievementCategory::Exploration, 2);

    auto* comp = registry.try_get<AchievementComponent>(player);
    REQUIRE(comp != nullptr);

    bool combatDone = false, craftingDone = false, explorerPartial = false;
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].achievementId == 1 && comp->achievements[i].completed) combatDone = true;
        if (comp->achievements[i].achievementId == 2 && comp->achievements[i].completed) craftingDone = true;
        if (comp->achievements[i].achievementId == 3) explorerPartial = (comp->achievements[i].current == 2);
    }
    REQUIRE(combatDone == true);
    REQUIRE(craftingDone == true);
    REQUIRE(explorerPartial == true);
}

// ============================================================================
// Additional Achievement Tests - extending coverage
// ============================================================================

TEST_CASE("AchievementSystem progress accumulates beyond criteria", "[achievements]") {
    Registry registry;
    AchievementSystem system;
    
    std::vector<AchievementDef> achievements = {
        {1, "Grinder", "Kill 10 enemies",
         AchievementCategory::Combat, 10, 100, 0, nullptr, nullptr},
    };
    system.initialize(achievements);
    
    EntityID player = registry.create();
    registry.emplace<PlayerComponent>(player);
    system.attachToPlayer(registry, player);
    
    // Update with way more than criteria
    system.updateProgress(registry, player, AchievementCategory::Combat, 25);
    
    auto* comp = registry.try_get<AchievementComponent>(player);
    REQUIRE(comp != nullptr);
    
    // Achievement should be completed
    bool found = false;
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].achievementId == 1 && comp->achievements[i].completed) {
            found = true;
            // Progress should be capped at criteria
            REQUIRE(comp->achievements[i].current == 10);
        }
    }
    REQUIRE(found == true);
}

TEST_CASE("AchievementSystem multiple players independent", "[achievements]") {
    Registry registry;
    AchievementSystem system;
    
    std::vector<AchievementDef> achievements = {
        {1, "Killer", "Kill 5 enemies",
         AchievementCategory::Combat, 5, 50, 0, nullptr, nullptr},
    };
    system.initialize(achievements);
    
    EntityID p1 = registry.create();
    EntityID p2 = registry.create();
    registry.emplace<PlayerComponent>(p1);
    registry.emplace<PlayerComponent>(p2);
    
    system.attachToPlayer(registry, p1);
    system.attachToPlayer(registry, p2);
    
    // p1 completes achievement
    system.updateProgress(registry, p1, AchievementCategory::Combat, 5);
    
    // p2 has no progress
    system.updateProgress(registry, p2, AchievementCategory::Combat, 1);
    
    REQUIRE(system.hasAchievement(registry, p1, 1) == true);
    REQUIRE(system.hasAchievement(registry, p2, 1) == false);
}

TEST_CASE("AchievementSystem reset player progress", "[achievements]") {
    Registry registry;
    AchievementSystem system;
    
    std::vector<AchievementDef> achievements = {
        {1, "Killer", "Kill 1 enemy",
         AchievementCategory::Combat, 1, 50, 0, nullptr, nullptr},
    };
    system.initialize(achievements);
    
    EntityID player = registry.create();
    registry.emplace<PlayerComponent>(player);
    system.attachToPlayer(registry, player);
    
    // Complete the achievement
    system.updateProgress(registry, player, AchievementCategory::Combat, 1);
    REQUIRE(system.hasAchievement(registry, player, 1) == true);
    
    // Reset progress would require detach + reattach (test detach behavior)
    system.detachFromPlayer(registry, player);
    system.attachToPlayer(registry, player);
    
    // After reattach, achievement should be incomplete again
    auto* comp = registry.try_get<AchievementComponent>(player);
    REQUIRE(comp != nullptr);
    bool stillComplete = false;
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].completed) stillComplete = true;
    }
    REQUIRE(stillComplete == false);
}
