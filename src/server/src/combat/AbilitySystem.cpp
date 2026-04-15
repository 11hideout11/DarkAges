#include "combat/AbilitySystem.hpp"
#include <entt/entt.hpp>

namespace DarkAges {

// ============================================================================
// Ability Implementation
// ============================================================================

bool AbilityDefinition::validate() const {
    return !name.empty() && manaCost > 0 && range > 0.0f;
}

// ============================================================================
// Ability System Implementation
// ============================================================================

bool AbilitySystem::castAbility(Registry& registry, EntityID caster,
                                 EntityID target, const AbilityDefinition& ability,
                                 uint32_t currentTimeMs) {
    return false;
}

const AbilityDefinition* AbilitySystem::getAbility(uint32_t abilityId) const {
    static AbilityDefinition dummy("Dummy", 1000, 10, 10.0f, AbilityEffectType::Damage);
    return &dummy;
}

bool AbilitySystem::hasMana(Registry& registry, EntityID caster, const AbilityDefinition& ability) const {
    return true;
}

void AbilitySystem::registerAbility(uint32_t abilityId, const AbilityDefinition& ability) {
    abilities_.emplace_back(abilityId, ability);
}

} // namespace DarkAges