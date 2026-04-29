#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class IdleState final : public State {
public:
    IdleState();
    void Enter(Registry& registry, uint32_t entity) override;
    StateStatus Update(Registry& registry, uint32_t entity, float deltaSec) override;
    void Exit(Registry& registry, uint32_t entity) override;
    const char* Name() const override { return "Idle"; }
    State* GetNextState(CombatState* /*combat*/) const override;
};

} // namespace DarkAges::combat::detail
