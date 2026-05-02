#include "combat/CombatSystem.hpp"
#include "combat/detail/CooldownState.hpp"
#include "combat/detail/RecoveryState.hpp"

namespace DarkAges::combat::detail {

CooldownState::CooldownState() = default;

void CooldownState::Enter(Registry& registry, EntityID entity, const CombatConfig& config) {
    timer_ = 0.0f;
    attackCooldownMs_ = config.attackCooldownMs;
}

StateStatus CooldownState::Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) {
    if (const CombatState* cs = registry.try_get<CombatState>(entity)) {
        // Use absolute time to ensure cooldown respects real time even if FSM updates are skipped
        if (cs->lastAttackTime > 0 && currentTimeMs >= cs->lastAttackTime + attackCooldownMs_) {
            return StateStatus::Finish;
        }
    }
    return StateStatus::Continue;
}

void CooldownState::Exit(Registry& registry, EntityID entity) {}

State* CooldownState::GetNextState(CombatState* /*combat*/) const {
    return new RecoveryState();
}

} // namespace DarkAges::combat::detail

