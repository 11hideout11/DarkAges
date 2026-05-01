# PRD: Node-Based FSM Implementation
## Introduction
The combat system currently uses inline state flags (e.g., `_isAttacking`, `_isDead`, `_isDodgeRolling`) rather than a formal finite state machine architecture. This limits extensibility and makes complex state logic difficult to manage.
**Problem Statement:** Inline state flags create coupling between state checks, make adding new states error-prone, and prevent code reuse across entities.
---

## Goals
- Implement node-based FSM pattern for all combat states
- Provide reusable state base class with enter/exit/update/handle_input
- Achieve clean state transitions with signals
- Support state hierarchy for nested states
- Maintain testability with mockable states
---

## User Stories
### US-001: Implement State Base Class
**Description:** As a developer, I need a State base class so I can create reusable state implementations.
**Acceptance Criteria:**
- [ ] State class with virtual Enter/Exit/Update/HandleInput methods
- [ ] CharacterBody3D reference stored
- [ ] finished signal emitted on state completion
- [ ] Generic type parameter for character
- [ ] Unit tests for base transitions

### US-002: Create Combat States
**Description:** As a developer, I need combat states (Idle, Walk, Attack, HitReaction, Death, Dodge) so combat feels responsive.
**Acceptance Criteria:**
- [ ] IdleState: blend space transitions, rotation behavior
- [ ] WalkState: velocity-based animation blending
- [ ] AttackState: timing validation, hitbox activation
- [ ] HitReactionState: stun duration, knockback
- [ ] DeathState: cleanup, respawn timer
- [ ] DodgeState: invulnerability frames, distance

### US-003: Integrate with AnimationTree
**Description:** As an animator, I need states to drive AnimationTree parameters so animations are smooth.
**Acceptance Criteria:**
- [ ] State transitions set AnimationTree values
- [ ] Blend tree positions updated on state enter
- [ ] Animation events trigger state transitions
- [ ] Root motion extraction correct
- [ ] Hitbox timing synced to animations

### US-004: Implement State Hierarchy
**Description:** As a developer, I need hierarchical states so similar states can share logic.
**Acceptance Criteria:**
- [ ] Parent state with shared enter/exit logic
- [ ] Child states inherit base behavior
- [ ] Pre/post callbacks for override points
- [ ] State history for debugging
- [ ] Guard conditions for transitions
---

## Functional Requirements
- FR-1: Create State base class in `src/client/src/states/State.cs`
- FR-2: Create CharacterStateMachine controller
- FR-3: Implement Idle, Walk, Run, Jump, Fall, Land states
- FR-4: Implement Attack1H, Attack2H, HeavyAttack states
- FR-5: Implement HitReaction, Stagger, Knockback states
- FR-6: Implement Death, Respawn states
- FR-7: Implement Dodge, Roll states
- FR-8: Create AnimationTree state mappings
- FR-9: Add unit tests for state machine
- FR-10: Update PredictedPlayer to use FSM
---

## Non-Goals
- No AI behavior states (NPC uses separate FSM)
- No dialogue/conversation states
- No UI interaction states
- No inventory management states
- No network prediction in FSM (handled separately)
---

## Technical Considerations
### Base Class Template
```csharp
public abstract class State : Node
{
    public signal finished(next_state: StringName);
    
    protected CharacterBody3D character;
    protected StateMachine stateMachine;
    
    public virtual void Enter() {}
    public virtual void Exit() {}
    public virtual void Update(delta: double) {}
    public virtual void HandleInput(@event: InputEvent) {}
    public virtual bool CanEnter() { return true; }
}
```
### Transition Pattern
```csharp
// In state implementation:
public override void Update(delta) {
    if (should_transition) {
        EmitSignal("finished", "next_state_name");
    }
}
```
---

## Success Metrics
- All existing functionality preserved (2,129 tests pass)
- New states added without modifying existing code
- State transitions traceable in logs
- FSM test coverage >90%
---

## Open Questions
1. Should NPC AI share state base class?2. Is Godot State Charts plugin preferred?
3. Should states be scriptable by designers?
4. What is the rollback strategy if FSM causes issues?
---