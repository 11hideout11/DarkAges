#include "combat/CombatSystem.hpp"
#include "combat/detail/RecoveryState.hpp"
#include "combat/detail/IdleState.hpp"

namespace DarkAges::combat::detail {

RecoveryState::RecoveryState() : timer_(0.0f), duration_(0.5f) {}

void RecoveryState::Enter(Registry& registry, EntityID entity) {
    timer_ = 0.0f;
    // Recovery = global cooldown window minus (attack + active + cooldown)
    if (const CombatConfig* cfg = registry.try_get<CombatConfig>(entity)) {
        float totalGCD = cfg->globalCooldownMs / 1000.0f;
        float attackTotal = cfg->globalCooldownMs / 1000.0f;
        // active phase is single tick; use config.attackActiveMs if present
        attackTotal += 0.033f; // one-tick active window (~1 frame)
        float cooldown = cfg->attackCooldownMs / 1000.0f;
        duration_ = totalGCD - attackTotal - cooldown;
        if (duration_ < 0.05f) duration_ = 0.05f;  // clamp minimum
    }
}

StateStatus RecoveryState::Update(Registry& registry, EntityID entity, float dt) {
    timer_ += dt;
    if (timer_ >= duration_) {
        return StateStatus::Finish;
    }
    return StateStatus::Continue;
}

void RecoveryState::Exit(Registry& registry, EntityID entity) {}

State* RecoveryState::GetNextState(CombatState* /*combat*/) const {
    return new IdleState();
}

} // namespace DarkAges::combat::detail
