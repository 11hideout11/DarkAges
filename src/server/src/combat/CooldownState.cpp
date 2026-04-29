#include "combat/CombatSystem.hpp"
#include "combat/detail/CooldownState.hpp"
#include "combat/detail/RecoveryState.hpp"

namespace DarkAges::combat::detail {

CooldownState::CooldownState() : timer_(0.0f), duration_(0.5f) {}

void CooldownState::Enter(Registry& registry, EntityID entity) {
    timer_ = 0.0f;
    if (const CombatConfig* cfg = registry.try_get<CombatConfig>(entity)) {
        duration_ = cfg->attackCooldownMs / 1000.0f;
    }
}

StateStatus CooldownState::Update(Registry& registry, EntityID entity, float dt) {
    timer_ += dt;
    if (timer_ >= duration_) {
        return StateStatus::Finish;
    }
    return StateStatus::Continue;
}

void CooldownState::Exit(Registry& registry, EntityID entity) {}

State* CooldownState::GetNextState(CombatState* /*combat*/) const {
    return new RecoveryState();
}

} // namespace DarkAges::combat::detail
