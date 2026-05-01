# PRD-015: Procedural Leaning System — Velocity-Based Character Tilt

**Version:** 1.0
**Status:** 🔄 In Progress — Implementation created
**Owner:** ANIMATION_AGENT
**Priority:** HIGH (P1 — Visual Coherence Requirement per COMPATIBILITY_ANALYSIS.md)
**Dependencies:** PRD-008 (CombatStateMachine FSM — for movement state integration)
**Also referenced in:** PROJECT_STATUS.md "Visual Polish" section (2026-04-25+26 mentions "procedural leaning")

---

## Implementation Status (2026-05-01)

### ✅ Completed
- [x] Created: `ProceduralLeaning.cs` - Velocity-based character tilt
- [x] Velocity-based lean angle (forward/backward)
- [x] Turn banking (lateral acceleration)
- [x] Smooth interpolation
- [x] State-aware (disabled during combat states)

### 📋 Pending Integration
- [ ] Add to Player.tscn as child node
- [ ] Configure export properties

---

## 1. Overview

### 1.1 Purpose
Implement **procedural leaning** (also called "banking" or "tilting") where the character's upper body (torso/spine) tilts sideways during sharp directional changes, based on current velocity. This adds weight and realism to movement in third-person view.

**Industry standard:** Monster Hunter World, Elden Ring, Sekiro — characters lean into turns.

### 1.2 Scope
Client-side animation modification in `src/client/`:
- New: `ProceduralLeaning.cs` — leaning calculation component
- Attachment to `Player.tscn` and `RemotePlayer.tscn`
- Integration with `AnimationTree` via `AnimationNodeStateMachine` or `AnimationNodeBlendTree`
- Tuning parameters exposed in inspector
- Server has **NO knowledge** of leaning (purely visual)

**EXCLUDED:**
- Server-side validation of leaning (not needed)
- Network replication (leaning is client-side only)

---

## 2. Requirements

### 2.1 Functional Requirements
ID     | Requirement                        | Priority | Notes
-------|------------------------------------|----------|------
LEAN-001 | Velocity-based tilt calculation     | P0       | Torso rotates Z-axis based on lateral velocity
LEAN-002 | Smooth interpolation (damping)      | P0       | No snapping, 0.15s settle time
LEAN-003 | Direction-aware leaning             | P0       | Leans INTO turn (not away)
LEAN-004 | Tilt magnitude cap                  | P1       | Max 15° lean at full speed
LEAN-005 | G state reset                       | P1       | Leaning clears during attacks/dodges
LEAN-006 | Remote player replication           | P1       | Leaning visible on others (client-predicted)
LEAN-007 | Configuration tuning               | P1       | Deadzone, max angle, damping exposed

### 2.2 Non-Functional
- Update cost: <0.1ms per player (simple vector math)
- No impact on server bandwidth (local visual only)
- Works in both first-person and third-person views
- Compatible with Foot IK (PRD-011)

---

## 3. Technical Design

### 3.1 Leaning Math
Input: `Velocity` component (Vector3) from ECS
```
lateralVelocity = new Vector3(velocity.X, 0, velocity.Z).Length()
turnAngle = Mathf.Atan2(lateralVelocity * turnFactor, 1.0f)

Lean direction: sign of cross product (desired direction × current forward)
Lean magnitude: clamp(lateralVelocity / maxSpeed * maxLeanAngle, 0, maxLeanAngle)

Smooth damp: currentLean = Mathf.SmoothDamp(currentLean, targetLean, ref velocity, damping)
```

### 3.2 Integration Points

**Player.tscn scene tree:**
```
Player (CharacterBody3D)
├── PredictedPlayer.cs (input + movement)
├── AnimationTree (existing)
│   └── AnimationNodeStateMachine
│       ├── Idle
│       ├── Moving
│       ├── Attacking
│       └── ...
├── ProceduralLeaning.cs (NEW)
│   └── Target: Spine bone (or Torso) via Skeleton3D
├── FootIKController.cs (PRD-011)
└── CombatStateMachine (PRD-008 — state machine node)
```

**RemotePlayer.tscn:**
- Same `ProceduralLeaning.cs` attached
- Leaning computed client-side for remote players (prediction based on last known velocity)
- No network overhead

### 3.3 Animation Tree Integration

Option A: **Direct bone transform** via `Skeleton3D` API
- `Skeleton3D.SetBonePose(boneId, Transform)` in `_Process()`
- Pros: Simple, immediate; Cons: Overwrites animation

Option B: **AnimationTree blend space** (preferred)
- Create "Leaning" parameter in AnimationTree
- Add AnimationNodeBlendSpace1D for "tilt" track
- ProceduralLeaning.cs sets `AnimationTree.Set("parameters/lean", value)`
- Animation author creates lean-left/lean-right poses
- Pros: Animation-driven, smoother; Cons: Requires authoring poses

**Recommendation:** Option A (direct bone) for MVP — simpler, no asset dependencies.

### 3.4 File Deliverables

| File | Type | Description |
|------|------|-------------|
| `src/client/src/movement/ProceduralLeaning.cs` | Script | Leaning calculation component |
| `src/client/scenes/Player.tscn` | Scene | Attach ProceduralLeaning node |
| `src/client/scenes/RemotePlayer.tscn` | Scene | Same attachment |
| `tests/client/TestProceduralLeaning.cs` | Test | Validate leaning angle boundaries |
| `docs/procedural-leaning.md` | Docs | Parameters, tuning, math explanation |

