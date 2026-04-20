#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>

// NPC AI System — handles mob behavior: idle, wander, chase, attack, flee
// Uses a simple state machine: Idle -> Wander -> Chase -> Attack -> Flee -> Dead

namespace DarkAges {

class CombatSystem;  // Forward declaration

class NPCAISystem {
public:
    NPCAISystem() = default;

    // Update all NPC AI states
    void update(Registry& registry, uint32_t currentTimeMs);

    // Set the combat system for NPC attacks
    void setCombatSystem(CombatSystem* cs) { combatSystem_ = cs; }

    // Callback for when an NPC deals damage (for combat event logging)
    using DamageCallback = std::function<void(EntityID npc, EntityID target, int16_t damage)>;
    void setDamageCallback(DamageCallback cb) { damageCallback_ = std::move(cb); }

private:
    // Behavior state handlers
    void updateIdle(Registry& registry, EntityID npc, NPCAIState& ai,
                    const Position& pos, uint32_t currentTimeMs, uint32_t dtMs);
    void updateWander(Registry& registry, EntityID npc, NPCAIState& ai,
                      const Position& pos, uint32_t currentTimeMs, uint32_t dtMs);
    void updateChase(Registry& registry, EntityID npc, NPCAIState& ai,
                     const Position& pos, uint32_t currentTimeMs, uint32_t dtMs);
    void updateAttack(Registry& registry, EntityID npc, NPCAIState& ai,
                      const Position& pos, uint32_t currentTimeMs, uint32_t dtMs);
    void updateFlee(Registry& registry, EntityID npc, NPCAIState& ai,
                    const Position& pos, uint32_t currentTimeMs, uint32_t dtMs);
    void updateDead(Registry& registry, EntityID npc, NPCAIState& ai,
                    uint32_t currentTimeMs, uint32_t dtMs);

    // Find nearest player within aggro range
    EntityID findNearestPlayer(Registry& registry, const Position& pos, float aggroRange) const;

    // Check if target is valid and alive
    bool isTargetValid(Registry& registry, EntityID target) const;

    // Calculate distance squared between two positions
    float distanceSq(const Position& a, const Position& b) const;

    // Move NPC toward a target position
    void moveToward(Registry& registry, EntityID npc, const Position& target, float speed);

    // Move NPC away from a target position
    void moveAway(Registry& registry, EntityID npc, const Position& threat, float speed);

    // Perform melee attack on target
    void performNPCAttack(Registry& registry, EntityID npc, EntityID target,
                          NPCAIState& ai, const NPCStats& stats, uint32_t currentTimeMs);

    // Set NPC velocity to zero (stop moving)
    void stopMovement(Registry& registry, EntityID npc);

    CombatSystem* combatSystem_{nullptr};
    DamageCallback damageCallback_;

    // NPC movement speed (meters per second, stored as fixed-point-compatible)
    static constexpr float NPC_MOVE_SPEED = 4.0f;    // Normal chase speed
    static constexpr float NPC_FLEE_SPEED = 6.0f;    // Flee speed (faster)
    static constexpr float NPC_WANDER_SPEED = 2.0f;  // Wander speed (slower)
};

} // namespace DarkAges