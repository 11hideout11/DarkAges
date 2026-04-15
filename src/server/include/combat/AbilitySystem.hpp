#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <string>
#include <vector>

// [COMBAT_AGENT] Ability system for DarkAges MMO
// Handles ability casting, cooldowns, mana costs, and effects

namespace DarkAges {

// ============================================================================
// Ability Effect Types
// ============================================================================

// [COMBAT_AGENT] Types of ability effects
enum class AbilityEffectType : uint8_t {
    Damage = 1,
    Heal = 2,
    Buff = 3,
    Debuff = 4,
    Status = 5
};

// ============================================================================
// Ability Definition
// ============================================================================

// [COMBAT_AGENT] Ability definition (detailed spec for ability system)
struct AbilityDefinition {
    std::string name;
    uint32_t cooldownMs;
    uint16_t manaCost;
    float range;
    AbilityEffectType effectType;

    AbilityDefinition() : name(""), cooldownMs(0), manaCost(0), range(0.0f), effectType(AbilityEffectType::Damage) {}

    AbilityDefinition(const std::string& n, uint32_t cd, uint16_t mana, float r, AbilityEffectType eff)
        : name(n), cooldownMs(cd), manaCost(mana), range(r), effectType(eff) {}

    bool validate() const;
};

// ============================================================================
// Ability System
// ============================================================================

// [COMBAT_AGENT] Ability casting system
class AbilitySystem {
public:
    AbilitySystem() = default;

    bool castAbility(Registry& registry, EntityID caster, EntityID target,
                     const AbilityDefinition& ability, uint32_t currentTimeMs);

    const AbilityDefinition* getAbility(uint32_t abilityId) const;

    bool hasMana(Registry& registry, EntityID caster, const AbilityDefinition& ability) const;

    void registerAbility(uint32_t abilityId, const AbilityDefinition& ability);

private:
    std::vector<std::pair<uint32_t, AbilityDefinition>> abilities_;
};

} // namespace DarkAges