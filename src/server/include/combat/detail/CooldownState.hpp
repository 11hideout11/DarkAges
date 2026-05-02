#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class CooldownState final : public State {
public:
    CooldownState();
    void Enter(Registry& registry, EntityID entity, const CombatConfig& config) override;
    StateStatus Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) override;
    void Exit(Registry& registry, EntityID entity) override;
    const char* Name() const override { return "Cooldown"; }
    State* GetNextState(CombatState* /*combat*/) const override;

private:
    float timer_ = 0.0f; // retained for potential compatibility, not used in absolute-time logic
    uint32_t attackCooldownMs_ = 0; // cooldown duration in milliseconds
};

} // namespace DarkAges::combat::detail

