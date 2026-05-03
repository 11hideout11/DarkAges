# PRD: Godot State Charts Migration

## Introduction

The DarkAges client currently uses inline boolean flags for combat/state management (e.g., isAttacking, isDead, isHit). This creates ambiguous state transitions and limits combat depth. Per CURRENT_STATUS.md, the project should migrate to Godot State Charts for proper finite state machine (FSM) architecture using the StateChart node.

## Goals

- Migrate combat state from inline flags to StateChart
- Implement proper state transition guards
- Enable visual debugging of state machine
- Support substate nesting for complex behaviors

## User Stories

### SC-001: Attack State Machine
**Description:** As a combat designer, I want attack states to have clear transitions.

**Acceptance Criteria:**
- [ ] States: Idle -> Attack -> Recovery -> Idle
- [ ] Guard prevents attack during attack state
- [ ] Event triggers transition to attack state
- [ ] Timer triggers recovery completion

### SC-002: Damage State Machine
**Description:** As a combat designer, I want damage states with proper entry/exit.

**Acceptance Criteria:**
- [ ] States: Normal -> Hit -> Normal
- [ ] Invincible state during hitstun
- [ ] Death transition when HP <= 0
- [ ] Respawn after timer

### SC-003: Movement State Machine
**Description:** As a game designer, I want movement states properly separated.

**Acceptance Criteria:**
- [ ] States: Idle, Walk, Run, Sprint, Jump, Fall
- [ ] Velocity-based transitions
- [ ] Ground check guards

### SC-004: Debug Visualization
**Description:** As a developer, I want to visualize active state for debugging.

**Acceptance Criteria:**
- [ ] Active state highlighted in editor
- [ ] State transitions logged
- [ ] Console command to dump state

## Functional Requirements

- FR-1: Add StateChart node to Player scene
- FR-2: Define state machine in .statechart file
- FR-3: Add guarded transitions with conditions
- FR-4: Connect states to animation tree
- FR-5: Debug draw active state

## Non-Goals

- No client-side state prediction (keep server authoritative)
- No network protocol changes (internal only)
- No complex nested substates initially
- No state persistence across scenes

## Technical Considerations

### Godot 4.2 StateChart Format
```xml
<!-- player_combat.statechart -->
<statechart>
  <initial>Idle</initial>
  
  <state name="Idle">
    <transition to="Attack" event="attack"/>
  </state>
  
  <state name="Attack">
    <on_enter>
      trigger_animation("attack")
      start_cooldown()
    </on_enter>
    <transition to="Recovery" after="500"/>
  </state>
  
  <state name="Recovery">
    <on_enter>
      trigger_animation("recovery")
    </on_enter>
    <transition to="Idle" after="300"/>
  </state>
  
  <state name="Hit">
    <transition to="Idle" event="damage" guard="health > 0"/>
    <transition to="Dead" guard="health <= 0"/>
  </state>
  
  <state name="Dead">
    <transition to="Idle" after="respawn_time"/>
  </state>
</statechart>
```

### Connection to AnimationTree
```gdscript
# Player.gd
func _on_state_changed(old_state, new_state):
    animation_tree.set("parameters/combat_state", new_state)
```

### Integration Points
- PredictedPlayer.cs current state -> StateChart
- CombatSystem combat events -> StateChart events
- AnimationTree parameter binding

## Success Metrics

- All combat states use StateChart
- No unreachable states
- Transition guards prevent invalid transitions
- Debug visualization functional

## Open Questions

- Should player movement and combat use separate charts?
- Do we need parallel states (movement + combat)?
- What state for dodge/roll action?