#pragma once

#include "State.hpp"

namespace DarkAges::combat::detail {

class AttackState final : public State {
public:
    AttackState();
    void Enter(Registry& registry, EntityID entity) override;
    StateStatus Update(Registry& registry, EntityID entity, float deltaSec) override;
    void Exit(Registry& registry, EntityID entity) override;
    const char* Name() const override { return "Attack"; }
    State* GetNextState(CombatState* combat) const override;

private:
    void createHitbox(Registry& registry, uint32_t attacker);
    void checkCollision(Registry& registry, uint32_t attacker);
    void removeHitbox(Registry& registry, uint32_t attacker);

    enum class Phase { Windup, Active, Recover };
    float timer_ = 0.0f;
    float windupDuration_ = 0.1f;
    float activeDuration_ = 0.033f;
    Phase phase_ = Phase::Windup;
    bool createdHitbox_ = false;
};

} // namespace DarkAges::combat::detail
