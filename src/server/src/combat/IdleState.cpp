#include "combat/CombatSystem.hpp"
#include "combat/detail/IdleState.hpp"

namespace DarkAges::combat::detail {

IdleState::IdleState() = default;

void IdleState::Enter(Registry& registry, EntityID entity) {
    // Reset state timer if needed
    // CombatState* combat = registry.try_get<CombatState>(entity);
    // if (combat) { combat->stateTimer = 0.0f; }
}

StateStatus IdleState::Update(Registry& registry, EntityID entity, float dt) {
    // Idle state persists indefinitely until externally transitioned
    return StateStatus::Continue;
}

void IdleState::Exit(Registry& registry, EntityID entity) {
    // Nothing to clean up
}

State* IdleState::GetNextState(CombatState* /*combat*/) const {
    // No automatic transition from idle; external input required
    return nullptr;
}

} // namespace DarkAges::combat::detail
