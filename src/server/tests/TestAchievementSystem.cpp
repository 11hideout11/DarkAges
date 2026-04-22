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