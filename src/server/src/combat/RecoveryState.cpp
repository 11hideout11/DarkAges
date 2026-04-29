#include "combat/CombatSystem.hpp"
#include "combat/detail/RecoveryState.hpp"
#include "combat/detail/IdleState.hpp"

namespace DarkAges::combat::detail {

RecoveryState::RecoveryState() : timer_(0.0f), duration_(0.5f) {}

void RecoveryState::Enter(Registry& registry, EntityID entity, const CombatConfig& config) {
    timer_ = 0.0f;
    // RecoveryTime = GlobalCooldown - (AttackWindup + AttackActive + Cooldown)
    constexpr float ACTIVE_DURATION = 0.033f; // one-tick active window (~1 frame)
    float totalGCD = config.globalCooldownMs / 1000.0f;
    float windup = config.attackWindupMs / 1000.0f;
    float cooldown = config.attackCooldownMs / 1000.0f;
    duration_ = totalGCD - windup - ACTIVE_DURATION - cooldown;
    if (duration_ < 0.05f) duration_ = 0.05f;  // clamp minimum to avoid negative/zero
}

StateStatus RecoveryState::Update(Registry& registry, EntityID entity, float dt, uint32_t currentTimeMs) {
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

