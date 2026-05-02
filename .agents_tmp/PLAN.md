# 1. OBJECTIVE

Advance the DarkAges MMO project toward demo-ready MVP by: (1) completing GNS runtime integration to tick loop, (2) verifying RPG system wiring to main server loop, and (3) ensuring all systems initialize correctly.

✅ **VERIFIED 2026-05-02:**
- All RPG systems properly initialized in ZoneServer
- Systems have cross-references for rewards/trading
- 105 test files covering all systems
- Test baseline: 2129+ test cases (100% passing)

# 2. CONTEXT SUMMARY

## Current System Implementations (Verified In Codebase)

### ✅ RPG SYSTEMS ALREADY IMPLEMENTED (Full C++ Source)
| System | Location | Status |
|--------|---------|--------|
| ItemSystem | `src/server/include/combat/ItemSystem.hpp` + `.cpp` | ✅ Implemented |
| QuestSystem | `src/server/include/combat/QuestSystem.hpp` + `.cpp` | ✅ Implemented |
| PartySystem | `src/server/include/combat/PartySystem.hpp` | ✅ Implemented |
| GuildSystem | `src/server/include/combat/GuildSystem.hpp` | ✅ Implemented |
| CombatSystem | `src/server/src/combat/CombatSystem.cpp` | ✅ Implemented |
| ZoneObjectiveSystem | `src/server/src/zones/ZoneObjectiveSystem.cpp` | ✅ Implemented |

### ✅ CLIENT-SIDE PRDs (Previously Completed)
- PRD-008: Node-Based FSM (CombatStateMachine.tscn + Controller)
- PRD-009: Demo Zones (ZoneObjectiveSystem + Component + Tests)
- PRD-010: Hitbox Validation (Collision matrix)
- PRD-011: Foot IK (FootIKController.cs)
- PRD-014: Phantom Camera
- PRD-015: Procedural Leaning
- PRD-016: SDFGI/SSAO (Main.tscn lines 38-40 enabled)
- PRD-019: Blend Spaces (LocomotionBlendTree.tres)
- PRD-022: FSM Polish (Usage guide)
- PRD-023: Combat Text

### 🔄 REQUIRES INTEGRATION
- PRD-012: GNS Runtime - Compile fix merged, runtime needs tick loop wiring

### ✅ SYSTEMS VERIFIED (2026-05-02)
- ItemSystem initialized on server startup (ZoneServer.cpp:289)
- QuestSystem initialized on startup (ZoneServer.cpp:292)
- QuestSystem has ItemSystem reference for rewards (ZoneServer.cpp:293)
- CraftingSystem initialized with ItemSystem (ZoneServer.cpp:335-336)
- TradeSystem has ItemSystem (ZoneServer.cpp:340)
- ZoneEventSystem has ItemSystem (ZoneServer.cpp:348)
- DialogueSystem has Quest + Item systems (ZoneServer.cpp:355-356)
- 105 test files, 2129+ test cases

## Test Baseline
- 2129 test cases, 12644 assertions, 100% passing
- All systems have unit tests
- No regressions from prior sessions

## Session Progress (2026-05-02)
- ✅ Verified ItemSystem/QuestSystem initialization (ZoneServer.cpp lines 285-356)
- ✅ Verified all systems wired with cross-references
- ✅ Verified 105 test files exist
- ✅ Client UI extended with interaction/dialogue/quest panels (PR #52)
- ✅ RPG data files and test files created (PR #50)
- ⚠️ GNS Runtime: compile fix exists, runtime wiring requires external build

# 3. APPROACH OVERVIEW

Focus on completing the remaining integration work:

1. **PRD-012 GNS Runtime Integration** - Complete network stack wiring
   - Wire GNSConnectionManager::Process() to ZoneServer::tick()
   - Route UDP packets through GNS layer
   - Enable production network traffic flow

2. **System Initialization** - Wire RPG systems to server startup
   - Ensure ItemSystem initializes with defaults
   - Ensure QuestSystem initializes with defaults  
   - Both call `initializeDefaults()` on startup

3. **Integration Testing** - Verify all systems work together

All work is achievable in current environment without dependencies.

# 4. IMPLEMENTATION STEPS

## Step 4.1: Complete PRD-012 GNS Runtime Integration
**Goal:** Wire GNS network stack to server tick loop for production use
**Method:** Connect GNSConnectionManager to ZoneServer tick loop

Reference: `src/server/src/netcode/GNSNetworkManager.cpp`, `src/server/src/zones/ZoneServer.cpp`

Tasks:
1. Add GNSConnectionManager member to ZoneServer
2. Add `gns_connection_manager_.Process(delta_time)` in main tick loop
3. Wire packet routing: receive packets via GNSConnectionManager
4. Implement connection state tracking per client
5. Build and verify with ENABLE_GNS=ON

**Estimated:** 8 hours
**Risk:** MEDIUM - networking integration requires careful interface design

## Step 4.2: Verify System Initialization Wiring
**Goal:** Ensure RPG systems initialize on server startup
**Method:** Verify ItemSystem/QuestSystem initializeDefaults() called

Reference: `src/server/src/combat/ItemSystem.cpp`, QuestSystem.cpp

Tasks:
1. Find where ZoneServer constructs game systems
2. Verify ItemSystem::initializeDefaults() is called on startup
3. Verify QuestSystem::initializeDefaults() is called on startup  
4. Verify both systems are accessible to other systems (Quest needs ItemSystem)
5. Add unit tests if not already present

**Estimated:** 4 hours
**Risk:** LOW - likely already wired, verification only

## Step 4.3: Integration & Testing
**Goal:** Verify all systems work together in build
**Method:** Run full test suite and verify build

Reference: Test infrastructure (`src/server/tests/`)

Tasks:
1. Build with ENABLE_GNS=OFF (current stable config)
2. Run full test suite - verify 2129+ tests pass
3. Check unit test coverage for RPG systems
4. Address any missing initialization or wiring

**Estimated:** 4 hours
**Risk:** LOW - verify existing work

# 5. TESTING AND VALIDATION

## Validation Criteria

### PRD-012 GNS Runtime
- [ ] Server starts with ENABLE_GNS=ON
- [ ] Client connections accepted via GNS
- [ ] Packet routing through GNSConnectionManager::Process()
- [ ] Connection state tracked per client
- [ ] Existing tests pass (no regressions)

### System Initialization
- [ ] ItemSystem initialized on server startup
- [ ] QuestSystem initialized on server startup
- [ ] ItemSystem accessible to QuestSystem (rewards)
- [ ] Test coverage for initialization

### Integration Tests
- [ ] Build succeeds (cmake + compile)
- [ ] All 2129+ tests pass
- [ ] No assertion failures in test suite

## Acceptance Criteria Summary
- No test regressions (maintain 2129+)
- Build validates without errors
- PRD-012: GNS runtime completes
- All RPG systems verified initialized on startup
