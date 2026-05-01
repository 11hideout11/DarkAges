# PRD-008: CombatStateMachine Node-Based FSM Template

**Version:** 1.0
**Status:** 🔄 In Progress — Scene + Controller created, integration pending
**Owner:** COMBAT_AGENT
**Priority:** CRITICAL (P0 — MVP Blocking)
**Dependencies:** PRD-003 (Combat System), PRD-005 (Client)

---

## Implementation Status (2026-05-01)

### ✅ Completed
- [x] Created: `src/client/scenes/CombatStateMachine.tscn` - Visual FSM scene with AnimationTree
- [x] Created: `src/client/src/combat/fsm/CombatStateMachineController.cs` - Controller script
- [x] AnimationNodeStateMachine with 7 states (Idle, Walk, Run, Attack, Dodge, Hit, Death)
- [x] Global cooldown support (1.2s default)
- [x] Hit stop effect integration
- [x] Signal-based callbacks (StateEntered, StateExited, TransitionRequested, CooldownStarted)

### 📋 Pending Integration
- [ ] Connect CombatStateMachine.tscn to Player.tscn
- [ ] Connect CombatStateMachine.tscn to RemotePlayer.tscn
- [ ] Wire signals between AnimationStateMachine and CombatStateMachineController
- [ ] Test in Godot editor
- [ ] Run demo validation

### 📋 Deliverables Remaining
- Documentation: `docs/state-machine-usage.md`

---

## 1. Overview

### 1.1 Purpose

Create a reusable, node-based Finite State Machine (FSM) template for third-person combat that can be attached to any character entity in the Godot 4.2 client. This provides a visual, designer-friendly state machine that integrates with AnimationTree and drives combat state transitions, animation blending, and input blocking.

### 1.2 Scope

- `CombatStateMachine.tscn` — PackedScene resource containing the FSM node structure
- `CombatStateMachine.gd` or `CombatStateMachine.cs` — State machine controller script
- AnimationNodeStateMachine configuration within AnimationTree
- State transition rules, conditions, and callbacks
- Integration with `AnimationStateMachine.cs` (existing client-side FSM)
- Documentation for designers on how to use and extend the template

### 1.3 Out of Scope

- Combat balance tuning (PRP-003)
- New combat abilities (covered by separate PRDs)
- Server-side combat logic (CombatSystem already exists)

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| FSM-001 | CombatStateMachine.tscn scene file | P0 | PackedScene with proper node hierarchy |
| FSM-002 | Node-based state machine layout | P0 | Visual FSM in AnimationTree editor |
| FSM-003 | States: Idle, Moving, Sprinting, Attacking, Hit, Dodging, Dead | P0 | Matches AnimationStateMachine.StateType |
| FSM-004 | Transition conditions (blend times, input, cooldowns) | P0 | Configurable via AnimationNodeStateMachine |
| FSM-005 | State callbacks (enter/exit) | P1 | Hooks for ability logic |
| FSM-006 | Global cooldown integration | P1 | GCD blocks certain transitions |
| FSM-007 | Reusable across player, NPC, and remote entities | P2 | Template pattern |
| FSM-008 | Animation parameters driven by state | P0 | AnimationTree blend tree integration |

### 2.2 Technical Requirements

