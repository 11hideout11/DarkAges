#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <vector>
#include <functional>
#include <string>

// [COMBAT_AGENT] Status effect system for DarkAges MMO
// Handles buffs, debuffs, DoTs, HoTs, and crowd control effects

namespace DarkAges {

// ============================================================================
// Status Effect Types
// ============================================================================

enum class StatusEffectType : uint8_t {
    Buff = 0,        // Positive stat modifier
    Debuff = 1,      // Negative stat modifier
    DoT = 2,         // Damage over time
    HoT = 3,         // Heal over time
    Stun = 4,        // Cannot move or attack
    Root = 5,        // Cannot move, can attack
    Silence = 6,     // Cannot cast abilities
    Slow = 7,        // Reduced movement speed
    Shield = 8,      // Absorbs damage
    Count
};

// Which stat an effect modifies
enum class StatType : uint8_t {
    None = 0,
    MovementSpeed = 1,
    AttackDamage = 2,
    AttackSpeed = 3,
    Armor = 4,
    HealthRegen = 5,
    ManaRegen = 6,
    Count
};

// ============================================================================
// Status Effect Definition
// ============================================================================

struct StatusEffect {
    uint32_t effectId{0};          // Unique instance ID
    uint32_t templateId{0};        // Template/ability that created this
    StatusEffectType type{StatusEffectType::Buff};
    StatType statType{StatType::None};

    // Timing
    uint32_t durationMs{0};        // Total duration (0 = permanent until removed)
    uint32_t appliedAtMs{0};       // When this was applied
    uint32_t expiresAtMs{0};       // When this expires (computed)

    // Periodic effects (DoT/HoT)
    uint32_t tickIntervalMs{0};    // How often to tick (0 = not periodic)
    uint32_t lastTickMs{0};        // Last time this ticked
    int16_t tickAmount{0};         // Amount per tick (positive=heal, negative=damage)

    // Stat modifiers (Buff/Debuff)
    float modifierPercent{0.0f};   // Percentage modifier (e.g., 0.2 = +20%)
    int16_t modifierFlat{0};       // Flat modifier

    // Shield
    int16_t shieldAbsorb{0};       // Damage remaining on shield

    // Stacking
    uint8_t stacks{1};             // Current stack count
    uint8_t maxStacks{1};          // Maximum stacks

    // Source tracking
    EntityID source{entt::null};   // Who applied this
    EntityID target{entt::null};   // Who has this

    // Metadata
    std::string name;              // Display name
    bool debuff{false};            // Can be cleansed if true

    [[nodiscard]] bool isExpired(uint32_t currentTimeMs) const {
        return durationMs > 0 && currentTimeMs >= expiresAtMs;
    }

    [[nodiscard]] bool shouldTick(uint32_t currentTimeMs) const {
        if (tickIntervalMs == 0) return false;
        return (currentTimeMs - lastTickMs) >= tickIntervalMs;
    }

    [[nodiscard]] bool isCrowdControl() const {
        return type == StatusEffectType::Stun ||
               type == StatusEffectType::Root ||
               type == StatusEffectType::Silence;
    }
};

// ============================================================================
// Aggregated Stat Modifiers
// ============================================================================

struct StatModifiers {
    float speedMultiplier{1.0f};
    float damageMultiplier{1.0f};
    float attackSpeedMultiplier{1.0f};
    float armorMultiplier{1.0f};
    float healthRegenMultiplier{1.0f};
    float manaRegenMultiplier{1.0f};
    int16_t shieldRemaining{0};

    // Crowd control flags
    bool stunned{false};
    bool rooted{false};
    bool silenced{false};

    void reset() {
        speedMultiplier = 1.0f;
        damageMultiplier = 1.0f;
        attackSpeedMultiplier = 1.0f;
        armorMultiplier = 1.0f;
        healthRegenMultiplier = 1.0f;
        manaRegenMultiplier = 1.0f;
        shieldRemaining = 0;
        stunned = false;
        rooted = false;
        silenced = false;
    }
};

// ============================================================================
// Active Effects Component (attaches to entities)
// ============================================================================

struct ActiveStatusEffects {
    static constexpr uint8_t MAX_EFFECTS = 16;

