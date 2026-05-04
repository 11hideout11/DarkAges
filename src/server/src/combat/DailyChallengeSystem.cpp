#include <combat/DailyChallengeSystem.hpp>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <netcode/NetworkManager.hpp>
#include <netcode/Protocol.hpp>

namespace DarkAges {

DailyChallengeSystem::DailyChallengeSystem(NetworkManager* network) : network_(network) {
}

void DailyChallengeSystem::generateDailyChallenges() {
    dailyChallenges_.clear();
    
    // Seed with day of year
    uint32_t seed = getDayOfYear();
    std::srand(seed);
    
    // Generate 3 challenges
    for (int i = 0; i < 3; i++) {
        DailyChallenge chall;
        generateChallenge(chall, seed + i);
        dailyChallenges_.push_back(chall);
    }
    
    lastGenerateDay_ = static_cast<uint32_t>(std::time(nullptr));
}

std::vector<DailyChallenge> DailyChallengeSystem::getChallengesForPlayer(uint32_t accountId) {
    return dailyChallenges_;
}

void DailyChallengeSystem::updateProgress(entt::registry& registry, uint32_t accountId, 
                                        ChallengeType type, uint32_t amount) {
    bool found = false;
    for (auto& prog : playerProgress_) {
        if (prog.accountId == accountId) {
            found = true;
            // Find challenges of this type
            for (auto& c : dailyChallenges_) {
                if (c.type == type) {
                    if (prog.progress < c.target) {
                        prog.progress += amount;
                        if (prog.progress >= c.target) {
                            prog.completed = true;
                            prog.completedAt = static_cast<uint32_t>(std::time(nullptr));
                        }
                    }
                }
            }
        }
    }
    
    // If player not found, add new progress entry
    if (!found) {
        ChallengeProgress prog;
        prog.accountId = accountId;
        prog.progress = 0;
        prog.completed = false;
        prog.completedAt = 0;
        playerProgress_.push_back(prog);
    }
}

void DailyChallengeSystem::claimRewards(entt::registry& registry, uint32_t accountId) {
    for (auto& prog : playerProgress_) {
        if (prog.accountId == accountId) {
            for (auto& c : dailyChallenges_) {
                if (c.id == prog.challengeId && prog.completed) {
                    // Grant rewards
                    // XP
                    // Gold
                    // Items
                    prog.completed = false;  // Reset after claiming
                }
            }
        }
    }
}

uint32_t DailyChallengeSystem::getNextResetTime() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    
    // Set to midnight
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    tm->tm_mday++;  // Next day
    
    std::time_t midnight = std::mktime(tm);
    return static_cast<uint32_t>(midnight);
}

bool DailyChallengeSystem::needsRegeneration() {
    std::time_t now = std::time(nullptr);
    uint32_t today = static_cast<uint32_t>(now) / 86400;
    uint32_t lastDay = lastGenerateDay_ / 86400;
    return today != lastDay;
}

void DailyChallengeSystem::generateChallenge(DailyChallenge& chal, uint32_t seed) {
    std::srand(seed);
    chal.id = seed;
    // Simplified challenge generation
    chal.type = static_cast<ChallengeType>(seed % 6);
    chal.target = 10 + (seed % 20);
    chal.xpReward = 100 + (seed * 10);
    chal.goldReward = 50 + (seed * 5);
}

void DailyChallengeSystem::sendDailyChallengesToClient(ConnectionID connectionId) {
    if (!network_) return;
    
    if (needsRegeneration()) {
        generateDailyChallenges();
    }
    
    // For each challenge, create a packet and send
    for (const auto& challenge : dailyChallenges_) {
        DailyChallengePacket pkt;
        pkt.accountId = 1;  // Placeholder - should be based on connection
        pkt.challengeId = challenge.id;
        pkt.progress = 0;  // Placeholder
        pkt.xpReward = challenge.xpReward;
        pkt.goldReward = challenge.goldReward;
        pkt.itemReward = challenge.itemReward;
        
        // Find player progress if any
        auto it = std::find_if(playerProgress_.begin(), playerProgress_.end(),
            [challengeId = challenge.id](const ChallengeProgress& prog) {
                return prog.challengeId == challengeId;
            });
        
        if (it != playerProgress_.end()) {
            pkt.progress = it->progress;
        }
        
        network_->sendDailyChallengeUpdate(connectionId, pkt);
    }
}

uint32_t DailyChallengeSystem::getDayOfYear() {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    return static_cast<uint32_t>(tm->tm_yday);
}

} // namespace DarkAges
