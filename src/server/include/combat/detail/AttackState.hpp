#pragma once

#include "State.hpp"

// Forward declaration for configuration; full definition in CombatSystem.hpp
struct CombatConfig;

namespace DarkAges::combat::detail {

class AttackState final : public State {
public:
    AttackState();
    void Enter(Registry& registry, EntityID entity, const CombatConfig& config) override;
    StateStatus Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) override;
    void Exit(Registry& registry, EntityID entity) override;
    const char* Name() const override { return "Attack"; }
    State* GetNextState(CombatState* combat) const override;

private:
    void createHitbox(Registry& registry, EntityID attacker);
    bool checkCollision(Registry& registry, EntityID attacker);  // returns true if any hit
    void removeHitbox(Registry& registry, EntityID attacker);

    enum class Phase { Windup, Active, Recover };
    float timer_ = 0.0f;
    float windupDuration_ = 0.1f;
    float activeDuration_ = 0.033f;
    Phase phase_ = Phase::Windup;
    bool createdHitbox_ = false;
    const CombatConfig* config_ = nullptr;  // pointer to combat configuration (non-owning)
};

} // namespace DarkAges::combat::detail

