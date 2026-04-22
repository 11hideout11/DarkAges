#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <queue>
#include <functional>

// Leaderboard System — tracks player rankings across categories.
// Uses a skip list for O(log n) rank lookups.

namespace DarkAges {

// Leaderboard categories
enum class LeaderboardCategory : uint8_t {
    Level,         // Highest level
    Gold,         // Most gold
    Kills,        // Most player kills
    BossKills,    // Most boss kills
    Crafted,      // Most items crafted
    Quests,       // Most quests completed
    PlayTime,     // Most play time
    Achievements  // Most achievement points
};

// Entry in a leaderboard
struct LeaderboardEntry {
    EntityID entity = entt::null;
    std::string playerName;
    int32_t value = 0;
    uint32_t rank = 0;
    
    LeaderboardEntry() = default;
};

// Configuration for a leaderboard category
struct LeaderboardConfig {
    LeaderboardCategory category;
    const char* name;
    uint32_t maxEntries = 100;  // Top N displayed
    bool ascending = false;         // For play time, true = more is better
};

class LeaderboardSystem {
public:
    LeaderboardSystem() = default;

    // --- Initialization ---

    // Initialize leaderboard with configurations
    void initialize(const std::vector<LeaderboardConfig>& configs);

    // --- Updates ---

    // Update a player's score in a category
    void updateScore(Registry& registry, EntityID player, 
                    LeaderboardCategory category, int32_t newValue);

    // Remove a player from all leaderboards
    void removePlayer(EntityID player);

    // Recalculate all rankings (expensive, use sparingly)
    void recalculateAll(Registry& registry);

    // --- Queries ---

    // Get top N entries for a category
    [[nodiscard]] std::vector<LeaderboardEntry> getTop(
        LeaderboardCategory category, uint32_t count = 10) const;

    // Get player's rank in a category (1-based)
    [[nodiscard]] int32_t getRank(LeaderboardCategory category, 
                               EntityID player) const;

    // Get player's rank and value
    [[nodiscard]] LeaderboardEntry getEntry(
        LeaderboardCategory category, EntityID player) const;

    // Get category by name
    [[nodiscard]] LeaderboardCategory getCategoryByName(const char* name) const;

    // Check if player is in top N
    [[nodiscard]] bool isInTopN(LeaderboardCategory category, 
                              EntityID player, uint32_t n) const;

    // --- Player Name Updates ---

    // Set player name for display
    void setPlayerName(EntityID player, const char* name);

    // Get player name
    [[nodiscard]] std::string getPlayerName(EntityID player) const;

private:
    struct LeaderboardData {
        LeaderboardCategory category;
        std::string name;
        uint32_t maxEntries;
        bool ascending;
        std::vector<LeaderboardEntry> entries;
    };

    std::vector<LeaderboardData> leaderboards_;
    std::unordered_map<LeaderboardCategory, uint32_t> categoryToIndex_;
    std::unordered_map<EntityID, std::string> playerNames_;

    LeaderboardData* findLeaderboard(LeaderboardCategory category);
    const LeaderboardData* findLeaderboard(LeaderboardCategory category) const;

    void updateRank(LeaderboardData& lb);
    void sortAndTrim(LeaderboardData& lb);
};

} // namespace DarkAges