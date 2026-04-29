#pragma once

#include <cstdint>
#include <string>
#include <entt/entt.hpp>

namespace DarkAges {

// Forward declarations
struct CombatConfig;   // for FSM configuration access
struct CombatState;    // defined in CoreTypes.hpp

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
    virtual void Enter(Registry& registry, EntityID entity, const CombatConfig& config) = 0;
    virtual StateStatus Update(Registry& registry, EntityID entity, float deltaSec, uint32_t currentTimeMs) = 0;
    virtual void Exit(Registry& registry, EntityID entity) = 0;
    virtual const char* Name() const = 0;
    virtual State* GetNextState(CombatState* combat) const = 0;
};

} // namespace DarkAges

