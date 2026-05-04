#pragma once

#include "ecs/CoreTypes.hpp"
#include "combat/ProgressionCalculator.hpp"
#include <entt/entt.hpp>

namespace DarkAges {

// ============================================================================
// NewGamePlusSystem — WP-3.3: New Game Plus
// Tracks player's New Game Plus level and applies scaling to enemies and rewards.
// ============================================================================

class NewGamePlusSystem {
public:
    NewGamePlusSystem() = default;
    
    // Initialize with reference to ProgressionCalculator for stat scaling
    void initialize(ProgressionCalculator* calculator) {
        calculator_ = calculator;
    }
    
    // Get the player's current NG+ level (for per-player tracking)
    [[nodiscard]] uint32_t getNGPlusLevel(entt::registry& registry, EntityID player) const;
    
    // Increase NG+ level (called when player completes all zones on current NG+)
    void increaseNGPlusLevel(entt::registry& registry, EntityID player);
    
    // Reset NG+ level to 0 (start over)
    void resetNGPlusLevel(entt::registry& registry, EntityID player);
    
    // Set global NG+ level for server-wide scaling
    void setGlobalNGPlusLevel(uint32_t level) { globalNGPlusLevel_ = level; }
    
    // Get global NG+ level
    [[nodiscard]] uint32_t getGlobalNGPlusLevel() const { return globalNGPlusLevel_; }
    
    // Get difficulty multiplier based on NG+ level (global or per-player)
    // 1.0 = normal, 1.1 = +10% per NG+ (harder enemies, better rewards)
    [[nodiscard]] float getDifficultyMultiplier(uint32_t ngPlusLevel) const {
        return 1.0f + (ngPlusLevel * 0.1f);
    }
    
    // Get XP multiplier (more XP on NG+)
    [[nodiscard]] float getXPMultiplier(uint32_t ngPlusLevel) const {
        return 1.0f + (ngPlusLevel * 0.2f);
    }
    
    // Get gold multiplier
    [[nodiscard]] float getGoldMultiplier(uint32_t ngPlusLevel) const {
        return 1.0f + (ngPlusLevel * 0.15f);
    }
    
private:
    ProgressionCalculator* calculator_{nullptr};
    uint32_t globalNGPlusLevel_{0};  // Server-wide NG+ level for enemy scaling
};

} // namespace DarkAges
