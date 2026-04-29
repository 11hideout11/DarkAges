#pragma once

#include <cstdint>
#include <string>
#include <entt/entt.hpp>

namespace DarkAges {

// Forward declaration for CombatState (circular dependency; defined in CoreTypes.hpp)
struct CombatState;

// Type aliases matching ECS core types
using Registry = entt::registry;
using EntityID = entt::entity;

enum class StateStatus {
    Continue,  // State remains active
    Finish     // State completed; transition to next
};

class State {
public:
    virtual ~State() = default;
    virtual void Enter(Registry& registry, EntityID entity) = 0;
    virtual StateStatus Update(Registry& registry, EntityID entity, float deltaSec) = 0;
    virtual void Exit(Registry& registry, EntityID entity) = 0;
    virtual const char* Name() const = 0;
    virtual State* GetNextState(CombatState* combat) const = 0;
};

} // namespace DarkAges
