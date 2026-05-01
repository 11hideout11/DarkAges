# PRD-019: AnimationTree Blend Space System

**Version:** 1.0
**Status:** 🔄 Not Started — Current AnimationTree lacks blend space implementation
**Owner:** ANIMATION_AGENT
**Priority:** MEDIUM (P3 — Visual Polish, not MVP-critical but quality requirement)
**Dependencies:** PRD-008 (CombatStateMachine FSM — for parameter-driven blends)
**Issue:** #7 from PROJECT_ISSUES_TRACKER.md

---

## 1. Overview

### 1.1 Purpose
Implement **blend spaces** in the `AnimationTree` to procedurally blend between movement and combat animations based on player velocity, input direction, and combat state. This replaces hard-coded animation transitions with smooth, parameter-driven interpolation.

### 1.2 Problem

**Current AnimationTree setup (PlayerAnimations.tres):**
```
AnimationTree (node-based)
└── AnimationNodeStateMachine
    ├── Idle → Walk → Run (discrete transitions, NOT blended)
    ├── Attack1 → Attack2 → Attack3 (no speed scaling)
    └── Dodge (single animation, no directional blending)
```

**Problems:**
- Walk/Run are discrete states → requires state machine transition (visible pop)
- No speed-based blending (e.g., slow walk vs. sprint)
- No directional blend (move-forward vs. strafe-left vs. backpedal)
- No combat blend (moving while attacking not smoothly integrated)

**Solution:** Use `AnimationNodeBlendSpace1D` and `AnimationNodeBlendSpace2D` to blend animations continuously based on floating-point parameters.

---

## 2. Requirements

### 2.1 Blend Space Definitions

| Blend Space | Type | Parameters | Animations |
|-------------|------|------------|------------|
| **Locomotion** | BlendSpace2D | horizontalSpeed × verticalSpeed | Idle, Walk, Run, Backpedal, StrafeLeft, StrafeRight |
| **CombatMovement** | BlendSpace1D | attackIntent (0-1) | CombatIdle, CombatWalk, CombatRun |
| **TurnInPlace** | BlendSpace1D | turnAngle (-180° to +180°) | TurnLeft90, TurnRight90 |
| **DodgeDirection** | BlendSpace1D | dodgeDir (0=forward, 1=left, 2=right, 3=back) | Dodge_F, Dodge_L, Dodge_R, Dodge_B |

### 2.2 Animation Parameters (AnimationTree Parameters)

New parameters to drive blends:

| Parameter | Type | Range | Source |
|-----------|------|-------|--------|
| `movement_speed` | float | 0.0 — 10.0 m/s | Velocity component length |
| `direction_x` | float | -1.0 — 1.0 | Input vector X (-left, +right) |
| `direction_z` | float | -1.0 — 1.0 | Input vector Z (-forward, +backward) |
| `combat_mode` | bool | false/true | CombatStateMachine.IsInCombat |
| `attack_intent` | float | 0.0 — 1.0 | IsAttackButtonDown (pre-attent windup) |
| `turn_angle` | float | -π — +π | Angle between desired direction and current facing |
| `dodge_direction` | int | 0-3 | Dodge input direction enum |

---

## 3. Blend Space Design

### 3.1 Locomotion BlendSpace2D (2D)

X-axis: `direction_x` (-1 strafe left → +1 strafe right)
Y-axis: `direction_z` (-1 backward → +1 forward)

Sample points:
```
    Y ^ (+Z forward)
      |
      |   [Walk_F]    [Run_F]
      |       ●────────●
      |       │        │
      |   ●───┼────┼───●  ← blend zone
      |   │   │    │   │
      |  [Walk_L] [Walk_R]  ← X axis
      |   -1           +1
      |
```

**Implementation:**
```
BlendSpace2D: "locomotion"
  - Parameter: "direction_x", min=-1, max=1
  - Parameter: "direction_z", min=-1, max=1
  - Animations:
      (-1, 1)   : StrafeLeft (walk)
      (+1, 1)   : StrafeRight (walk)
      (0, 1)    : WalkForward
      (0, -1)   : WalkBackward
      (0, 0)    : Idle
      (±1, ±1)  : DiagonalWalk (45°)
  - Automatically blends between nearest samples
```

### 3.2 CombatMovement BlendSpace1D

Parameter: `combat_speed` (0-idle → 1-walk → 2-run)
- 0.0: CombatIdle
- 0.5: CombatWalk
- 1.0: CombatRun
- Blend: interpolation blends combat-ready stances smoothly

### 3.3 Dodge BlendSpace1D

`dodge_direction` enum → blends to correct dodge animation:
```
0 (forward) → dodge_forward.anim
1 (left)    → dodge_left.anim
2 (right)   → dodge_right.anim
3 (back)    → dodge_back.anim
```
(Since dodge is discrete, this is technically a selection not blend. Use AnimationNodeRandomPlayer or state machine transitions instead. BlendSpace not needed for dodge.)

**Correction:** Dodge should use **state machine transitions**, not blend space. Remove from requirements.

---

## 4. File Changes

### 4.1 Animation Resource Files

