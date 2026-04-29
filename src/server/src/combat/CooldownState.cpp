#include "combat/CombatSystem.hpp"
#include "combat/detail/CooldownState.hpp"
#include "combat/detail/RecoveryState.hpp"

namespace DarkAges::combat::detail {

CooldownState::CooldownState() : timer_(0.0f), duration_(0.5f) {}

void CooldownState::Enter(Registry& registry, uint32_t entity) {
    timer_ = 0.0f;
    if (const CombatConfig* cfg = registry.try_get<CombatConfig>(entity)) {
        duration_ = cfg->attackCooldownMs / 1000.0f;
    }
}

StateStatus CooldownState::Update(Registry& registry, uint32_t entity, float dt) {
    timer_ += dt;
    if (timer_ >= duration_) {
        return StateStatus::Finish;
    }
    return StateStatus::Continue;
}

void CooldownState::Exit(Registry& registry, uint32_t entity) {}

State* CooldownState::GetNextState(CombatState* /*combat*/) const {
    return new RecoveryState();
}

} // namespace DarkAges::combat::detail
