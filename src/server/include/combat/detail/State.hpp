#pragma once

#include <cstdint>
#include <string>

namespace DarkAges {

struct Registry;  // forward (EnTT registry defined elsewhere)

enum class StateStatus {
    Continue,  // State remains active
    Finish     // State completed; transition to next
};

class State {
public:
    virtual ~State() = default;
    virtual void Enter(Registry& registry, uint32_t entity) = 0;
    virtual StateStatus Update(Registry& registry, uint32_t entity, float deltaSec) = 0;
    virtual void Exit(Registry& registry, uint32_t entity) = 0;
    virtual const char* Name() const = 0;
    virtual State* GetNextState(class CombatState* combat) const = 0;
};

} // namespace DarkAges
