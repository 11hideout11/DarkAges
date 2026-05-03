#include "combat/DailyChallengeSystem.hpp"
#include <ctime>
#include <cstring>

namespace DarkAges {

void DailyChallengeSystem::generateDailyChallenges() {
    uint32_t day = getDayOfYear();
    if (day != lastGenerateDay_) {
        generateDailyChallenges();  // Creates 3 challenges
        lastGenerateDay_ = day;
    }
}

void DailyChallengeSystem::generateDailyChallenges() {
    dailyChallenges_.clear();
    
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    uint32_t seed = static_cast<uint32_t>(tm->tm_yday);
    
    // Generate 3 challenges for today
    DailyChallenge chal;
    for (int i = 0; i < 3; i++) {
        generateChallenge(chal, seed + i);
        chal.id = i + 1;
        dailyChallenges_.push_back(chal);
    }
}

void DailyChallengeSystem::generateChallenge(DailyChallenge& chal, uint32_t seed) {
    // Use seed to vary challenge types
    uint32_t typeIdx = (seed + chal.id) % 6;
    
    chal.type = static_cast<ChallengeType>(typeIdx);
    
    switch (chal.type) {
        case ChallengeType::KillEnemies:
            strcpy(chal.name, "Monster Hunter");
            strcpy(chal.description, "Defeat 20 enemies");
            chal.target = 20;
            chal.xpReward = 200;
            chal.goldReward = 100;
            break;
        case ChallengeType::CollectItems:
            strcpy(chal.name, "Collector");
            strcpy(chal.description, "Collect 10 items");
            chal.target = 10;
            chal.xpReward = 150;
            chal.goldReward = 75;
            break;
        case ChallengeType::ExploreZones:
            strcpy(chal.name, "Explorer");
            strcpy(chal.description, "Visit 3 different zones");
            chal.target = 3;
            chal.xpReward = 100;
            chal.goldReward = 50;
            break;
        case ChallengeType::DealDamage:
            strcpy(chal.name, "Damage Dealer");
            strcpy(chal.description, "Deal 5000 damage");
            chal.target = 5000;
            chal.xpReward = 300;
            chal.goldReward = 150;
            break;
        case ChallengeType::CompleteQuests:
            strcpy(chal.name, "Quester");
            strcpy(chal.description, "Complete 3 quests");
            chal.target = 3;
            chal.xpReward = 250;
            chal.goldReward = 125;
            break;
        case ChallengeType::SurviveTime:
            strcpy(chal.name, "Survivor");
            strcpy(chal.description, "Survive for 15 minutes");
            chal.target = 15;
            chal.xpReward = 200;
            chal.goldReward = 100;
            break;
    }
}

std::vector<DailyChallenge> DailyChallengeSystem::getChallengesForPlayer(uint32_t accountId) {
    return dailyChallenges_;
}

void DailyChallengeSystem::updateProgress(entt::registry& registry, uint32_t accountId,
                                 ChallengeType type, uint32_t amount) {
    for (auto& prog : playerProgress_) {
        if (prog.challengeId == 0) continue;
        
        DailyChallenge* chal = nullptr;
        for (auto& c : dailyChallenges_) {
            if (c.id == prog.challengeId) {
                chal = &c;
                break;
            }
        }
        
        if (chal && chal->type == type && !prog.completed) {
            prog.progress += amount;
            
            if (prog.progress >= chal->target) {
                prog.completed = true;
                prog.completedAt = static_cast<uint32_t>(std::time(nullptr));
            }
        }
    }
}

void DailyChallengeSystem::claimRewards(entt::registry& registry, uint32_t accountId) {
    for (auto& prog : playerProgress_) {
        if (prog.completed) {
            DailyChallenge* chal = nullptr;
            for (auto& c : dailyChallenges_) {
                if (c.id == prog.challengeId) {
                    chal = &c;
                    break;
                }
            }
            
            if (chal) {
                // Grant rewards via PlayerProgression
                // Would add XP, gold here
                prog.completed = false;  // Reset after claim
                prog.progress = 0;
            }
        }
    }
}

uint32_t DailyChallengeSystem::getNextResetTime() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    
    // Next midnight UTC
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    tm->tm_mday++;
    
    std::time_t midnight = std::mktime(tm);
    return static_cast<uint32_t>(midnight);
}

bool DailyChallengeSystem::needsRegeneration() {
    return getDayOfYear() != lastGenerateDay_;
}

uint32_t DailyChallengeSystem::getDayOfYear() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    return static_cast<uint32_t>(tm->tm_yday);
}

} // namespace DarkAges