---

## 4. Tuning Parameters

Expose in inspector via `[Export]`:

| Parameter | Type | Default | Min | Max | Description |
|-----------|------|---------|-----|-----|-------------|
| `MaxLeanAngle` | float | 15.0° | 0 | 30 | Maximum tilt at full lateral speed |
| `LeanDeadzone` | float | 0.5 m/s | 0 | 2 | Velocity threshold before leaning starts |
| `Damping` | float | 0.15 s | 0.05 | 0.5 | Smooth damp time (lower = snappier) |
| `TurnFactor` | float | 0.8 | 0 | 2 | How aggressively turn intensity affects lean |
| `BoneName` | String | "Spine" | — | — | Which skeleton bone to rotate |

**Parameter discovery:**
```csharp
// Through trial testing:
- MaxLeanAngle: 12° feels subtle, 18° feels "gamey" — choose 15°
- Damping: 0.1 → too twitchy; 0.2 → too sluggish; 0.15 → balanced
- LeanDeadzone: 0.5 m/s ensures minor strafing doesn't cause tilt
```

---

## 5. State Machine Integration

CombatStateMachine (PRD-008) states:
- **Moving**: Leaning ENABLED
- **Idle**: Leaning DISABLED (reset to 0)
- **Attacking / Dodging / Hit / Dead**: Leaning DISABLED
  (Attack animations include body motion; shouldn't double-bend)

Implementation:
```csharp
public override void _Process(double delta)
{
    if (Player.CombatStateMachine.CurrentState is MovingState)
    {
        ApplyLeaning((float)delta);
    }
    else
    {
        // Reset leaning smoothly when not moving
        currentLean = Mathf.SmoothDamp(currentLean, 0, ref velocity, damping);
        ApplyToBone(currentLean);
    }
}
```

---

## 6. Validation & Testing

### 6.1 Unit Tests
`tests/client/TestProceduralLeaning.cs`:
- [ ] `LeaningZeroWhenStationary()`: velocity=0 → angle=0°
- [ ] `LeaningIncreasesWithSpeed()`: 1 m/s → 5°, 5 m/s → 12°, 10 m/s → 15° cap
- [ ] `LeaningDirectionMatchesTurn()`: strafe right → positive Z-rotation
- [ ] `DampingWorks()`: sudden stop → lean decays over ~0.15s
- [ ] `DisabledInCombatStates()`: attacking → lean forced to 0

### 6.2 Integration Tests
`tools/demo/client_instrumentation_validator.py` update:
- Capture 10-s sample of player movement
- Verify `ProceduralLeaning.leanAngle` stays within [-15°, +15°]
- No spikes or jitter (>1°/frame change)
- Leaning visible on RemotePlayer instances (not just local)

### 6.3 Manual QA
- [ ] Circle-strafe left → character tilts left
- [ ] Circle-strafe right → character tilts right
- [ ] Stop abruptly → lean smoothly decays to 0
- [ ] Attack while moving → leaning disabled during attack
- [ ] Remote player leaning visible across network

---

## 7. Performance Budget

| Metric | Target | Budget |
|--------|--------|--------|
| Lean update cost | <0.05ms | Per-player per-frame |
| Network overhead | 0 B | No state sent |
| Memory | +64 bytes/player | Component struct |

---

## 8. Acceptance Criteria

✅ **Core Functionality**
- Player tilts when moving laterally (strafe)
- Maximum 15° lean at full speed
- Leaning disabled during non-moving/combat states
- Smooth interpolation (0.15s damping) — no snapping

✅ **Technical Quality**
- Zero test regressions (baseline 2129 tests pass)
- Performance: <0.1ms update cost
- Works with Foot IK (no bone conflict; Spine vs Feet)
- Works with AnimationStateMachine (state transitions reset lean)

✅ **Code Standards**
- ProceduralLeaning.cs follows Hermes conventions (regions, XML comments)
- Unit tests written and passing
- No warnings in build (`dotnet build`)
- Inspector parameters fully exposed

✅ **Documentation**
- `docs/procedural-leaning.md` explains math and tuning
- `AGENTS.md` Recent Commits updated with PR reference
- `PHASE_8_FINAL_SIGNOFF.md` (if applicable) updated

---

## 9. References

- **COMPATIBILITY_ANALYSIS.md**: Identifies "Procedural Leaning — None" as gap
- **PROJECT_STATUS.md** (2026-04-29): Notes "procedural leaning" in Visual Polished items
- **AGENTS.md**: Not mentioned in critical rules (acceptable omission)
- **Research/ThirdPersonCombatStandardsResearch/**: Industry standard requirement

---

## 10. Risk Notes

⚠️ **Risks:**
1. Bone name mismatch (Spine vs Spine001 vs torso) — need to verify rig
2. AnimationTree may conflict with direct bone transform — test thoroughly
3. Remote player leaning prediction might feel "off" if client extrapolates incorrectly

🚫 **Mitigations:**
1. Add bone name as configurable parameter (BoneName export)
2. Use Skeleton3D `SetBonePose` after animation (post-physics) to override
3. Use local velocity client-predicted with server reconciliation buffer

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to ANIMATION_AGENT — this builds on Foot IK but is separate concern
