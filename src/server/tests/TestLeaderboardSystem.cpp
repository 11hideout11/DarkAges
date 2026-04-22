#include <catch2/catch_test_macros.hpp>
#include "combat/LeaderboardSystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/CoreTypes.hpp"

using namespace DarkAges;

TEST_CASE("LeaderboardSystem basics", "[leaderboard]") {
    Registry registry;
    LeaderboardSystem system;
    
    std::vector<LeaderboardConfig> configs = {
        {LeaderboardCategory::Level, "Level", 10, false},
        {LeaderboardCategory::Gold, "Gold", 10, false},
        {LeaderboardCategory::Kills, "Kills", 10, false},
    };
    system.initialize(configs);
    
    SECTION("basic operations") {
        EntityID player1 = registry.create();
        EntityID player2 = registry.create();
        
        system.setPlayerName(player1, "Alice");
        system.setPlayerName(player2, "Bob");
        
        system.updateScore(registry, player1, LeaderboardCategory::Level, 10);
        system.updateScore(registry, player2, LeaderboardCategory::Level, 20);
        
        // Get top
        auto top = system.getTop(LeaderboardCategory::Level, 10);
        REQUIRE(top.size() == 2);
        REQUIRE(top[0].entity == player2);  // Higher level first
        REQUIRE(top[0].value == 20);
    }
    
    SECTION("ranking") {
        EntityID player1 = registry.create();
        EntityID player2 = registry.create();
        EntityID player3 = registry.create();
        
        system.setPlayerName(player1, "Alice");
        system.setPlayerName(player2, "Bob");
        system.setPlayerName(player3, "Charlie");
        
        system.updateScore(registry, player1, LeaderboardCategory::Level, 10);
        system.updateScore(registry, player2, LeaderboardCategory::Level, 30);
        system.updateScore(registry, player3, LeaderboardCategory::Level, 20);
        
        REQUIRE(system.getRank(LeaderboardCategory::Level, player1) == 3);
        REQUIRE(system.getRank(LeaderboardCategory::Level, player2) == 1);
        REQUIRE(system.getRank(LeaderboardCategory::Level, player3) == 2);
    }
    
    SECTION("gold leaderboard") {
        EntityID player1 = registry.create();
        EntityID player2 = registry.create();
        
        system.setPlayerName(player1, "RichPlayer");
        system.updateScore(registry, player1, LeaderboardCategory::Gold, 1000);
        
        auto top = system.getTop(LeaderboardCategory::Gold, 10);
        REQUIRE(top.size() == 1);
        REQUIRE(top[0].value == 1000);
        REQUIRE(top[0].playerName == std::string("RichPlayer"));
    }
    
    SECTION("kills leaderboard") {
        EntityID player1 = registry.create();
        EntityID player2 = registry.create();
        
        system.setPlayerName(player1, "Killer1");
        system.setPlayerName(player2, "Killer2");
        
        system.updateScore(registry, player1, LeaderboardCategory::Kills, 50);
        system.updateScore(registry, player2, LeaderboardCategory::Kills, 100);
        
        auto top = system.getTop(LeaderboardCategory::Kills, 5);
        REQUIRE(top.size() == 2);
        REQUIRE(top[0].entity == player2);  // More kills
    }
    
    SECTION("get entry") {
        EntityID player1 = registry.create();
        
        system.setPlayerName(player1, "TestPlayer");
        system.updateScore(registry, player1, LeaderboardCategory::Level, 25);
        
        auto entry = system.getEntry(LeaderboardCategory::Level, player1);
        REQUIRE(entry.entity == player1);
        REQUIRE(entry.value == 25);
        REQUIRE(entry.playerName == std::string("TestPlayer"));
    }
    
    SECTION("is in top N") {
        EntityID player1 = registry.create();
        EntityID player2 = registry.create();
        
        system.setPlayerName(player1, "TopPlayer");
        system.setPlayerName(player2, "LowPlayer");
        
        system.updateScore(registry, player1, LeaderboardCategory::Level, 100);
        system.updateScore(registry, player2, LeaderboardCategory::Level, 1);
        
        REQUIRE(system.isInTopN(LeaderboardCategory::Level, player1, 5) == true);
        // With only 2 entries and maxEntries=10, both are in top 5
        REQUIRE(system.isInTopN(LeaderboardCategory::Level, player2, 5) == true);
    }
    
    SECTION("get category by name") {
        auto cat = system.getCategoryByName("Level");
        REQUIRE(cat == LeaderboardCategory::Level);
        
        cat = system.getCategoryByName("Gold");
        REQUIRE(cat == LeaderboardCategory::Gold);
        
        cat = system.getCategoryByName("Unknown");
        REQUIRE(cat == LeaderboardCategory::Level);  // Default
    }
    
    SECTION("remove player") {
        EntityID player1 = registry.create();
        
        system.setPlayerName(player1, "Removed");
        system.updateScore(registry, player1, LeaderboardCategory::Level, 50);
        
        REQUIRE(system.getRank(LeaderboardCategory::Level, player1) == 1);
        
        system.removePlayer(player1);
        
        REQUIRE(system.getRank(LeaderboardCategory::Level, player1) == 0);
    }
    
    SECTION("recalculate") {
        EntityID player1 = registry.create();
        EntityID player2 = registry.create();
        
        system.setPlayerName(player1, "P1");
        system.setPlayerName(player2, "P2");
        
        // Add multiple scores
        system.updateScore(registry, player1, LeaderboardCategory::Kills, 10);
        system.updateScore(registry, player1, LeaderboardCategory::Kills, 5);  // Later
        system.updateScore(registry, player2, LeaderboardCategory::Kills, 15);
        
        // Latest score should be current
        auto entry = system.getEntry(LeaderboardCategory::Kills, player1);
        REQUIRE(entry.value == 5);  // Latest
        
        system.recalculateAll(registry);
        
        entry = system.getEntry(LeaderboardCategory::Kills, player1);
        REQUIRE(entry.value == 5);
    }
}

TEST_CASE("LeaderboardSystem tie-breaking", "[leaderboard]") {
    Registry registry;
    LeaderboardSystem system;
    
    std::vector<LeaderboardConfig> configs = {
        {LeaderboardCategory::Level, "Level", 10, false},
    };
    system.initialize(configs);
    
    // Same level, different entity IDs
    EntityID player1 = registry.create();
    EntityID player2 = registry.create();
    EntityID player3 = registry.create();
    
    system.setPlayerName(player1, "A");
    system.setPlayerName(player2, "B");
    system.setPlayerName(player3, "C");
    
    // All have same level
    system.updateScore(registry, player1, LeaderboardCategory::Level, 10);
    system.updateScore(registry, player2, LeaderboardCategory::Level, 10);
    system.updateScore(registry, player3, LeaderboardCategory::Level, 10);
    
    auto top = system.getTop(LeaderboardCategory::Level, 3);
    REQUIRE(top.size() == 3);
    
    // Lower entity ID should rank higher in tie
    REQUIRE(top[0].entity == player1);
    REQUIRE(top[1].entity == player2);
    REQUIRE(top[2].entity == player3);
}