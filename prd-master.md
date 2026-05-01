# DarkAges MMO — Product Requirements Master Document

**Version:** 1.0  
**Date:** 2026-05-01  
**Status:** Initial Specification  

---

## Executive Summary

This document consolidates all identified gaps, problems, and areas for improvement in the DarkAges MMO project into a unified product requirements specification. Eight distinct PRDs have been created addressing critical infrastructure, gameplay, and polish areas.

### Project State Overview

| Aspect | Status | Notes |
|--------|--------|-------|
| Server | ✅ Operational | 32K LOC, 60Hz tick |
| Client | ✅ Functional | 9K LOC, Godot 4.2.4 |
| Tests | ✅ 2,129 passing | 12,644 assertions |
| Combat FSM | ✅ Complete | PR #28 merged |
| Demo Mode | ✅ Operational | Zones configured |
| GNS Networking | ⚠️ Blocked | WebRTC dependency |
| Art Pipeline | ⚠️ Research Phase | Implementation pending |
| Documentation | ⚠️ Inconsistent | Drift identified |

---

## Gap Analysis Summary

### Critical Gaps (Must Address for MVP)

| # | Gap | Impact | Priority | PRD Reference |
|---|-----|-------|----------|---------------|
| 1 | GNS Integration Blocked | Production networking | Critical | `prd-gns-*-*.md` |
| 2 | Protocol. cpp Excluded | Wire format untested | Critical | `prd-protobuf-*.md` |
| 3 | Phase 1-5 Undocumented | Verification gap | Critical | `prd-docs-*.md` |
| 4 | Demo Readiness Contradiction | Stakeholder confusion | High | `prd-docs-*.md` |

### High-Priority Gaps (Major Functionality)

| # | Gap | Impact | Priority | PRD Reference |
|---|-----|-------|----------|---------------|
| 5 | Art Pipeline Not Implemented | Visual quality | High | `prd-art-*-*.md` |
| 6 | No Node-Based FSM | Extensibility | High | `prd-fsm-*.md` |
| 7 | No Inventory System | Progression depth | Medium | `prd-inventory-*.md` |
| 8 | No Abilities System | Skill variety | Medium | `prd-abilities-*.md` |

### Medium-Priority Gaps (Polish)

| # | Gap | Impact | Priority | PRD Reference |
|---|-----|-------|----------|---------------|
| 9 | No Foot IK | Visual polish | Low | `prd-foot-ik-*.md` |
| 10 | Blend Spaces Not Implemented | Animation quality | Low | Deferred |
| 11 | Validator Connection Handling | Testing tool | Low | Issue #9 tracked |

---

## PRD Inventory

### PRD-1: GNS Networking Integration Completion
- **Filename:** `prd-gns-networking-completion.blob`  
- **Status:** Blocked by WebRTC access  
- **Key Deliverables:**
  - Alternative WebRTC source resolution
  - CMake fallback logic
  - GNS integration tests
- **Success Criteria:**
  - `ENABLE_GNS=ON` builds succeed without WebRTC submodule
  - 10,000 concurrent connections supported
  - 99.99% packet delivery achieved

### PRD-2: Protobuf Protocol Implementation
- **Filename:** `prd-protobuf-protocol-impl.blob`  
- **Status:** Excluded when GNS disabled  
- **Key Deliverables:**
  - Protocol. cpp enabled in all builds
  - Wire format validation tests
  - Schema documentation
- **Success Criteria:**
  - Protobuf path tested in CI
  - Backward compatibility maintained
  - Version negotiation implemented

### PRD-3: Art Pipeline Implementation
- **Filename:** `prd-art-pipeline-impl.blob`  
- **Status:** Research complete, implementation pending  
- **Key Deliverables:**
  - Character creation pipeline
  - 3 zones with modular kit
  - VFX shaders (damage, outline, dissolve)
  - Draw calls <80 per frame
- **Success Criteria:**
  - Demo-Ready visuals achieved
  - All Tier 1 assets completed
  - Zone loading <5 seconds

### PRD-4: Node-Based FSM Implementation
- **Filename:** `prd-node-based-fsm-impl.blob`  
- **Status:** Using inline state flags  
- **Key Deliverables:**
  - State base class
  - 12+ combat states
  - AnimationTree integration
  - State hierarchy support
- **Success Criteria:**
  - All existing tests pass
  - New states addable without modification
  - FSM test coverage >90%

### PRD-5: Foot IK System
- **Filename:** `prd-foot-ik-system.blob`  
- **Status:** Not implemented  
- **Key Deliverables:**
  - SkeletonIK3D integration
  - Terrain raycast system
  - Stair handling
- **Success Criteria:**
  - Feet align to terrain
  - 60fps maintained at 400 entities

### PRD-6: Abilities & Talents System
- **Filename:** `prd-abilities-talents-system.blob`  
- **Status:** Basic attacks only  
- **Key Deliverables:**
  - Abilities framework
  - 3 talent trees (10 points each)
  - 20+ abilities
  - Server validation
- **Success Criteria:**
  - 2 new abilities added per sprint
  - All abilities server-validated

