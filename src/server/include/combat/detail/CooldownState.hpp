#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class CooldownState final : public State {
public:
    CooldownState();
    void Enter(Registry& registry, uint32_t entity) override;
    StateStatus Update(Registry& registry, uint32_t entity, float deltaSec) override;
    void Exit(Registry& registry, uint32_t entity) override;
    const char* Name() const override { return "Cooldown"; }
    State* GetNextState(CombatState* combat) const override;

private:
    float timer_ = 0.0f;
    float duration_ = 0.5f;  // default 500ms
};

} // namespace DarkAges::combat::detail
