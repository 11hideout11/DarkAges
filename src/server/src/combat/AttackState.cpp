#include "combat/CombatSystem.hpp"
#include "combat/detail/AttackState.hpp"

namespace DarkAges::combat::detail {

using namespace Constants;

AttackState::AttackState()
    : timer_(0.0f), windupDuration_(0.1f), activeDuration_(0.033f),
      phase_(Phase::Windup), createdHitbox_(false) {}

void AttackState::Enter(Registry& registry, uint32_t entity) {
    timer_ = 0.0f;
    phase_ = Phase::Windup;
    createdHitbox_ = false;

    // Pull attack windup from entity's CombatConfig
    if (const CombatConfig* cfg = registry.try_get<CombatConfig>(entity)) {
        windupDuration_ = cfg->attackWindupMs / 1000.0f;
    }
    // TODO: Play windup animation on client via network
}

StateStatus AttackState::Update(Registry& registry, uint32_t entity, float dt) {
    timer_ += dt;

    if (phase_ == Phase::Windup) {
        if (timer_ >= windupDuration_) {
            phase_ = Phase::Active;
            timer_ = 0.0f;
        }
        return StateStatus::Continue;
    }

    if (phase_ == Phase::Active) {
        if (!createdHitbox_) {
            createHitbox(registry, entity);
            createdHitbox_ = true;
            checkCollision(registry, entity);
        }
        // One-tick active phase; then recover
        if (timer_ >= activeDuration_) {
            phase_ = Phase::Recover;
            timer_ = 0.0f;
        }
        return StateStatus::Continue;
    }

    // Recover: wait until total attack duration passes
    float totalDuration = windupDuration_ + activeDuration_ + 0.1f;  // minimal recover
    if (timer_ >= totalDuration) {
        return StateStatus::Finish;
    }
    return StateStatus::Continue;
}

void AttackState::Exit(Registry& registry, uint32_t entity) {
    // Ensure any lingering hitbox is cleared
    removeHitbox(registry, entity);
}

State* AttackState::GetNextState(CombatState* /*combat*/) const {
    return new CooldownState();
}

// Simple spherical hitbox centered in front of attacker
void AttackState::createHitbox(Registry& registry, uint32_t attacker) {
    const Position* pos = registry.try_get<Position>(attacker);
    const Rotation* rot = registry.try_get<Rotation>(attacker);
    if (!pos || !rot) return;

    Hitbox hb;
    hb.entity = attacker;
    hb.radius = 2.0f * FIXED_TO_FLOAT;  // 2 world units
    hb.origin = { pos->x * FIXED_TO_FLOAT, pos->y * FIXED_TO_FLOAT, pos->z * FIXED_TO_FLOAT };

    glm::vec3 forward = getForwardVector(rot->yaw);
    hb.origin += forward * (1.5f * FIXED_TO_FLOAT);

    hb.startTime = std::chrono::steady_clock::now();
    registry.emplace_or_replace<Hitbox>(attacker, hb);
}

void AttackState::checkCollision(Registry& registry, uint32_t attacker) {
    const Hitbox* hb = registry.try_get<Hitbox>(attacker);
    if (!hb) return;

    auto view = registry.view<Position, CombatState>();
    view.each([&](uint32_t target, const Position& targetPos, CombatState& targetCombat) {
        if (target == attacker) return;
        if (targetCombat.isDead) return;

        float dx = (targetPos.x * FIXED_TO_FLOAT) - hb->origin.x;
        float dy = (targetPos.y * FIXED_TO_FLOAT) - hb->origin.y;
        float dz = (targetPos.z * FIXED_TO_FLOAT) - hb->origin.z;
        float distSq = dx*dx + dy*dy + dz*dz;

        if (distSq <= hb->radius * hb->radius) {
            // HIT — apply damage server-authoritatively
            applyDamage(registry, target, attacker, 25, 0);
        }
    });
}

void AttackState::removeHitbox(Registry& registry, uint32_t attacker) {
    registry.remove<Hitbox>(attacker);
}

} // namespace DarkAges::combat::detail
