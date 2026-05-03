# DarkAges PRD Roadmap - Gap Analysis & Implementation Plan

**Generated:** 2026-05-01
**Status:** Implementation Planning

---

## Executive Summary

This document catalogs identified gaps in the DarkAges MMO and provides a prioritized roadmap for implementation. Based on analysis of CURRENT_STATUS.md, PHASE8_EXECUTION_PLAN.md, and source code, the following critical gaps have been identified.

### Project Status Overview

| Component | Status | Gap Severity |
|-----------|--------|-------------|
| Server Core | ✅ Operational | Low |
| Combat System | ✅ Operational | Medium (no GCD, Hitbox layers) |
| Client Animation | ⚠️ Partial | High (no visual assets) |
| Visual Assets | ❌ None | CRITICAL (MVP blocker) |
| Ability System | ⚠️ Stub | High |
| Camera | ⚠️ Basic | Medium |
| State Management | ⚠️ Inline flags | Medium |

---

## Gap Documentation Index

### Priority 1: MVP-Critical (Must Fix for Demo)

| PRD | Gap | Severity | Estimated Effort |
|----|-----|----------|------------------|
| `prd-visual-assets.md` | ZERO visual assets exist | CRITICAL | 2-4 weeks |
| `prd-global-cooldown.md` | No attack throttling | HIGH | 1 week |
| `prd-ability-system.md` | Ability stub needs completion | HIGH | 2 weeks |

### Priority 2: Gameplay Enhancement

| PRD | Gap | Severity | Estimated Effort |
|----|-----|----------|------------------|
| `prd-hitbox-system.md` | No server hitbox layers | MEDIUM | 1-2 weeks |
| `prd-foot-ik.md` | Feet clip through terrain | MEDIUM | 1 week |
| `prd-camera-system.md` | Basic camera lacking polish | MEDIUM | 1 week |

### Priority 3: Architectural Improvement

| PRD | Gap | Severity | Estimated Effort |
|----|-----|----------|------------------|
| `prd-state-charts.md` | Inline flags vs proper FSM | MEDIUM | 1 week |

### Priority 4: Social Systems (Post-MVP)

| PRD | Gap | Severity | Estimated Effort |
|----|-----|----------|------------------|
| `prd-party-system.md` | Party UI hidden, no functionality | MEDIUM | 1-2 weeks |
| `prd-guild-system.md` | No guild system | MEDIUM | 2 weeks |
| `prd-quest-system.md` | QuestTracker UI exists, no server | MEDIUM | 1-2 weeks |
| `prd-crafting-system.md` | Header exists, no implementation | MEDIUM | 1 week |
| `prd-trade-system.md` | Header exists, no implementation | MEDIUM | 1 week |

### Priority 5: Polish & Systems

| PRD | Gap | Severity | Estimated Effort |
|----|-----|----------|------------------|
| `prd-audio-system.md` | No audio system | LOW | 1 week |
| `prd-settings-ui.md` | No settings UI | LOW | 1 week |

---

## Detailed Gap Analysis

### Gap 1: Visual Assets (CRITICAL - MVP Blocker)

**Current State:**
- Server fully operational (2097 tests passing)
- Combat FSM, AnimationTree configured
- Zero 3D models, textures, shaders

**Impact:**
- Project NOT READY under new MVP criteria
- Demo shows nothing visually

**Recommendation:** `prd-visual-assets.md`

---

### Gap 2: Global Cooldown (HIGH - Gameplay)

**Current State:**
- Attacks can chain infinitely
- No timing constraint

**Impact:**
- Removes meaningful combat decisions
- No skill expression

**Files to Modify:**
- `src/server/include/combat/CombatSystem.hpp`
- `src/server/src/combat/CombatSystem.cpp`

**Recommendation:** `prd-global-cooldown.md`

---

### Gap 3: Hitbox System (MEDIUM - Combat)

**Current State:**
- Uses lag-compensated raycast validation
- No hitbox/hurtbox component layers

**Impact:**
- Limited combat depth
- Ambiguous hit detection

