#pragma once

#include <cstdint>

namespace DarkAges {

/**
 * @brief Manages zone difficulty scaling for Hard Mode zones.
 *
 * Scales NPC health, damage, and XP rewards based on the zone's
 * difficulty multiplier. Applied at spawn time so CombatSystem
 * requires zero changes.
 *
 * difficultyMultiplier = 1.0f — Normal mode (baseline)
 * difficultyMultiplier = 1.5f — Hard mode (50% increase)
 * difficultyMultiplier = 2.0f — Nightmare mode (100% increase)
 */
class ZoneDifficultySystem {
public:
    explicit ZoneDifficultySystem(float multiplier = 1.0f)
        : difficultyMultiplier_(multiplier) {}

    // --- Setters / Getters ---

    void setDifficultyMultiplier(float multiplier) { difficultyMultiplier_ = multiplier; }
    [[nodiscard]] float getDifficultyMultiplier() const { return difficultyMultiplier_; }

    [[nodiscard]] bool isHardMode() const { return difficultyMultiplier_ > 1.01f; }

    // --- Scaling functions (applied at spawn time) ---

    /// Scale NPC max health by difficulty multiplier.
    [[nodiscard]] int16_t scaleHealth(int16_t baseHealth) const {
        if (difficultyMultiplier_ <= 1.0f) return baseHealth;
        float scaled = static_cast<float>(baseHealth) * difficultyMultiplier_;
        return static_cast<int16_t>(scaled + 0.5f);  // Round to nearest
    }

    /// Scale NPC base damage by difficulty multiplier.
    [[nodiscard]] uint16_t scaleDamage(uint16_t baseDamage) const {
        if (difficultyMultiplier_ <= 1.0f) return baseDamage;
        float scaled = static_cast<float>(baseDamage) * difficultyMultiplier_;
        return static_cast<uint16_t>(scaled + 0.5f);
    }

    /// Scale NPC XP reward by difficulty multiplier.
    [[nodiscard]] uint32_t scaleXpReward(uint32_t xpReward) const {
        if (difficultyMultiplier_ <= 1.0f) return xpReward;
        float scaled = static_cast<float>(xpReward) * difficultyMultiplier_;
        return static_cast<uint32_t>(scaled + 0.5f);
    }

    /// Scale NPC level offset (for boss level boost in hard mode).
    [[nodiscard]] uint8_t scaleLevel(uint8_t baseLevel) const {
        if (difficultyMultiplier_ <= 1.0f) return baseLevel;
        int extra = static_cast<int>((difficultyMultiplier_ - 1.0f) * 10.0f + 0.5f);
        return static_cast<uint8_t>(std::min<int>(255, baseLevel + extra));
    }

private:
    float difficultyMultiplier_{1.0f};
};

} // namespace DarkAges
