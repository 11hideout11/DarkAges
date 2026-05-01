# PRD-014: Phantom Camera 3rd-Person Follow & Lock-On System

**Version:** 1.0
**Status:** 🔄 In Progress — Implementation created
**Owner:** CAMERA_AGENT
**Priority:** HIGH (P1 — Visual Coherence Requirement)
**Dependencies:** PRD-008 (CombatStateMachine FSM — for lock-on state integration)

---

## Implementation Status (2026-05-01)

### ✅ Completed
- [x] Created: `PhantomCamera.cs` - Lock-on targeting camera
- [x] Smooth third-person follow (inherits from existing CameraController)
- [x] Lock-on targeting with automatic focus
- [x] Target switching (next/prev)
- [x] Smooth transitions between free-look and lock-on modes

### 📋 Pending Integration
- [ ] Replace CameraController script in Player.tscn with PhantomCamera
- [ ] Configure input map actions (lock_on, lock_on_next, lock_on_prev)
- [ ] Add enemies to "enemies" group for detection

---

## 1. Overview

### 1.1 Purpose
Replace the current basic `SpringArm3D` camera setup with a **Phantom Camera (PCam)** implementation that provides:
- Professional 3rd-person follow with deadzone/soft zone
- Collision avoidance (raycast push-in)
- Height/rotation smoothing
- Lock-on targeting integration with auto-rotation
- Cinematic feel with configurable parameters

### 1.2 Scope
All camera-related client code in `src/client/`:
- `CameraController.cs` — existing basic camera (replace)
- `Player.tscn` — camera node attachment
- `TargetLockSystem.cs` — lock-on integration (already exists but needs camera wiring)
- New: `PhantomCameraController.cs` — PCam wrapper
- New: `CameraConfig.res` — tuning parameters

**EXCLUDED:** Cinematic cutscenes (post-MVP polish)

---

## 2. Requirements

### 2.1 Functional Requirements
ID    | Requirement                             | Priority | Notes
------|-----------------------------------------|----------|--------
CAM-001 | Replace SpringArm3D with PhantomCamera | P0       | Use `PhantomCamera3D` node from Godot asset
CAM-002 | Deadzone configuration                  | P0       | Player stays in center deadzone until near edge
CAM-003 | Soft zone follow                        | P0       | Smoothly interpolate to player position
CAM-004 | Collision avoidance                     | P1       | Raycast forward, push camera in if obstructed
CAM-005 | Height smoothing                        | P1       | Smooth vertical follow, not jittery
CAM-006 | Lock-on auto-rotation                   | P0       | Camera orbits locked target smoothly
CAM-007 | Input sensitivity tuning                | P1       | Mouse sensitivity affects rotation speed
CAM-008 | Platform-specific deadzone profiles    | P2       | PC vs controller profiles

### 2.2 Non-Functional Requirements
- Update cost: <0.5ms per frame (camera update)
- 60fps locked — no frame drops from camera
- Works in demo mode and multiplayer (all clients see same camera behavior)
- No visible snapping during movement state transitions

---

## 3. Current Gap

**Current State:**
```
src/client/CameraController.cs exists but is BASIC:
  - Uses SpringArm3D
  - Simple collision raycast (length 3.0)
  - No deadzone/softzone configuration
  - No smoothing parameters
  - No lock-on integration

Player.tscn structure:
  - Player (CharacterBody3D)
    - CameraPivot (Node3D)
      - SpringArm3D (length 4.0)
        - Camera3D
  - No PhantomCamera3D node present

TargetLockSystem.cs exists but:
  - Highlights target with glow
  - Does NOT reorient camera to follow target
```

**Gap:** Missing professional-grade camera system aligned with UE5 Phantom Camera standard (COMPATIBILITY_ANALYSIS.md Priority 1).

---

## 4. Technical Design

### 4.1 PhantomCamera3D Node
Godot 4.2 has `PhantomCamera3D` as a built-in node (from Phantom Camera 4.x plugin). This is a specialized camera controller providing:
- Configurable follow envelope (deadzone/softzone)
- Collision handling with raycast & push-in
- Smooth damping (position/rotation)
- Look-at targets for lock-on

**Integration:**
```gdscript
# PhantomCameraController.cs (C#)
public partial class PhantomCameraController : Node
{
    [Export] public PhantomCamera3D PhantomCam { get; set; }
    [Export] public Player Player { get; set; }
    [Export] public float DeadzoneRadius { get; set; } = 0.3f;
    [Export] public float SoftZoneRadius { get; set; } = 0.6f;
    [Export] public float FollowSpeed { get; set; } = 8.0f;
    [Export] public float RotationSpeed { get; set; } = 12.0f;
    
    // State
    private bool _isLockedOn = false;
    private EntityID _lockedTarget = EntityID.Invalid;
    
    public override void _Process(double delta)
    {
        if (_isLockedOn && IsInstanceValid(Player.TargetLockSystem.LockedTarget))
        {
            PhantomCam.EnableOrbitMode(Player.TargetLockSystem.LockedTarget.GlobalPosition);
        }
        else
        {
            UpdateFollowMode((float)delta);
        }
    }
}
```

### 4.2 Deadzone Configuration
Deadzone: Inner radius where camera does NOT move (player centered)
Softzone: Outer radius where camera interpolates to keep player visible

```
        ┌────────────────┐
        │   Soft Zone    │
        │    ╱══╲        │
        │   ╱  ╲ ╲       │
        │  ╱  Dead ╲     │  Camera orbits player
        │ ╱  Zone  ╲╲    │  Camera does NOT move
        │ ╲        ╲╲   │  when player inside deadzone
        └───────────────┘
```

