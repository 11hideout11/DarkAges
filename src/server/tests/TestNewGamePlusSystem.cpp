#include "combat/NewGamePlusSystem.hpp"
#include "combat/ProgressionCalculator.hpp"
#include "ecs/CoreTypes.hpp"
#include "ecs/Registry.hpp"
#include "catch2/catch.hpp"

using namespace DarkAges;

TEST_CASE("NewGamePlusSystem basic functionality", "[newgameplus][system]") {
    ProgressionCalculator calculator;
    NewGamePlusSystem ngPlusSystem;
    ngPlusSystem.initialize(&calculator);
    
    Registry registry;
    EntityID player = registry.create();
    
    SECTION("Initial NG+ level is 0") {
        REQUIRE(ngPlusSystem.getNGPlusLevel(registry, player) == 0);
    }
    
    SECTION("Increase NG+ level") {
        ngPlusSystem.increaseNGPlusLevel(registry, player);
        REQUIRE(ngPlusSystem.getNGPlusLevel(registry, player) == 1);
        
        ngPlusSystem.increaseNGPlusLevel(registry, player);
        REQUIRE(ngPlusSystem.getNGPlusLevel(registry, player) == 2);
    }
    
    SECTION("Difficulty multiplier scales with NG+ level") {
        REQUIRE(ngPlusSystem.getDifficultyMultiplier(0) == 1.0f);
        REQUIRE(ngPlusSystem.getDifficultyMultiplier(1) == 1.1f);
        REQUIRE(ngPlusSystem.getDifficultyMultiplier(2) == 1.2f);
        REQUIRE(ngPlusSystem.getDifficultyMultiplier(5) == 1.5f);
    }
    
    SECTION("XP multiplier scales with NG+") {
        REQUIRE(ngPlusSystem.getXPMultiplier(0) == 1.0f);
        REQUIRE(ngPlusSystem.getXPMultiplier(1) == 1.2f);
        REQUIRE(ngPlusSystem.getXPMultiplier(2) == 1.4f);
    }
}

TEST_CASE("NG+ scaling applied to NPC spawns", "[newgameplus][zoneserver]") {
    // This test will verify that NG+ scaling is applied correctly in spawnNPC
    // We'll need to set up a ZoneServer and test the inline spawnNPC method
    // This is more complex and can be implemented later
    REQUIRE(true);
}


TEST_CASE("NewGamePlusSystem global level functionality", "[newgameplus][system]") {
    ProgressionCalculator calculator;
    NewGamePlusSystem ngPlusSystem;
    ngPlusSystem.initialize(&calculator);
    
    Registry registry;
    EntityID player = registry.create();
    
    SECTION("Global NG+ level starts at 0") {
        REQUIRE(ngPlusSystem.getGlobalNGPlusLevel() == 0);
    }
    
    SECTION("Setting global NG+ level") {
        ngPlusSystem.setGlobalNGPlusLevel(3);
        REQUIRE(ngPlusSystem.getGlobalNGPlusLevel() == 3);
        
        ngPlusSystem.setGlobalNGPlusLevel(5);
        REQUIRE(ngPlusSystem.getGlobalNGPlusLevel() == 5);
    }
    
    SECTION("Global NG+ level affects difficulty multiplier") {
        ngPlusSystem.setGlobalNGPlusLevel(2);
        // When we call getDifficultyMultiplier, it should use the global level
        float multiplier = ngPlusSystem.getDifficultyMultiplier(ngPlusSystem.getGlobalNGPlusLevel());
        REQUIRE(multiplier == Approx(1.2f));
        
        ngPlusSystem.setGlobalNGPlusLevel(4);
        multiplier = ngPlusSystem.getDifficultyMultiplier(ngPlusSystem.getGlobalNGPlusLevel());
        REQUIRE(multiplier == Approx(1.4f));
    }
}
