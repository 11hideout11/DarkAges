# PRD-008: Combat State Machine (FSM)

**Version:** 1.0  
**Status:** 🟡 Planned  
**Owner:** CLIENT_AGENT  
**Priority:** CRITICAL  
**Dependencies:** PRD-003 (Combat System)

---

## 1. Overview

### 1.1 Purpose
Implement a formal node-based state machine for Godot 4.2 combat animations, replacing inline flags with a proper hierarchical FSM that supports concurrent animation states and modular transitions.

### 1.2 Scope
- CombatStateMachine scene (CombatStateMachine.tscn)
- Core states: Idle, Moving, Attacking, Hit, Dodging, Dead
- State transitions with transition guards
- Animation blending between concurrent states

---

## 2. Requirements

### 2.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| FSM-001 | CombatStateMachine.tscn as StateChart root | P0 | Godot 4.2 StateChart |
| FSM-002 | State nodes: Idle, Moving, Attacking, Hit, Dodging, Dead | P0 | Each as distinct scene |
| FSM-003 | Transition guards with condition expressions | P0 | e.g., can_dodge, can_attack |
| FSM-004 | One-shot transitions (Dodge -> Idle after 500ms) | P0 | Auto-exit duration |
| FSM-005 | Interruptible states (Hit interrupts Attack) | P1 | Priority-based preemption |
| FSM-006 | Concurrent blend support | P1 | 2D blend tree |

### 2.2 State Definitions

```
CombatStateMachine
├── IdleState (root default)
│   └── transitions: → Moving (on move_input), → Attacking (on attack_input), → Dead (on death)
├── MovingState
│   └── transitions: → Idle (on stop_input), → Attacking (on attack_input), → Dodging (on dodge_input)
├── AttackingState
│   └── transitions: → Idle (animation complete), → Hit (on received_damage, preempt)
├── HitState
│   └── transitions: → Idle (stagger duration), → Dead (if fatal)
└── DodgingState
    └── transitions: → Idle (dodge duration)
```

### 2.3 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| State Transition Latency | <16ms | <32ms |
| Blend Tree Evaluation | <2ms | <4ms |
| Memory Overhead | <10KB | <20KB |

---

## 3. Current Gap

**Gap:** CombatSystem exists but lacks formal node-based state machine. Uses inline state variables and manual if/else transitions.

**Location:** src/client/scripts/PredictedPlayer.cs (operational but needs refactor)

---

## 4. Implementation Strategy

### 4.1 Architecture
Use Godot 4.2 StateChart pattern from src/client/scenes/combat/:
- CombatStateMachine.tres (Resource)
- animations/ (AnimationLibrary)
- transitions/ (TransitionGraph)

### 4.2 State Transitions
- Guard evaluation order: check interrupt priority, evaluate conditions, execute match
- Transition guards using expressions: can_dodge, can_attack

### 4.3 Animation Blending
2D blend tree for concurrent animations (e.g., upper body attack while lower body moves)

---

## 5. Deliverables

### 5.1 Scene Files
src/client/scenes/combat/:
- CombatStateMachine.tscn (root FSM scene)
- states/ (IdleState, MovingState, AttackingState, HitState, DodgingState, DeadState)
- transitions/ (IdleToMoving, MovingToIdle, AnyToHit)
- guards/ (CanAttack, CanDodge, IsMoving)

### 5.2 Integration
- Player.tscn: Add CombatStateMachine child
- PredictedPlayer.cs: Remove inline flags, use FSM
- NetworkClient: Send state change events

---

## 6. Testing

| Test | Location | Criteria |
|------|----------|--------|
| State transitions | tests/ | All 12 transitions pass |
| Animation blending | E2E | No Z-fighting, smooth blend |
| Network sync | Integration | State events sent within 50ms |
| Performance | Benchmark | <2ms FSM tick |

---

## 7. Acceptance Criteria

- [ ] CombatStateMachine.tscn created with 6 states
- [ ] All state transitions documented and tested
- [ ] Attack combo (attack_1 -> attack_2 -> attack_3) works
- [ ] Dodge interrupt (any state -> Dodge -> previous) works
- [ ] Hit preempt (Attack -> Hit) works
- [ ] Death state (Hit -> Dead) on fatal damage
- [ ] No test regressions (baseline: 2,129 cases, 12,644 assertions)
- [ ] Build passes: cmake --build build -j$(nproc)

---

*Last Updated: 2026-05-01*
