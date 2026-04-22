#include "combat/AchievementSystem.hpp"
#include "ecs/Components.hpp"
#include <algorithm>

namespace DarkAges {

void AchievementSystem::initialize(
    const std::vector<AchievementDef>& achievements) {
    definitions_ = achievements;
    
    // Build ID->index map
    idToIndex_.clear();
    for (uint32_t i = 0; i < definitions_.size(); ++i) {
        idToIndex_[definitions_[i].id] = i;
    }
}

void AchievementSystem::attachToPlayer(Registry& registry, EntityID player) {
    auto& comp = registry.emplace<AchievementComponent>(player);
    comp.count = 0;
    comp.totalPoints = 0;
    
    // Initialize all achievements as tracked (not-progressed yet)
    for (uint32_t i = 0; i < AchievementComponent::MAX_ACHIEVEMENTS && 
         i < definitions_.size(); ++i) {
        comp.achievements[i].player = player;
        comp.achievements[i].achievementId = definitions_[i].id;
        comp.achievements[i].current = 0;
        comp.achievements[i].completed = false;
        comp.achievements[i].completedAt = 0;
    }
    comp.count = std::min(static_cast<uint32_t>(definitions_.size()),
                        AchievementComponent::MAX_ACHIEVEMENTS);
}

void AchievementSystem::detachFromPlayer(Registry& registry, EntityID player) {
    registry.remove<AchievementComponent>(player);
}

void AchievementSystem::updateProgress(Registry& registry, EntityID player,
                                     AchievementCategory category, 
                                     int32_t amount) {
    auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp) return;

    // Update progress for all achievements in this category
    for (uint32_t i = 0; i < comp->count; ++i) {
        auto& progress = comp->achievements[i];
        
        // Skip already completed
        if (progress.completed) continue;
        
        // Find definition
        auto it = idToIndex_.find(progress.achievementId);
        if (it == idToIndex_.end()) continue;
        
        const auto& def = definitions_[it->second];
        if (def.category != category) continue;
        
        // Update progress
        progress.current += amount;
        
        // Check if completed
        if (progress.current >= def.criteria) {
            unlockAchievement(registry, player, progress, def);
        }
    }
}

bool AchievementSystem::checkAchievements(Registry& registry, EntityID player) {
    auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp) return false;

    bool anyUnlocked = false;
    
    for (uint32_t i = 0; i < comp->count; ++i) {
        auto& progress = comp->achievements[i];
        if (progress.completed) continue;
        
        auto it = idToIndex_.find(progress.achievementId);
        if (it == idToIndex_.end()) continue;
        
        const auto& def = definitions_[it->second];
        
        if (progress.current >= def.criteria) {
            unlockAchievement(registry, player, progress, def);
            anyUnlocked = true;
        }
    }
    
    return anyUnlocked;
}

void AchievementSystem::unlockAchievement(Registry& registry, EntityID player,
                                          AchievementProgress& progress,
                                          const AchievementDef& def) {
    progress.completed = true;
    progress.current = def.criteria;  // Cap at criteria
    
    auto* comp = registry.try_get<AchievementComponent>(player);
    if (comp) {
        // Calculate points: base 10 + (criteria / 10)
        uint32_t points = 10 + static_cast<uint32_t>(def.criteria / 10);
        comp->totalPoints += points;
    }
    
    // Fire callback - let external code handle rewards
    if (achievementUnlockedCallback_) {
        achievementUnlockedCallback_(player, def, comp ? comp->totalPoints : 0);
    }
}

std::vector<AchievementProgress> AchievementSystem::getPlayerProgress(
    const Registry& registry, EntityID player) const {
    const auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp) return {};
    
    std::vector<AchievementProgress> result;
    for (uint32_t i = 0; i < comp->count; ++i) {
        result.push_back(comp->achievements[i]);
    }
    return result;
}

bool AchievementSystem::hasAchievement(
    const Registry& registry, EntityID player, uint32_t achievementId) const {
    const auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp) return false;
    
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].achievementId == achievementId) {
            return comp->achievements[i].completed;
        }
    }
    return false;
}

float AchievementSystem::getCompletionPercentage(
    const Registry& registry, EntityID player) const {
    const auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp || comp->count == 0) return 0.0f;
    
    int32_t completed = 0;
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].completed) ++completed;
    }
    
    return static_cast<float>(completed) / static_cast<float>(comp->count);
}

uint32_t AchievementSystem::getPlayerPoints(
    const Registry& registry, EntityID player) const {
    const auto* comp = registry.try_get<AchievementComponent>(player);
    return comp ? comp->totalPoints : 0;
}

const AchievementDef* AchievementSystem::getAchievementDef(
    uint32_t id) const {
    auto it = idToIndex_.find(id);
    if (it == idToIndex_.end()) return nullptr;
    return &definitions_[it->second];
}

AchievementProgress* AchievementSystem::findProgress(
    Registry& registry, EntityID player, uint32_t achievementId) {
    auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp) return nullptr;
    
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].achievementId == achievementId) {
            return &comp->achievements[i];
        }
    }
    return nullptr;
}

const AchievementProgress* AchievementSystem::findProgress(
    const Registry& registry, EntityID player, uint32_t achievementId) const {
    const auto* comp = registry.try_get<AchievementComponent>(player);
    if (!comp) return nullptr;
    
    for (uint32_t i = 0; i < comp->count; ++i) {
        if (comp->achievements[i].achievementId == achievementId) {
            return &comp->achievements[i];
        }
    }
    return nullptr;
}

} // namespace DarkAges