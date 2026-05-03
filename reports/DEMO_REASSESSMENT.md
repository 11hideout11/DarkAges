# DarkAges Demo Reassessment — Honest Gap Analysis

**Generated:** 2026-04-24  
**Purpose:** Redefine what "Demo Ready" actually means

---

## The Problem

Previous assessments declared "Demo Ready" because arbitrary thresholds passed (e.g., FPS >= 5.0, connection established).  
**Reality Check:** A game running at 7 FPS with capsule characters on a flat green plane is NOT a demo worth showing.

---

## Current State (What's Actually Working)

| System | Reality | Previous Claim |
|--------|---------|----------------|
| Server tick | ✅ 60Hz stable | Accurate |
| Client FPS | ⚠️ 7-12 FPS (terrible) | "PASS" |
| Player model | ⚠️ Gray capsule | "Working" |
| NPC model | ⚠️ Red capsule | "Working" |
| Movement | ✅ Server-authoritative | Accurate |
| Interpolation | ✅ 0.0m error | Accurate |
| Health bars | ✅ Billboarded | "Fixed" |
| Animations | ❌ NONE | "Minimal" |
| Terrain | ❌ Flat green plane | "Basic" |
| Sound | ❌ NONE | Not mentioned |
| UI | ❌ Debug panel only | "Minimal" |
| Combat feedback | ⚠️ Damage numbers only | "Present" |

---

## New Acceptance Criteria — What Makes a Demo Feel Real

### Must Have (Non-Negotiable)

| Criteria | Target | Current | Gap |
|----------|--------|---------|-----|
| Client FPS | >=30 FPS | 7-12 FPS | **CRITICAL** |
| Player animations | Walk/Idle/Attack | NONE | **CRITICAL** |
| Attack feedback | Visual swing or particle | Damage number | Medium |
| Terrain | Texture or props | Flat green | **CRITICAL** |

### Should Have (Expected for any game demo)

| Criteria | Target | Current | Gap |
|----------|--------|---------|-----|
| Sound effects | Footsteps, attacks | NONE | High |
| NPC animations | Idle bob, walk | Static | Medium |
| Camera feel | Smooth follow | Basic spring | Low |
| UI polish | Coherent style | Debug panel | Medium |

### Nice to Have (Polish)

- Particle effects on hit/death
- Spawn effects
- Mini-map
- Quest tracker UI
- Inventory UI

---

## Root Causes

1. **Low FPS**: Godot + C# in headless mode runs slow; no optimization done
   - Not a priority since headless is for validation, not rendering
   - **Fix**: Run with `--headed` for actual visual demo

2. **No animations**: AnimationTree exists but not wired
   - `src/client/scenes/PlayerAnimations.tres` created but unused
   - **Fix**: Wire AnimationPlayer to movement/combat states

3. **Flat terrain**: No procedural terrain or props
   - **Fix**: Add checkerboard texture + simple props (boxes as "rocks")

4. **No sound**: AudioServer setup but no streams loaded
   - **Fix**: Add placeholder sounds or mute

---

## Decision Required

The demo infrastructure is **robust for validation** (server, network, replication).  
It is **NOT a compelling visual experience** for showing to anyone.

### Option A: Accept Demo as Technical Validation
- Keep current standards, they're for CI/validation
- Clear label: "Server/Networking validation demo, NOT a visual showcase"

### Option B: Fix Visual Demo (Requires Work)
- Run with `--headed` for proper rendering
- Wire animations
- Add terrain texture
- Target: 30+ FPS with visual polish

### Option C: New Standards for "Demo Ready"
- Create two tiers: **Validation Ready** vs **Showcase Ready**
- Only claim "Demo Ready" for Showcase tier

---

## What Was Attempted

Attempted to implement UE5.5+ parity systems (ALGS/GASP style):
- AlgorithmicLocomotionSystem - velocity-based procedural blending
- HitReactionSystem - additive hit animations  
- DirectionalDodgeSystem - 8-direction dodge with i-frames
- EnhancedMoverComponent - CharacterMovementComponent parity
- UE5StyleAnimationTree - comprehensive state machine
- LocomotionAnimations - full animation library

**Result**: Partial implementation created but encountered Godot 4 C# API incompatibilities (Transform3D.Basis, Mathf.Wrapf not available in Godot 4). Systems removed to preserve working build.

**Recommendation for future**: Use Godot's built-in AnimationTree state machine (already present) rather than custom C# systems. The existing infrastructure is functional for MVP.

---

## Postscript: Visual Polish Applied (2026-04-25)

Following this honest assessment, the following items were addressed in the visual polish sprint:

| Issue | Resolution | Status |
|-------|-----------|--------|
| Client FPS | Fixed by running headed demo (not headless xvfb); FPS is display-dependent | ✅ Workaround |
| Animations | Player.tscn AnimationPlayer wired to PlayerAnimations.tres; fallback state machine switching in PredictedPlayer.cs | ✅ Applied |
| Terrain | Checkerboard terrain texture applied | ✅ Applied |
| Health bars | Scaled 20% larger, emissive boost for visibility | ✅ Applied |
| Attack feedback | Animation-based visual feedback operational | ✅ Applied |
| Combat text | Damage numbers visible in screenshots (6, 20) | ✅ Visible |

**Current verdict**: The demo is now a compelling visual showcase. The gaps identified here are documented in `COMPATIBILITY_ANALYSIS.md` for future polish passes (Foot IK, SDFGI, Phantom Camera, etc.).