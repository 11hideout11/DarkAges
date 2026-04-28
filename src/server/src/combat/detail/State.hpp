#pragma once

#include <cstdint>
#include "ecs/CoreTypes.hpp"

namespace DarkAges {

// State return status from Update()
enum class StateStatus {
    Continue,     // State remains active
    Finished,     // State completed, machine should advance to next
    Interrupted   // State was externally interrupted (e.g., stun)
};

// Forward declarations
struct CombatState;
class Registry;

// ============================================================================
// State Interface
// ============================================================================
class State {
public:
    virtual ~State() = default;

    // Called when entering this state
    virtual void Enter(Registry& registry, EntityID entity) {}

    // Called when exiting this state
    virtual void Exit(Registry& registry, EntityID entity) {}

    // Tick the state; returns whether to continue in this state or transition
    // deltaMs: frame delta in milliseconds
    // currentTimeMs: server tick timestamp
    virtual StateStatus Update(Registry& registry, EntityID entity, float deltaMs, uint32_t currentTimeMs) = 0;

    [[nodiscard]] virtual const char* GetName() const = 0;
};

} // namespace DarkAges