- **Godot 4.2 Mono** (C#) — matches project standard
- **Integration point**: Character body scene (Player.tscn, RemotePlayer.tscn)
- **AnimationTree**: Use `AnimationNodeStateMachine` as root
- **Crossfade**: 0.1s blend between states
- **State durations**: Configurable via export properties

### 2.3 UX Requirements

- Designers can open `.tscn` and edit FSM visually
- Clear state names and transition labels
- Conditions exposed as AnimationTree parameters (e.g., `is_attacking`, `is_dodging`)
- No code edits needed for basic state machine adjustments

---

## 3. Current Gap

**Gap:** While `AnimationStateMachine.cs` (C#) provides code-based state logic, the project lacks a **node-based FSM template scene** that designers can visually edit in the Godot editor. The new MVP criteria require "Proper FSM (node-based preferred)" for a reusable third-person combat template.

**Evidence:**
- Existing: `src/client/src/combat/fsm/AnimationStateMachine.cs` (330 lines, code-only)
- Missing: `CombatStateMachine.tscn` — visual state machine resource
- Missing: Dedicated AnimationTree `.tres` resource with configured state machine

**Impact:** Combat state logic is hardcoded; designers cannot iterate on state transitions, blend times, or add new states without modifying C# code. This violates the "reusable template" requirement and slows combat iteration.

---

## 4. Implementation Plan

### Phase 1: Create AnimationTree Resource

1. Create `assets/animations/CombatStateMachine.tres`:
   ```
   [resource type="AnimationTree" format=3 uid="..."]
   tree_root = SubResource(AnimationNodeStateMachine)
   ```
   - Configure `AnimationNodeStateMachine` with states
   - Add transitions with 0.1s crossfade
   - Set up parameters: `state`, `is_attacking`, `is_dodging`, `is_hit`, `is_dead`, `gcd_remaining`

2. Path: `/root/projects/DarkAges/assets/animations/CombatStateMachine.tres`

### Phase 2: Create CombatStateMachine.tscn

1. Create scene with node structure:
   ```
   CombatStateMachine (Node3D)
   ├── AnimationPlayer
   ├── AnimationTree (root = AnimationNodeStateMachine)
   └── [StateMachineController] (C# script)
   ```

2. Export properties:
   - `StateDuration_Attack: float = 0.5`
   - `StateDuration_Dodge: float = 0.4`
   - `StateDuration_Hit: float = 0.3`
   - `GlobalCooldown: float = 1.2`
   - `BlendTime: float = 0.1`

3. Save as: `/root/projects/DarkAges/src/client/scenes/CombatStateMachine.tscn`

### Phase 3: Controller Script

Create `CombatStateMachineController.cs`:
- Monitors `AnimationNodeStateMachine` state
- Emits signals: `StateEntered`, `StateExited`, `TransitionRequested`
- Manages GCD logic (blocks transitions while GCD active)
- Integrates with `InputManager` for attack/dodge input

Path: `/root/projects/DarkAges/src/client/src/combat/CombatStateMachineController.cs`

### Phase 4: Integration

1. Update `Player.tscn` to include `CombatStateMachine` node
2. Update `RemotePlayer.tscn` similarly
3. Wire `AnimationStateMachine.cs` to read state from `CombatStateMachine` node
4. Ensure state synchronization between client prediction and server

### Phase 5: Documentation

Create `docs/state-machine-usage.md`:
- How to open `.tscn` in Godot editor
- Adding new states (step-by-step)
- Configuring transition conditions
- Exporting as template for NPCs

---

## 5. Acceptance Criteria

**Must pass before merge:**

- [ ] `CombatStateMachine.tscn` opens in Godot 4.2 editor without errors
- [ ] All 7 states present in AnimationNodeStateMachine
- [ ] Transitions work (can move from Idle→Attacking→Idle via input)
- [ ] GCD blocks attack if still cooling down
- [ ] Player.tscn successfully instantiates CombatStateMachine
- [ ] Animation blend smooth (0.1s crossfade, verified visually)
- [ ] No test regressions (baseline: 2129 cases, 12644 assertions)
- [ ] Build passes: `cmake --build build_validate -j$(nproc)`
- [ ] All tests pass: `ctest --output-on-failure -j1`
- [ ] Documentation complete (`docs/state-machine-usage.md`)

---

## 6. Deliverables

| Item | Path | Type |
|------|------|------|
| FSM Scene | `src/client/scenes/CombatStateMachine.tscn` | Godot PackedScene |
| Animation Resource | `assets/animations/CombatStateMachine.tres` | AnimationTree resource |
| Controller Script | `src/client/src/combat/CombatStateMachineController.cs` | C# |
| Integration Updates | `src/client/scenes/Player.tscn`, `RemotePlayer.tscn` | Scene updates |
| Documentation | `docs/state-machine-usage.md` | Markdown |
| Unit Tests | `src/client/tests/TestCombatStateMachine.cs` | C# test |

---

## 7. Test Plan

### 7.1 Unit Tests (C#)

- `TestCombatStateMachine.cs`:
  - Test all state transitions
  - Test GCD blocking
  - Test invalid transitions (Hit → Attack blocked)
  - Test state duration timers
  - Test signal emissions

### 7.2 Integration Tests

- `TestCombatFSMIntegration.cpp` (existing): Ensure server still syncs correctly
- Visual validation via demo pipeline: Verify animations play correctly during combat

---

## 8. Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Breaks existing AnimationStateMachine.cs | Keep both; new node-based FSM replaces code-based one gradually |
| State desync between client/server | Server doesn't care about visual states — only cares about ability validation |
| Godot 4.2 AnimationTree API changes | Lock Godot version in `packages.config` (already at 4.2) |

---

## 9. Related Documents

- PRD-003: Combat System
- PRD-005: Client Architecture
- `src/client/src/combat/fsm/AnimationStateMachine.cs` — current FSM implementation
- `tools/demo/MVP_DEMO_STANDARDS.md` — Gap analysis

---

**Last Updated:** 2026-05-01
**PRD Author:** Autonomous Agent (gap analysis)
