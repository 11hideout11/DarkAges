#include "combat/LeaderboardSystem.hpp"
#include "ecs/Components.hpp"
#include <algorithm>
#include <cstring>

namespace DarkAges {

void LeaderboardSystem::initialize(const std::vector<LeaderboardConfig>& configs) {
    leaderboards_.clear();
    categoryToIndex_.clear();
    
    for (const auto& config : configs) {
        LeaderboardData lb;
        lb.category = config.category;
        lb.name = config.name;
        lb.maxEntries = config.maxEntries;
        lb.ascending = config.ascending;
        
        uint32_t index = static_cast<uint32_t>(leaderboards_.size());
        leaderboards_.push_back(lb);
        categoryToIndex_[config.category] = index;
    }
}

void LeaderboardSystem::updateScore(Registry& registry, EntityID player,
                             LeaderboardCategory category, int32_t newValue) {
    auto* lb = findLeaderboard(category);
    if (!lb) return;
    
    // Find existing entry or add new
    auto it = std::find_if(lb->entries.begin(), lb->entries.end(),
        [player](const LeaderboardEntry& e) { return e.entity == player; });
    
    if (it != lb->entries.end()) {
        // Update existing
        it->value = newValue;
    } else {
        // Add new entry
        LeaderboardEntry entry;
        entry.entity = player;
        entry.playerName = getPlayerName(player);
        entry.value = newValue;
        lb->entries.push_back(entry);
    }
    
    // Re-sort and trim
    sortAndTrim(*lb);
    updateRank(*lb);
}

void LeaderboardSystem::removePlayer(EntityID player) {
    playerNames_.erase(player);
    
    for (auto& lb : leaderboards_) {
        auto it = std::find_if(lb.entries.begin(), lb.entries.end(),
            [player](const LeaderboardEntry& e) { return e.entity == player; });
        if (it != lb.entries.end()) {
            lb.entries.erase(it);
        }
        // Re-rank
        for (auto& e : lb.entries) {
            e.rank = 0;
        }
        updateRank(lb);
    }
}

void LeaderboardSystem::recalculateAll(Registry& registry) {
    for (auto& lb : leaderboards_) {
        sortAndTrim(lb);
        updateRank(lb);
    }
}

std::vector<LeaderboardEntry> LeaderboardSystem::getTop(
    LeaderboardCategory category, uint32_t count) const {
    const auto* lb = findLeaderboard(category);
    if (!lb) return {};
    
    std::vector<LeaderboardEntry> result;
    uint32_t n = std::min(count, lb->maxEntries);
    
    for (uint32_t i = 0; i < lb->entries.size() && i < n; ++i) {
        if (lb->entries[i].rank > 0) {
            result.push_back(lb->entries[i]);
        }
    }
    return result;
}

int32_t LeaderboardSystem::getRank(LeaderboardCategory category, 
                            EntityID player) const {
    const auto* lb = findLeaderboard(category);
    if (!lb) return 0;
    
    for (const auto& entry : lb->entries) {
        if (entry.entity == player) {
            return static_cast<int32_t>(entry.rank);
        }
    }
    return 0;
}

LeaderboardEntry LeaderboardSystem::getEntry(
    LeaderboardCategory category, EntityID player) const {
    const auto* lb = findLeaderboard(category);
    if (!lb) return {};
    
    for (const auto& entry : lb->entries) {
        if (entry.entity == player) {
            return entry;
        }
    }
    return {};
}

LeaderboardCategory LeaderboardSystem::getCategoryByName(const char* name) const {
    for (const auto& lb : leaderboards_) {
        if (!lb.name.empty() && lb.name == name) {
            return lb.category;
        }
    }
    return LeaderboardCategory::Level;  // Default
}

bool LeaderboardSystem::isInTopN(LeaderboardCategory category, 
                                EntityID player, uint32_t n) const {
    const auto* lb = findLeaderboard(category);
    if (!lb) return false;
    
    for (const auto& entry : lb->entries) {
        if (entry.entity == player && entry.rank > 0 && entry.rank <= n) {
            return true;
        }
    }
    return false;
}

void LeaderboardSystem::setPlayerName(EntityID player, const char* name) {
    if (name) {
        playerNames_[player] = name;
    } else {
        playerNames_.erase(player);
    }
    
    // Update in all leaderboards
    for (auto& lb : leaderboards_) {
        for (auto& entry : lb.entries) {
            if (entry.entity == player) {
                entry.playerName = getPlayerName(player);
            }
        }
    }
}

std::string LeaderboardSystem::getPlayerName(EntityID player) const {
    auto it = playerNames_.find(player);
    if (it != playerNames_.end()) {
        return it->second;
    }
    return "";
}

LeaderboardSystem::LeaderboardData* LeaderboardSystem::findLeaderboard(
    LeaderboardCategory category) {
    auto it = categoryToIndex_.find(category);
    if (it == categoryToIndex_.end()) return nullptr;
    return &leaderboards_[it->second];
}

const LeaderboardSystem::LeaderboardData* LeaderboardSystem::findLeaderboard(
    LeaderboardCategory category) const {
    auto it = categoryToIndex_.find(category);
    if (it == categoryToIndex_.end()) return nullptr;
    return &leaderboards_[it->second];
}

void LeaderboardSystem::updateRank(LeaderboardData& lb) {
    for (auto& entry : lb.entries) {
        entry.rank = 0;
    }
    
    for (size_t i = 0; i < lb.entries.size(); ++i) {
        int32_t rank = 1;
        for (size_t j = 0; j < lb.entries.size(); ++j) {
            if (i == j) continue;
            
            // Higher value = better rank
            if (lb.entries[j].value > lb.entries[i].value) {
                ++rank;
            } else if (lb.entries[j].value == lb.entries[i].value &&
                       lb.entries[j].entity < lb.entries[i].entity) {
                // Tie-breaker: lower entity ID wins
                ++rank;
            }
        }
        lb.entries[i].rank = static_cast<uint32_t>(rank);
    }
}

void LeaderboardSystem::sortAndTrim(LeaderboardData& lb) {
    // Sort by value (descending by default)
    if (lb.ascending) {
        // More is better
        std::sort(lb.entries.begin(), lb.entries.end(),
            [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                if (a.value != b.value) return a.value < b.value;
                return a.entity < b.entity;
            });
    } else {
        // Higher is better
        std::sort(lb.entries.begin(), lb.entries.end(),
            [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
                if (a.value != b.value) return a.value > b.value;
                return a.entity < b.entity;
            });
    }
    
    // Trim to max entries
    if (lb.entries.size() > lb.maxEntries) {
        lb.entries.erase(lb.entries.begin() + lb.maxEntries, lb.entries.end());
    }
}

} // namespace DarkAges