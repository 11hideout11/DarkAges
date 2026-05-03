#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>

// ============================================================================
// New Game Plus System — WP-3.3 / PRD-040 Integration
//
// When a player completes all zones (tutorial → arena → boss), they become
// eligible to activate New Game Plus. This resets zone progression but
// increases overall difficulty by 10% per NG+ cycle.
//
// Integration with ZoneDifficultySystem:
//   effectiveDifficulty = baseDifficulty × (1.0 + ngPlusCount × 0.1)
//
// This feeds directly into ZoneDifficultySystem::setDifficultyMultiplier()
// so no changes to CombatSystem or NPCAISystem are needed.
// ============================================================================

namespace DarkAges {

class NewGamePlusSystem {
public:
    NewGamePlusSystem() = default;

    // --- Eligibility ---

    /// Check if player has completed all zones and is eligible for NG+
    [[nodiscard]] static bool isEligibleForNGPlus(const PlayerProgression& progression) {
        return progression.tutorialComplete
            && progression.arenaComplete
            && progression.bossComplete;
    }

    // --- Activation ---

    /// Activate New Game Plus for a player.
    /// Resets zone progression and increments the NG+ counter.
    static void activateNGPlus(PlayerProgression& progression) {
        if (!isEligibleForNGPlus(progression)) {
            return;  // Not eligible — no-op
        }

        // Increment NG+ cycle
        progression.ngPlusCount++;

        // Reset zone progression — player must clear zones again
        progression.tutorialComplete = false;
        progression.arenaComplete = false;
        progression.bossComplete = false;
        progression.highestZoneUnlocked = 98;  // Back to tutorial
    }

    // --- Scaling ---

    /// Compute the effective difficulty multiplier for a player at a given
    /// NG+ cycle. Applied on top of the zone's base difficulty.
    ///
    /// Formula: effective = baseMultiplier × (1.0 + ngPlusCount × 0.1)
    ///
    /// Examples:
    ///   NG+0, base 1.0 → 1.0   (normal)
    ///   NG+0, base 1.5 → 1.5   (hard mode, no NG+)
    ///   NG+1, base 1.0 → 1.1   (+10%)
    ///   NG+2, base 1.0 → 1.2   (+20%)
    ///   NG+1, base 1.5 → 1.65  (hard mode + NG+1)
    [[nodiscard]] static float getEffectiveDifficulty(float baseMultiplier,
                                                       uint32_t ngPlusCount) {
        if (ngPlusCount == 0) return baseMultiplier;
        const float ngPlusScaling = 1.0f + static_cast<float>(ngPlusCount) * 0.1f;
        return baseMultiplier * ngPlusScaling;
    }

    /// Get the NPC level offset for a given NG+ cycle.
    /// Per orchestration plan: enemy level × (1.1 per NG+)
    [[nodiscard]] static uint8_t getLevelOffset(uint32_t ngPlusCount) {
        if (ngPlusCount == 0) return 0;
        // 1 NG+ → +1 level, 2 NG+ → +2 levels, etc. (10% of base ≈ 1 per cycle)
        return static_cast<uint8_t>(std::min<uint32_t>(ngPlusCount, 255));
    }

    /// Get the XP reward multiplier for a given NG+ cycle.
    /// Higher NG+ = more risk = more reward
    [[nodiscard]] static float getXpMultiplier(uint32_t ngPlusCount) {
        if (ngPlusCount == 0) return 1.0f;
        return 1.0f + static_cast<float>(ngPlusCount) * 0.15f;  // +15% XP per NG+
    }

    /// Get the loot quality multiplier (affects drop rates) for a given NG+ cycle.
    [[nodiscard]] static float getLootMultiplier(uint32_t ngPlusCount) {
        if (ngPlusCount == 0) return 1.0f;
        return 1.0f + static_cast<float>(ngPlusCount) * 0.05f;  // +5% loot per NG+
    }
};

} // namespace DarkAges