### PRD-7: Inventory & Equipment System
- **Filename:** `prd-inventory-equipment-system.blob`  
- **Status:** Minimal placeholder  
- **Key Deliverables:**
  - 7 equipment slots
  - 50+ item database
  - Inventory UI with tooltips
  - Loot system
- **Success Criteria:**
  - Drag-and-drop equipment works
  - Loot drops functional

### PRD-8: Documentation Remediation
- **Filename:** `prd-documentation-remediation.blob`  
- **Status:** Multiple inconsistencies  
- **Key Deliverables:**
  - Doc audit script
  - Consistent PROJECT_STATUS.md
  - Updated AGENTS.md
  - Documentation workflow
- **Success Criteria:**
  - 0 contradictory statements
  - Automated doc linting passing

---

## Implementation Roadmap

### Phase 1: Critical Infrastructure (Weeks 1-4)

```
Week 1-2: Documentation Remediation
├── Audit all .md files
├── Update PROJECT_STATUS.md
├── Update AGENTS.md
└── Establish update workflow

Week 3-4: GNS Resolution
├── Evaluate WebRTC alternatives
├── Implement fallback logic
└── Test protocol paths
```

### Phase 2: Gameplay Foundation (Weeks 5-8)

```
Week 5-6: Node-Based FSM
├── Implement State base class
├── Create combat states
└── Integrate with AnimationTree

Week 7-8: Abilities Framework
├── Design ability types
├── Create 5 baseline abilities
└── Implement talent trees
```

### Phase 3: Content Systems (Weeks 9-12)

```
Week 9-10: Inventory System
├── Design item schema
├── Create equipment slots
└── Build inventory UI

Week 11-12: Art Pipeline Start
├── Toolchain validation
├── Character pipeline
└── VFX shaders
```

### Phase 4: Polish (Weeks 13-16)

```
Week 13-14: Foot IK
├── SkeletonIK3D setup
├── Terrain alignment
└── Stair handling

Week 15-16: Art Pipeline Complete
├── Character animations
├── Zone environments
└── Performance validation
```

---

## Prioritization Criteria

Each PRD was evaluated on:

1. **Business Impact:** Effect on demo readiness, user engagement
2. **Technical Risk:** Complexity, dependencies, testability
3. **Resource Requirements:** Time, skills, assets needed
4. **Dependencies:** What must complete first
5. **Reversibility:** Ease of rollback if issues arise

---

## Resource Estimates

| PRD | Difficulty | Time | Skills Required | Dependencies |
|----|-----------|------|----------------|--------------|
| PRD-1 (GNS) | High | 2 weeks | C++, CMake | None |
| PRD-2 (Protocol) | Medium | 1 week | Protobuf, C++ | PRD-1 |
| PRD-3 (Art) | High | 12 weeks | 3D Art, Blender | None |
| PRD-4 (FSM) | Medium | 4 weeks | C#, Godot | None |
| PRD-5 (Foot IK) | Medium | 2 weeks | C#, Godot | PRD-4 |
| PRD-6 (Abilities) | Medium | 4 weeks | C#, Game Design | PRD-4 |
| PRD-7 (Inventory) | Medium | 4 weeks | C#, UI Design | PRD-6 |
| PRD-8 (Docs) | Low | 1 week | Documentation | None |

---

## Success Metrics

### Project-Level
- All 8 PRDs implemented
- Test suite maintains 100% pass rate
- No documentation inconsistencies
- Demo mode validated with all features

### Per-PRD Checkpoints
- **PRD-1:** GNS builds pass in CI
- **PRD-2:** Wire format tests validate
- **PRD-3:** User feedback on visuals positive
- **PRD-4:** FSM test coverage >90%
- **PRD-5:** Feet visually align to terrain
- **PRD-6:** 20 abilities functional
- **PRD-7:** Equipment system complete
- **PRD-8:** Automated linting passes

---

## Open Questions

1. **GNS Deployment:** Is Steam deployment a hard requirement?
2. **Art Investment:** What is the budget for external artists?
3. **Character Customization:** Is character creation planned?
4. **Monetization:** What is the monetization model?
5. **Platform:** Is mobile platform in scope?

---

## Appendix: File Reference

| File | Purpose |
|------|--------|
| `prd-gns-networking-completion.blob` | GNS integration full spec |
| `prd-protobuf-protocol-implementation.blob` | Protocol implementation spec |
| `prd-art-pipeline-implementation.blob` | Art pipeline implementation spec |
| `prd-node-based-fsm-implementation.blob` | FSM implementation spec |
| `prd-foot-ik-system.blob` | Foot IK system spec |
| `prd-abilities-talents-system.blob` | Abilities system spec |
| `prd-inventory-equipment-system.blob` | Inventory system spec |
| `prd-documentation-remediation.blob` | Documentation remediation spec |
| `PROJECT_ISSUSES_TRACKER.blob` | Issue tracking baseline |
| `COMPATIBILITY_ANALYSIS.blob` | Standards alignment reference |

---

*Generated: 2026-05-01*  
*Version: 1.0*  
*Author: OpenHands Analysis*