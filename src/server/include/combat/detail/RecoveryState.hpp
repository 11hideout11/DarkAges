#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class RecoveryState final : public State {
public:
    RecoveryState();
    void Enter(Registry& registry, EntityID entity, const CombatConfig& config) override;
    StateStatus Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) override;
    void Exit(Registry& registry, EntityID entity) override;
    const char* Name() const override { return "Recovery"; }
    State* GetNextState(CombatState* /*combat*/) const override;

private:
    float timer_ = 0.0f;
    float duration_ = 0.5f;  // computed from global cooldown
};

} // namespace DarkAges::combat::detail

