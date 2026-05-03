# DarkAges - Gap Analysis & Implementation Master Plan

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Analysis Complete

---

## Executive Summary

This document consolidates all identified gaps, problems, and areas for improvement in the DarkAges MMO project. Ten new PRD documents have been created to address these issues.

### Project Baseline

| Component | LOC | Status |
|-----------|-----|--------|
| Server | ~32K | ✅ Operational |
| Client | ~9K | ✅ Functional |
| Tests | 2129 | ✅ Passing |
| Combat FSM | - | ⚠️ Created, not integrated |
| GNS Networking | - | ⚠️ Compile fixed, runtime pending |
| Inventory | - | ⚠️ Placeholder only |

---

## Gap Analysis

### Critical Gaps (Blocking)

| Priority | Gap | PRD | Status |
|----------|-----|-----|--------|
| 1 | GNS Runtime Integration | PRD-017 | Proposed |
| 2 | Node-Based FSM Integration | PRD-018 | Proposed |
| 3 | Zone Objectives Integration | PRD-019 | Proposed |

### High Priority Gaps (Functional)

| Priority | Gap | PRD | Status |
|----------|-----|-----|--------|
| 4 | Inventory/Equipment System | PRD-021 | Proposed |
| 5 | Abilities/Talents System | PRD-022 | Proposed |
| 6 | SDFGI/SSAO Lighting | PRD-020 | Proposed |

### Medium Priority Gaps (Content)

| Priority | Gap | PRD | Status |
|----------|-----|-----|--------|
| 7 | Guild System | PRD-023 | Proposed |
| 8 | Party System | PRD-024 | Proposed |
| 9 | Quest System | PRD-025 | Proposed |
| 10 | Trade System | PRD-026 | Proposed |

---

## Implemented PRDs (Existing)

| PRD | Name | Status |
|-----|------|--------|
| PRD-008 | Node-Based Combat FSM Template | ✅ Created |
| PRD-009 | Demo Zones with Objectives | ✅ Created |
| PRD-010 | Hitbox/Hurtbox Validation | ✅ Complete |
| PRD-012 | GNS Compile Fix | ✅ Merged |
| PRD-013 | Phase 1-5 Verification | ✅ Complete |
| PRD-014 | Phantom Camera | ✅ Created |
| PRD-015 | Procedural Leaning | ✅ Created |

---

## Implementation Roadmap

### Phase A: Critical Infrastructure (Weeks 1-3)

```
Week 1-2: PRD-017 GNS Runtime
├── GNSConnectionManager tick integration
├── Connection state management
└── Performance validation

Week 3: PRD-018 FSM Integration  
├── Wire FSM to Player.tscn
├── Wire FSM to RemotePlayer.tscn
└── Test combat flow
```

### Phase B: Gameplay Foundation (Weeks 4-8)

```
Week 4-5: PRD-019 Zone Objectives
├── Integrate with tick loop
├── Track progress
└── Reward distribution

Week 6-7: PRD-021 Inventory
├── Item database
├── Equipment slots
└── Inventory UI

Week 8: PRD-022 Abilities Framework
├── Ability types
├── Cooldowns/mana
└── Basic abilities
```

### Phase C: Polish (Weeks 9-12)

```
Week 9: PRD-020 Lighting
├── SDFGI configuration
├── SSAO configuration
└── Performance tuning

Week 10-12: Social Systems
├── PRD-023 Guild System
├── PRD-024 Party System
└── PRD-025 Quest System
```

### Phase D: Economy (Weeks 13-16)

```
Week 13-14: PRD-026 Trade System
├── Trade flow
├── Player economy

Week 15-16: PRD-022 Talents
├── Talent trees
└── Unlock system
```

---

## Resource Estimates

| PRD | Difficulty | Time | Priority |
|-----|------------|------|----------|
| PRD-017 | High | 2 weeks | Critical |
| PRD-018 | Medium | 2 weeks | Critical |
| PRD-019 | Medium | 2 weeks | Critical |
| PRD-020 | Low | 1 week | High |
| PRD-021 | Medium | 3 weeks | High |
| PRD-022 | Medium | 4 weeks | High |
| PRD-023 | Medium | 2 weeks | Medium |
| PRD-024 | Low | 1 week | Medium |
| PRD-025 | Medium | 2 weeks | Medium |
| PRD-026 | Low | 1 week | Medium |

**Total estimated time:** 20 weeks

---

## Success Metrics

### Project Metrics
- All 10 new PRDs implemented
- Test suite maintains 100% pass rate
- 60fps maintained
- All systems server-authoritative

### Per-PRD Checkpoints
- **PRD-017:** 10K connections validated
- **PRD-018:** Combat animations work
- **PRD-019:** Zone completion works
- **PRD-020:** 60fps with SDFGI/SSAO
- **PRD-021:** 50 items functional
- **PRD-022:** 20 abilities work
- **PRD-023:** Guilds work
- **PRD-024:** Parties work
- **PRD-025:** 10 quests work
- **PRD-026:** Trade flow works

---

## Dependencies Flow

```
PRD-017 (GNS Runtime)
    ↓
PRD-018 (FSM Integration)
    ↓
PRD-019 (Zone Objectives)
    ↓
PRD-021 (Inventory)
    ↓
PRD-022 (Abilities)
    ↓
PRD-023-026 (Social/Economy)

PRD-020 (Lighting) - Independent
```

---

## Files Created

1. `prd-017-gns-runtime-integration.md` - GNS Runtime Integration
2. `prd-018-fsm-integration.md` - Node-Based FSM Integration  
3. `prd-019-zone-objectives-integration.md` - Zone Objectives Integration
4. `prd-020-sdfgi-ssao-lighting.md` - SDFGI/SSAO Lighting
5. `prd-021-inventory-equipment-system.md` - Inventory Equipment System
6. `prd-022-abilities-talents-system.md` - Abilities Talents System
7. `prd-023-guild-system.md` - Guild System Implementation
8. `prd-024-party-system.md` - Party System Implementation
9. `prd-025-quest-system.md` - Quest System Implementation
10. `prd-026-trade-system.md` - Trade System Implementation

---

**Generated:** 2026-05-02  
**Author:** OpenHands Analysis  
**Status:** PRD Documents Created - Ready for Implementation