### 4.3 Lock-On Integration
When lock-on active (`TargetLockSystem.IsLockedOn`):
1. Disable player-follow mode
2. Enable `PhantomCam.EnableOrbitMode(targetPosition)`
3. PhantomCamera automatically orbits target
4. Camera stays at fixed distance & height, rotates around target
5. On unlock → resume follow mode with deadzone

### 4.4 File Changes
| File | Action | Lines |
|------|--------|-------|
| `src/client/scenes/Player.tscn` | Replace SpringArm3D with PhantomCamera3D | — |
| `src/client/CameraController.cs` | Delete & replace with PhantomCameraController.cs | — |
| `src/client/src/camera/PhantomCameraController.cs` | New file | ~120 |
| `src/client/scenes/CameraConfig.res` | New resource (deadzone/softzone values) | — |
| `tests/client/TestPhantomCamera.cs` | New test | ~80 |

---

## 5. Visual Reference (Target Configuration)

Target camera behavior (final fantasy / monster hunter style):

| Parameter | Value | Reason |
|-----------|-------|--------|
| Distance | 4.0 units | Clear view of character |
| Deadzone radius | 0.3 (Y-axis 0.15) | Keep target centered |
| Softzone radius | 0.6 (Y-axis 0.3) | Smooth follow |
| Follow speed | 8.0 | Responsive but not jittery |
| Rotation speed | 12.0 | Quick lock-on rotation |
| Collision ray length | 3.5 | Pushes camera in when blocked |
| Collision push-in | 0.5 | Brings camera around obstacles |

---

## 6. Acceptance Criteria

✅ **Functional Completeness**
- PhantomCamera3D node replaces SpringArm3D in Player.tscn
- Deadzone + soft zone visible in editor (configurable via CameraConfig.res)
- Camera collision avoids walls/objects (raycast + push-in)
- Lock-on integration: camera orbits locked target smoothly
- Camera returns to follow mode when lock broken

✅ **Technical Completeness**
- Zero camera glitches at 60Hz for 10+ minute demo
- No frame drops (profiling shows <0.5ms camera update cost)
- Works in multiplayer (all clients independent)
- Configurable via `CameraConfig.res` (no hard-coded values)

✅ **Code Quality**
- `PhantomCameraController.cs` <200 lines
- Clear state machine: Follow mode vs Lock-on orbit mode
- Unit test: `TestPhantomCamera.cs` validates deadzone math
- Zero warnings in Godot editor (no invalid node paths)

✅ **Documentation**
- `docs/camera-setup.md` explains parameters
- `docs/locking-camera-flow.md` diagrams orbit behavior
- README updated with camera controls section

---

## 7. Integration with Lock-On System

**Key integration point:**

`TargetLockSystem.cs` (existing — server-side confirmed locks):
- Produces `IsLockedOn`, `LockedTarget` state

`PhantomCameraController.cs` (new — client-side camera):
- Reads `TargetLockSystem.IsLockedOn` from player
- When locked: calls `PhantomCam.SetLookAtTarget(target)`
- When unlocked: reverts to `PhantomCam.FollowPlayer(player)`

**No RPC needed** — lock-on state already replicated to client via `RemotePlayer` sync.

---

## 8. Implementation Roadmap

**Day 1 (4 hours):**
1. Research PhantomCamera3D node API (Godot 4.2)
2. Create `PhantomCameraController.cs` skeleton
3. Replace SpringArm3D with PhantomCamera3D in Player.tscn
4. Tune deadzone/softzone parameters

**Day 2 (4 hours):**
1. Implement lock-on orbit mode integration
2. Test camera collision avoidance in constrained spaces
3. Write `TestPhantomCamera.cs` unit tests
4. Profile camera update cost (<0.5ms requirement)

**Day 3 (2 hours):**
1. Polish: controller sensitivity profiles
2. Fix edge cases (target dies mid-orbit, player out of range)
3. Documentation (`docs/camera-setup.md`)
4. PR + two-agent review

**Total:** ~10 agent-hours (parallelizable with other agents)

---

## 9. Related PRDs

- **PRD-008** (Combat FSM) — provides state machine architecture for integration
- **NEXT_AGENT_PROMPT.md** — Lock-on targeting listed as COMPLETED, but camera integration was partial
- **COMPATIBILITY_ANALYSIS.md** — Identifies Phantom Camera as Priority 1 gap

---

## 10. Validation & Testing

### 10.1 Unit Tests (C#)
`tests/client/TestPhantomCamera.cs`:
- [ ] Deadzone boundary: player at (0.29, 0, 0) → camera stationary
- [ ] Softzone boundary: player at (0.61, 0, 0) → camera moves
- [ ] Lock-on transition: lock → orbit mode engaged
- [ ] Lock-off transition: orbit → follow mode resumes
- [ ] Collision: wall at distance 2.0 → camera pushed in to 1.5

### 10.2 Integration Tests (Demo Mode)
`tools/demo/client_instrumentation_validator.py` update:
- [ ] Verify camera position stable during player rotation
- [ ] Verify camera follows player smoothly during movement
- [ ] Verify camera orbits locked target
- [ ] Verify zero camera jitter over 60s recording

### 10.3 Manual QA
`tests/manual/camera_qa_checklist.txt`:
- [ ] Walk into corner — camera doesn't clip through walls
- [ ] Attack while moving — camera stable
- [ ] Lock onto moving target — camera orbits smoothly
- [ ] Mouse look at extremes — softzone follows correctly

---

## 11. Performance Budget

| Metric | Target | Actual (after) |
|--------|--------|----------------|
| Camera update time | <0.5ms | TBD (must measure) |
| Memory overhead | +2KB (PhantomCamera node) | TBD |
| Network impact | None (client-only) | ✓ |

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to CAMERA_AGENT with branch `autonomous/20260501-phantom-camera`
