#pragma once

#include "ecs/CoreTypes.hpp"
#include "physics/NavigationGrid.hpp"
#include <cstdint>
#include <functional>
#include <unordered_map>

// NPC AI System — handles mob behavior: idle, wander, chase, attack, flee
// Uses a simple state machine: Idle -> Wander -> Chase -> Attack -> Flee -> Dead
// Supports optional A* pathfinding via NavigationGrid for obstacle avoidance

namespace DarkAges {

class CombatSystem;  // Forward declaration

class NPCAISystem {
public:
    NPCAISystem() = default;

    // Update all NPC AI states
    void update(Registry& registry, uint32_t currentTimeMs);

    // Set the combat system for NPC attacks
    void setCombatSystem(CombatSystem* cs) { combatSystem_ = cs; }

    // Set navigation grid for pathfinding (nullptr = disable pathfinding)
    void setNavigationGrid(NavigationGrid* grid) { navGrid_ = grid; }

    // Callback for when an NPC deals damage (for combat event logging)
    using DamageCallback = std::function<void(EntityID npc, EntityID target, int16_t damage)>;
    void setDamageCallback(DamageCallback cb) { damageCallback_ = std::move(cb); }

    // Clear cached paths for all NPCs (e.g., when obstacles change)
    void clearPaths();

    // Clear cached path for a specific NPC
    void clearPath(EntityID npc);

private:
    // Per-NPC pathfinding state
    struct NPCPathState {
        Path currentPath;
        uint32_t lastPathCalcMs{0};       // When path was last calculated
        uint32_t pathTargetEntity{0};     // Entity we're pathing toward (0 = static target)
        float targetX{0.0f};              // Last known target position
        float targetZ{0.0f};
    };

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

    // Move NPC toward a target position (direct line, no pathfinding)
    void moveToward(Registry& registry, EntityID npc, const Position& target, float speed);

    // Move NPC using pathfinding (follows A* path, falls back to direct line)
    void moveTowardWithPathfinding(Registry& registry, EntityID npc,
                                   const Position& target, float speed,
                                   uint32_t currentTimeMs);

    // Move NPC away from a target position
    void moveAway(Registry& registry, EntityID npc, const Position& threat, float speed);

    // Move NPC away using pathfinding (flees toward spawn point)
    void moveAwayWithPathfinding(Registry& registry, EntityID npc,
                                 const Position& threat, const Position& fleeGoal,
                                 float speed, uint32_t currentTimeMs);

    // Perform melee attack on target
    void performNPCAttack(Registry& registry, EntityID npc, EntityID target,
                          NPCAIState& ai, const NPCStats& stats, uint32_t currentTimeMs);

    // Set NPC velocity to zero (stop moving)
    void stopMovement(Registry& registry, EntityID npc);

    // Check if line of sight is clear between two positions
    bool hasLineOfSight(const Position& a, const Position& b) const;

    // Calculate or retrieve cached path for an NPC
    const Path& getOrCalcPath(EntityID npc, const Position& from,
                              float toX, float toZ, uint32_t currentTimeMs,
                              uint32_t targetEntity = 0);

    // Follow an existing path (move toward next waypoint)
    bool followPath(Registry& registry, EntityID npc, NPCPathState& pathState,
                    float speed, uint32_t currentTimeMs);

    // Convert Position to float coordinates
    static void posToFloat(const Position& pos, float& x, float& z);

    CombatSystem* combatSystem_{nullptr};
    NavigationGrid* navGrid_{nullptr};
    DamageCallback damageCallback_;

    // Per-NPC path state
    std::unordered_map<uint32_t, NPCPathState> pathStates_;

    // NPC movement speed (meters per second, stored as fixed-point-compatible)
    static constexpr float NPC_MOVE_SPEED = 4.0f;    // Normal chase speed
    static constexpr float NPC_FLEE_SPEED = 6.0f;    // Flee speed (faster)
    static constexpr float NPC_WANDER_SPEED = 2.0f;  // Wander speed (slower)

    // Pathfinding configuration
    static constexpr uint32_t PATH_RECALC_MS = 2000;  // Recalculate path every 2s
    static constexpr float TARGET_MOVED_THRESHOLD = 3.0f;  // Recalc if target moved 3m
    static constexpr float WAYPOINT_REACH_DIST = 1.0f;  // Consider waypoint reached at 1m
};

} // namespace DarkAges