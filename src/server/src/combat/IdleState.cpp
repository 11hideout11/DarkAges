#include "combat/detail/IdleState.hpp"

namespace DarkAges::combat::detail {

IdleState::IdleState() = default;

void IdleState::Enter(Registry& registry, EntityID entity, const CombatConfig& /*config*/) {
    // No per-entity initialization required; idle is default resting state
}

StateStatus IdleState::Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) {
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

