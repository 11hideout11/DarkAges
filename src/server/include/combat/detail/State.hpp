#include "ecs/CoreTypes.hpp"

#pragma once

#include <cstdint>
#include <string>

namespace DarkAges {



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
    virtual State* GetNextState(class CombatState* combat) const = 0;
};

} // namespace DarkAges