`assets/animations/PlayerAnimations.tres` — MODIFY (existing file):
```
[gd_resource type="AnimationTree" format=3 uid="..."]

animation_root = "parameters/Idle/blend_position"
anim_player = NodePath("../AnimationPlayer")
```

**Add new blend spaces:**
```tres
[resource]
blend_space_2d = "parameters/locomotion"
blend_space_1d = "parameters/combat_speed"

[sub_resource type="BlendSpace2DResource" id="BlendSpace2D_locomotion"]
...
[sub_resource type="BlendSpace1DResource" id="BlendSpace1D_combat_speed"]
...
```

### 4.2 Code Changes

`src/client/src/combat/fsm/AnimationStateMachine.cs` — ADD blend parameter updates:
```csharp
public override void _Process(double delta)
{
    if (Player.IsInCombat)
    {
        SetParameter("combat_speed", CalculateCombatSpeed());
    }
    else
    {
        SetParameter("movement_speed", Velocity.Length());
        SetParameter("direction_x", InputDirection.X);
        SetParameter("direction_z", InputDirection.Z);
        SetParameter("turn_angle", CalculateTurnAngle());
    }
}
```

### 4.3 New Test Files

`tests/client/TestAnimationBlendSpaces.cs`:
- Validate blend parameters clamp correctly
- Verify blend space sampling returns correct animation
- Verify no blending artifacts at boundaries

---

## 5. Implementation Steps

**Step 1 — Update PlayerAnimations.tres (Godot editor):**
1. Open `Player.tscn` in Godot editor
2. Select `AnimationTree` node
3. In `AnimationNodeStateMachine`, add two children:
   - `BlendSpace2D` named "locomotion"
   - `BlendSpace1D` named "combat_movement"
4. Populate blend space points with animation resources (walk, run, idle, strafe)
5. Connect parameters: `parameters/locomotion/blend_position` to x/y parameters

**Step 2 — Animation Parameter Updates (C#):**
Modify `AnimationStateMachine.cs` to update blend parameters in `_Process()`:
```csharp
// Existing state machine sets discrete animation
// ADD continuous blend parameter updates alongside state changes
AnimationTree.Set("parameters/movement_speed", currentSpeed);
AnimationTree.Set("parameters/direction_x", inputDir.X);
...
```

**Step 3 — Test & Tune:**
- Play in editor, verify smooth blending between walk/run based on joystick pressure
- Verify diagonal strafing interpolates between strafe-left and forward
- Verify no foot sliding (animation speed properly scaled)

**Step 4 — Documentation:**
- `docs/animation-blend-spaces.md` — how blend spaces are configured
- `docs/parameters.md` — list all AnimationTree parameters (new canonical list)

---

## 6. Acceptance Criteria

✅ **Functional Completeness**
- Locomotion blend space blends smoothly between idle/walk/run/strafe
- Combat movement blend space blends between idle/walk/run while in combat
- No visible snapping when changing direction or speed
- Animation speed matches velocity (no foot sliding at any blend point)

✅ **Technical Quality**
- AnimationTree.tres parses without errors
- Blend space parameters clamped to valid ranges (no NaNs)
- Transition times set to 0 (continuous blend) — no crossfade needed
- Memory: +2KB for blend space resources (negligible)

✅ **Performance**
- Animation update cost: <0.1ms per player (profiled)
- No GC pressure in C# (no allocations in _Process)

✅ **Integration**
- Works with Foot IK (PRD-011) — no bone conflicts
- Works with Procedural Leaning (PRD-015) — additive on spine, independent
- All 2129 existing tests pass (this is client-only change)

---

## 7. File Inventory

**Modified:**
- `src/client/Player.tscn` — AnimationTree node updated
- `src/client/PlayerAnimations.tres` — blend spaces added
- `src/client/src/combat/fsm/AnimationStateMachine.cs` — parameter updates

**Created:**
- `tests/client/TestAnimationBlendSpaces.cs` — unit test
- `docs/animation-blend-spaces.md` — documentation

**No server changes.**

---

## 8. Validation Checklist

Manual QA:
- [ ] Forward joystick: Idle → Walk → Run blends smoothly
- [ ] Sideways: Strafe left blends from walk with 0.5 forward
- [ ] Backwards: Walk backward blends from idle
- [ ] Diagonal: Forward-left blends idl→walk→strafe-left (interpolation works)
- [ ] Combat: enter combat mode → automatic switch to combat blend space
- [ ] Dodge: dodge animation overrides blend space (state machine takes precedence)
- [ ] Remote players: blend spaces visible on other players (no popping)

Automated test:
- [ ] `TestBlendSpaceSampling()` queries `AnimationTree.Get("parameters/locomotion/blend_position")` matches expected point
- [ ] `TestBlendParameterClamping()` clamps out-of-range values to 0-1

---

## 9. Related PRDs

- **PRD-008** (Combat FSM) — state machine provides combat_mode flag
- **PRD-011** (Foot IK) — independent; both system can run concurrently
- **PRD-015** (Procedural Leaning) — additive on spine bone vs animation base pose

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to ANIMATION_AGENT; this is pure client-side work (parallel to server-side PRDs)
