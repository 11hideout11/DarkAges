#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class RecoveryState final : public State {
public:
    RecoveryState();
    void Enter(Registry& registry, EntityID entity) override;
    StateStatus Update(Registry& registry, EntityID entity, float deltaSec) override;
    void Exit(Registry& registry, EntityID entity) override;
    const char* Name() const override { return "Recovery"; }
    State* GetNextState(CombatState* combat) const override;

private:
    float timer_ = 0.0f;
    float duration_ = 0.5f;  // derived from global cooldown window
};

} // namespace DarkAges::combat::detail
