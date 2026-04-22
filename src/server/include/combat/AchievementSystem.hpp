#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// Achievement System — tracks player accomplishments and awards rewards.
// Provides categories: Combat, Crafting, Exploration, Collection, Social, Economy

namespace DarkAges {

// Achievement categories
enum class AchievementCategory : uint8_t {
    Combat,       // Kills, boss kills, PvP wins
    Crafting,     // Items crafted, profession levels
    Exploration,  // Zones visited, NPCs spoken to
    Collection,   // Items collected, rare finds
    Social,      // Guilds joined, parties, friends
    Economy,     // Gold earned, items sold
    Level,       // Level milestones
    Quest        // Quest completion
};

// Achievement definition
struct AchievementDef {
    uint32_t id;
    const char* name;
    const char* description;
    AchievementCategory category;
    int32_t criteria;        // Target count/value
    int32_t xpReward;       // XP awarded on completion
    int32_t goldReward;     // Gold awarded on completion
    const char* itemReward; // Item template (optional)
    const char* titleReward; // Title unlocked (optional)
};

// Player's achievement progress
struct AchievementProgress {
    EntityID player = entt::null;
    uint32_t achievementId = 0;
    int32_t current = 0;      // Current progress toward criteria
    bool completed = false;      // Achievement unlocked
    uint32_t completedAt = 0; // Timestamp when unlocked
};

// Component attached to player entity
struct AchievementComponent {
    static constexpr uint32_t MAX_ACHIEVEMENTS = 64;
    
    AchievementProgress achievements[MAX_ACHIEVEMENTS];
    uint32_t count = 0;
    uint32_t totalPoints = 0;
};

class AchievementSystem {
public:
    AchievementSystem() = default;

    // --- Initialization ---

    // Initialize with achievement definitions
    void initialize(const std::vector<AchievementDef>& achievements);

    // Get total achievement count
    [[nodiscard]] uint32_t getAchievementCount() const { 
        return static_cast<uint32_t>(definitions_.size()); 
    }

    // --- Player State ---

    // Attach achievement tracking to a player entity
    void attachToPlayer(Registry& registry, EntityID player);

    // Remove achievement tracking from player
    void detachFromPlayer(Registry& registry, EntityID player);

    // --- Progress Updates ---

    // Update progress for a specific achievement type
    // Called when relevant game events occur
    void updateProgress(Registry& registry, EntityID player,
                       AchievementCategory category, int32_t amount = 1);

    // Check and award completed achievements
    // Returns true if any achievements were unlocked
    bool checkAchievements(Registry& registry, EntityID player);

    // --- Queries ---

    // Get achievement progress for a player
    [[nodiscard]] std::vector<AchievementProgress> getPlayerProgress(
        const Registry& registry, EntityID player) const;

    // Check if player has a specific achievement
    [[nodiscard]] bool hasAchievement(
        const Registry& registry, EntityID player, uint32_t achievementId) const;

    // Get completion percentage for a player
    [[nodiscard]] float getCompletionPercentage(
        const Registry& registry, EntityID player) const;

    // Get points earned by player
    [[nodiscard]] uint32_t getPlayerPoints(
        const Registry& registry, EntityID player) const;

    // --- Rewards ---

    // Callback for when achievement is unlocked
    using AchievementUnlockedCallback = std::function<void(
        EntityID, const AchievementDef&, uint32_t)>;
    void setAchievementUnlockedCallback(AchievementUnlockedCallback cb) {
        achievementUnlockedCallback_ = std::move(cb);
    }

    // Get achievement definition by ID
    [[nodiscard]] const AchievementDef* getAchievementDef(uint32_t id) const;

private:
    std::vector<AchievementDef> definitions_;
    std::unordered_map<uint32_t, uint32_t> idToIndex_;
    AchievementUnlockedCallback achievementUnlockedCallback_;

    AchievementProgress* findProgress(Registry& registry, EntityID player, 
                              uint32_t achievementId);
    const AchievementProgress* findProgress(
        const Registry& registry, EntityID player, 
        uint32_t achievementId) const;

    void unlockAchievement(Registry& registry, EntityID player,
                      AchievementProgress& progress,
                      const AchievementDef& def);
};

} // namespace DarkAges