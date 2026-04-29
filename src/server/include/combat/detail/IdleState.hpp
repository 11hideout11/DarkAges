#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class IdleState final : public State {
public:
    IdleState();
    void Enter(Registry& registry, EntityID entity, const CombatConfig& /*config*/) override;
    StateStatus Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) override;
    void Exit(Registry& registry, EntityID entity) override;
    const char* Name() const override { return "Idle"; }
    State* GetNextState(CombatState* /*combat*/) const override;
};

} // namespace DarkAges::combat::detail