**Files to Modify:**
- `src/server/include/combat/Hitbox.hpp`
- `src/server/include/combat/LagCompensatedCombat.hpp`

**Recommendation:** `prd-hitbox-system.md`

---

### Gap 4: Foot IK (MEDIUM - Visual Polish)

**Current State:**
- No SkeletonIK3D implementation
- Feet clip through terrain

**Impact:**
- Breaks immersion
- Poor visual quality

**Files to Modify:**
- `src/client/src/Player.tscn`
- `src/client/src/Player.gd`

**Recommendation:** `prd-foot-ik.md`

---

### Gap 5: State Charts (MEDIUM - Architecture)

**Current State:**
- Uses inline boolean flags
- isAttacking, isDead, isHit stored directly

**Impact:**
- Ambiguous state transitions
- Difficult to debug

**Files to Modify:**
- `src/client/src/Player.tscn`
- New: `src/client/src/Player.statechart`

**Recommendation:** `prd-state-charts.md`

---

### Gap 6: Camera System (MEDIUM - Visual Polish)

**Current State:**
- Basic SpringArm3D with collision
- No dynamic distance, shake

**Impact:**
- Flat camera feel
- Missing impact feedback

**Files to Modify:**
- `src/client/src/CameraController.tscn`

**Recommendation:** `prd-camera-system.md`

---

### Gap 7: Ability System (HIGH - Gameplay)

**Current State:**
- `AbilitySystem` stub exists but incomplete
- No ability definitions, UI

**Impact:**
- Limited combat variety
- No progression system

**Files to Modify:**
- `src/server/include/combat/AbilitySystem.hpp`
- `src/client/src/ui/AbilityHotbar.tscn`

**Recommendation:** `prd-ability-system.md`

---

## Implementation Recommendation

### Phase 1: Visual Foundation (Weeks 1-2)
```
Week 1: Import basic player model + animations
Week 2: Basic enemy NPCs + demo zone environment
```
**Deliverable:** Playable visual demo

### Phase 2: Combat Enhancement (Weeks 3-4)
```
Week 3: Global cooldown system
Week 4: Ability system + UI
```
**Deliverable:** Meaningful combat gameplay

### Phase 3: Polish & Architecture (Weeks 5-6)
```
Week 5: Hitbox system + Foot IK
Week 6: State charts + Camera polish
```
**Deliverable:** Production-ready systems

---

## Dependencies

```
Visual Assets
    │
    ├──▶ Global Cooldown (needs combat)
    │
    ├─▶ Ability System (needs combat)
    │
    ├─▶ Hitbox System (needs combat)
    │
    ├─▶ Foot IK (needs visual)
    │
    └─▶ State Charts (needs visual)
         │
         └─▶ Camera System

Timeline:
Assets ─┬─► GCD ──► Ability ──► Hitbox ──► State Charts ──► Camera
        │
        └─► Foot IK
```

---

## Success Criteria

When all PRDs are complete:
- [ ] Visual demo with characters and enemies
- [ ] Combat has timing constraints (GCD)
- [ ] Abilities functional with UI
- [ ] Hit detection via proper hitbox layers
- [ ] No foot clipping via IK
- [ ] Proper FSM instead of inline flags
- [ ] Dynamic camera feel

---

## Files Created

| PRD Document | Purpose |
|-------------|---------|
| `prd-visual-assets.md` | Visual asset pipeline |
| `prd-global-cooldown.md` | Attack throttling |
| `prd-hitbox-system.md` | Hitbox/hurtbox components |
| `prd-foot-ik.md` | Terrain foot alignment |
| `prd-state-charts.md` | FSM migration |
| `prd-ability-system.md` | Ability completion |
| `prd-camera-system.md` | Camera enhancement |
| `prd-roadmap.md` | This summary |

---

## Next Steps

1. Review each PRD for accuracy
2. Prioritize based on MVP requirements
3. Begin implementation with `prd-visual-assets.md`
4. Iterate on remaining PRDs

---

*Generated by OpenHands agent - 2026-05-01*