    StatusEffect effects[MAX_EFFECTS];
    uint8_t count{0};
    StatModifiers computedMods;     // Recomputed each tick
    bool dirty{true};               // Needs recomputation

    // Find effect index by effect ID, returns -1 if not found
    [[nodiscard]] int findIndex(uint32_t effectId) const {
        for (uint8_t i = 0; i < count; ++i) {
            if (effects[i].effectId == effectId) return static_cast<int>(i);
        }
        return -1;
    }

    // Find first effect of a given type
    [[nodiscard]] int findTypeIndex(StatusEffectType type) const {
        for (uint8_t i = 0; i < count; ++i) {
            if (effects[i].type == type) return static_cast<int>(i);
        }
        return -1;
    }

    [[nodiscard]] bool hasEffect(StatusEffectType type) const {
        return findTypeIndex(type) >= 0;
    }

    [[nodiscard]] bool isCrowdControlled() const {
        return computedMods.stunned || computedMods.rooted;
    }

    [[nodiscard]] bool canCastAbilities() const {
        return !computedMods.stunned && !computedMods.silenced;
    }

    [[nodiscard]] bool canMove() const {
        return !computedMods.stunned && !computedMods.rooted;
    }
};

// ============================================================================
// Status Effect System
// ============================================================================

class StatusEffectSystem {
public:
    StatusEffectSystem() = default;

    // Apply a new status effect to an entity
    // Returns the assigned effect ID, or 0 on failure
    uint32_t applyEffect(Registry& registry, EntityID target,
                         StatusEffect effect, uint32_t currentTimeMs);

    // Remove a specific effect by ID
    bool removeEffect(Registry& registry, EntityID target, uint32_t effectId);

    // Remove all effects of a given type
    uint8_t removeEffectsByType(Registry& registry, EntityID target, StatusEffectType type);

    // Remove all debuffs (cleanse)
    uint8_t cleanse(Registry& registry, EntityID target);

    // Remove all effects from an entity
    void clearAll(Registry& registry, EntityID target);

    // Update all status effects - call once per tick
    // Returns total damage/heal applied from periodic effects
    int16_t update(Registry& registry, uint32_t currentTimeMs);

    // Check if an entity has a specific effect type
    [[nodiscard]] bool hasEffect(const Registry& registry, EntityID target,
                                 StatusEffectType type) const;

    // Get computed stat modifiers for an entity
    [[nodiscard]] const StatModifiers& getModifiers(const Registry& registry,
                                                     EntityID target) const;

    // Absorb damage through shields. Returns remaining damage after absorption.
    int16_t absorbDamage(Registry& registry, EntityID target, int16_t damage);

    // Callback for when periodic damage is dealt (DoT ticks)
    void setOnPeriodicDamage(std::function<void(EntityID target, EntityID source, int16_t damage)> cb) {
        onPeriodicDamage_ = std::move(cb);
    }

    // Callback for when periodic healing is applied (HoT ticks)
    void setOnPeriodicHeal(std::function<void(EntityID target, int16_t amount)> cb) {
        onPeriodicHeal_ = std::move(cb);
    }

    // Next effect ID generator (public for testing)
    uint32_t nextEffectId() { return ++nextEffectId_; }

private:
    // Recompute aggregated stat modifiers from active effects
    void recomputeModifiers(ActiveStatusEffects& active);

    // Process a single periodic effect tick
    void processPeriodicTick(Registry& registry, StatusEffect& effect, uint32_t currentTimeMs);

    // Remove effect at index (swap with last)
    void removeAtIndex(ActiveStatusEffects& active, uint8_t index);

    uint32_t nextEffectId_{0};

    std::function<void(EntityID, EntityID, int16_t)> onPeriodicDamage_;
    std::function<void(EntityID, int16_t)> onPeriodicHeal_;
};

} // namespace DarkAges
