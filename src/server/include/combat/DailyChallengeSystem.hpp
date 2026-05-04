#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <entt/entt.hpp>
#include <netcode/NetworkManager.hpp>

// ============================================================================
// Daily Challenge System — PRD-040 Integration
// Provides daily goals for player engagement
// ============================================================================

namespace DarkAges {

// Challenge types
enum class ChallengeType : uint8_t {
    KillEnemies = 0,      // Kill N enemies
    CollectItems = 1,       // Collect N items  
    ExploreZones = 2,       // Visit N zones
    DealDamage = 3,         // Deal N damage
    CompleteQuests = 4,    // Complete N quests
    SurviveTime = 5         // Survive N minutes
};

// Daily challenge definition
struct DailyChallenge {
    uint32_t id{0};
    char name[48]{0};
    char description[96]{0};
    ChallengeType type{ChallengeType::KillEnemies};
    uint32_t target{10};           // Target count
    uint32_t xpReward{100};        // XP bonus
    uint32_t goldReward{50};        // Gold bonus
    uint32_t itemReward{0};         // Item ID reward
};

// Player's challenge progress
struct ChallengeProgress {
    uint32_t accountId{0};
    uint32_t challengeId{0};
    uint32_t progress{0};
    bool completed{false};
    uint32_t completedAt{0};     // Timestamp
};

class DailyChallengeSystem {
public:
    DailyChallengeSystem(NetworkManager* network);
    
    // PRD-040: Generate daily challenges (3 per day)
    void generateDailyChallenges();
    
    // PRD-040: Get today's challenges for player
    std::vector<DailyChallenge> getChallengesForPlayer(uint32_t accountId);
    
    // PRD-040: Update progress
    void updateProgress(entt::registry& registry, uint32_t accountId, 
                     ChallengeType type, uint32_t amount);
    
    // PRD-040: Check and claim rewards
    void claimRewards(entt::registry& registry, uint32_t accountId);
    
    // PRD-040: Get next reset time (midnight UTC)
    uint32_t getNextResetTime();
    
    // PRD-040: Check if challenges need regeneration
    bool needsRegeneration();
    
    // Send daily challenges to a specific player (client)
    void sendDailyChallengesToClient(ConnectionID connectionId);
    
private:
    void generateChallenge(DailyChallenge& chal, uint32_t seed);
    uint32_t getDayOfYear();
    
    std::vector<DailyChallenge> dailyChallenges_;
    std::vector<ChallengeProgress> playerProgress_;
    uint32_t lastGenerateDay_{0};
    NetworkManager* network_{nullptr};
};

} // namespace DarkAges
