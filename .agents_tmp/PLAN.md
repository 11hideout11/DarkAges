# 1. OBJECTIVE

Advance the DarkAges MMO project toward MVP (Minimum Viable Product) by completing server-side and parallel implementation tasks that don't conflict with the Hermes agent's PRD-008 work (CombatStateMachine integration to Player.tscn/RemotePlayer.tscn).

Target areas:
- PRD-010: Hitbox/Hurtbox edge-case test expansion
- PRD-009: Server-side ZoneObjectiveSystem integration (tick loop + snapshot)
- PRD-012: GNS runtime abstraction layer (interface + stub)

These tasks are prioritized because:
1. They don't conflict with PRD-008 client integration work
2. They directly support the MVP criteria (combat validation, zone objectives, production networking)
3. They can be tested independently with existing test infrastructure

# 2. CONTEXT SUMMARY

## Hermes Agent Work (Non-Conflict Verification)

The Hermes agent explicitly stated they are working on:
- **PRD-008 integration**: Connect CombatStateMachine.tscn to Player.tscn and RemotePlayer.tscn
- Wire signal callbacks between AnimationStateMachine and CombatStateMachineController
- Integration testing in Godot editor

**What I must NOT do** (conflicts):
- ❌ Modify Player.tscn scene structure
- ❌ Modify RemotePlayer.tscn scene structure  
- ❌ Wire signals in player scripts
- ❌ Create Godot integration tests

## What I CAN Do (Parallel/Independent Work)

| PRD | Task | Conflict Risk | Dependencies |
|-----|------|------------|-------------|
| PRD-010 | Edge-case hitbox tests | LOW - pure C++ unit tests | PRD-010 core exists |
| PRD-009 | Server tick loop integration | LOW - ZoneObjectiveSystem different path | PRD-009 system exists |
| PRD-012 | GNS interface abstraction | LOW - networking layer, no player code | Compile-time fixed |

## Current Test Baseline (Verified)
- 2129 test cases, 12644 assertions, 100% passing
- unit_tests: 1302 cases
- test_combat: 140 cases
- test_zones: 198 cases |

# 3. APPROACH OVERVIEW

Three parallel work packages that can advance MVP without conflict:

### Approach A: PRD-010 Edge-Case Tests (Pure Server C++)
- Create edge-case test file: `TestHitboxEdgeCases.cpp`
- Test multi-hit simultaneous, iframes, boundary conditions
- No scene modifications, no Godot code
- Runs in existing test infrastructure

### Approach B: PRD-009 Server-Side Integration
- Connect `ZoneObjectiveComponent` to ZoneServer tick loop
- Add to snapshot serialization
- Server-only changes in C++

### Approach C: PRD-012 Network Socket Interface
- Create `INetworkSocket.hpp` interface
- Preserve existing `StubSocket` (current UDP code)
- Factory pattern for ENABLE_GNS=ON/OFF
- No player/scene modifications
| PRD-013 | Phase 1-5 Implementation Verification | 🟡 In Progress | P0 |
| PRD-014 | Phantom Camera 3rd-Person | 🟡 In Progress | P1 |
| PRD-015 | Procedural Leaning System | 🟡 In Progress | P1 |
| PRD-016 | SDFGI/SSAO/SSIL Post-Processing | 🟡 In Progress | P2 |
| PRD-017 | Protocol Layer Decoupling | 🟡 In Progress | P2 |
| PRD-018 | Production Database Integration | 🟡 In Progress | P2 |
| PRD-019 | AnimationTree Blend Spaces | 🟡 In Progress | P2 |
| PRD-020 | Godot Client Headless Fixes | 🟡 In Progress | P3 |
| PRD-021 | Demo Validator Connection Pooling | 🟡 In Progress | P3 |
| PRD-022 | Combat FSM Visual Polish | 🟡 In Progress | P2 |
| PRD-023 | Combat Floating Text Integration | 🟡 In Progress | P2 |
| PRD-024 | Documentation Audit | ✅ Complete | - |

# 4. IMPLEMENTATION STEPS

## Step 4.1: PRD-010 Edge-Case Hitbox Tests
**Goal:** Expand test coverage with edge cases not currently covered
**Method:** Create `TestHitboxEdgeCases.cpp` with multi-hit, iframes, rewind boundary tests

Reference: `src/server/tests/TestHitboxHurtbox.cpp` (existing pattern)

Test cases to add:
- Two hitboxes overlapping same hurtbox → only first registers
- Hitbox deactivation during active attack
- Hurtbox invulnerability frames (iframes)
- Rewind to exactly 2.000s boundary
- Hitbox offset + rotation edge cases
- Multiple hurtboxes on same entity

**Estimated:** 4 hours
**Risk:** LOW - isolated to test code

## Step 4.2: PRD-009 Server Tick Loop Integration
**Goal:** Connect ZoneObjectiveSystem to ZoneServer tick loop
**Method:** Add objective update call in main tick function

Reference: `src/server/src/zones/ZoneServer.cpp` (existing tick loop)

Tasks:
1. Add `#include "zones/ZoneObjectiveSystem.hpp"`
2. Add `ZoneObjectiveSystem::process(objects_, dt)` in tick loop
3. Add objective data to snapshot serialization
4. Verify builds and existing tests pass

**Estimated:** 4 hours
**Risk:** LOW - server-only changes

## Step 4.3: PRD-012 Network Socket Interface
**Goal:** Create abstraction layer for GNS/stub switching
**Method:** Define INetworkSocket interface, implement factory

Reference: `src/server/src/networking/NetworkSocket.cpp` (current implementation)

Tasks:
1. Create `src/server/include/networking/INetworkSocket.hpp`
2. Implement `GNSSocket.cpp` (GNS-backed)
3. Implement `StubSocket.cpp` (current UDP, extracted)
4. Create `NetworkSocketFactory.cpp`
5. Add ENABLE_GNS conditional in ZoneServer

**Estimated:** 6 hours
**Risk:** MEDIUM - requires careful interface design

# 5. TESTING AND VALIDATION

## Validation Criteria

### PRD-010 Tests
- [ ] New test file compiles
- [ ] All edge-case tests pass
- [ ] No test regressions (baseline 2129 cases maintained)
- [ ] Build: `cmake --build build -j$(nproc)` passes

### PRD-009 Integration
- [ ] Server starts with zone objectives
- [ ] Objectives tick in ZoneServer
- [ ] Snapshot includes objective data
- [ ] Existing zone tests pass

### PRD-012 Interface
- [ ] Interface compiles
- [ ] Stub mode (ENABLE_GNS=OFF) passes existing tests
- [ ] Factory correctly instantiates stub
- [ ] No protocol changes (wire format unchanged)

## Acceptance Criteria Summary
- No test regressions (2129 cases, 12644 assertions)
- All PRDs move from "pending" to "in progress" or "complete"
- Build validates without errors
- Demo pipeline operational (existing stub path)

---

## Acceptance Criteria Summary
- No test regressions (2129 cases, 12644 assertions)
- All PRDs move from "pending" to "in progress" or "complete"
- Build validates without errors
- Demo pipeline operational (existing stub path)

**Plan Status:** Ready for implementation  
**Focus:** Server-side and parallel tasks that don't conflict with Hermes agent PRD-008 work
