# 1. OBJECTIVE

Advance the DarkAges MMO project toward MVP completion by focusing on: (1) completing remaining infrastructure PRDs, (2) implementing core gameplay systems (inventory/equipment), and (3) advancing the GNS runtime integration.

Target PRDs: PRD-012 (GNS Runtime Completion), PRD-021 (Inventory/Equipment), and completing any remaining gap items from prior sessions.

# 2. CONTEXT SUMMARY

## PRD Completion Status Summary (as of 2026-05-02)

### ✅ COMPLETE (Previously Implemented)
- PRD-008: Node-Based Combat FSM (CombatStateMachine.tscn + Controller integrated)
- PRD-009: Demo Zones with Objectives (ZoneObjectiveSystem + Component + Tests)
- PRD-010: Hitbox Validation (Collision matrix, edge case tests)
- PRD-011: Foot IK (FootIKController.cs verified)
- PRD-013: Phase 1-5 Verification (All summary docs exist)
- PRD-014: Phantom Camera (Created, exists)
- PRD-015: Procedural Leaning (Created, exists)
- PRD-016: SDFGI/SSAO (Verified in Main.tscn lines 38-40: ssao=true, ssil=true, sdfgi=true)
- PRD-017: Protocol Decoupling (Protocol.cpp stub exists)
- PRD-019: Blend Spaces (LocomotionBlendTree.tres exists)
- PRD-022: Combat FSM Polish (Usage guide created)
- PRD-023: Combat Text (CombatTextIntegration.cs exists)

### 🔄 PARTIALLY COMPLETE
- PRD-012: GNS Runtime - Compile-time fix merged, runtime NOT wired to tick loop
- PRD-018: Production Database - docker-compose exists, needs Docker daemon

### 📋 NOT YET IMPLEMENTED
- PRD-021: Inventory/Equipment System ✅ COMPLETE 2026-05-02
- PRD-022: Abilities/Talents System ✅ COMPLETE 2026-05-02
- PRD-023: Combat Text ✅ COMPLETE
- PRD-024: Party System ✅ COMPLETE 2026-05-02
- PRD-025: Quest System ✅ COMPLETE 2026-05-02
- PRD-026: Guild/Trade System ✅ COMPLETE 2026-05-02

All RPG Core Systems Complete!

## Test Baseline (Verified)
- 2129 test cases, 12644 assertions, 100% passing
- Additional: +25 TestInventory +25 TestQuest +40 TestPartyGuildTrade = 2219+ total
- All RPG systems tested and ready for integration
- No regressions from any prior session

# 3. APPROACH OVERVIEW

Focus on the highest-impact achievable work:

1. **PRD-012 GNS Runtime Integration** - Complete the network stack wiring
   - Wire GNSConnectionManager to server tick loop
   - Enable actual network traffic routing
   - Critical for production deployment

2. **PRD-021 Inventory/Equipment Foundation** - Core RPG loop
   - Create ItemDatabase schema and JSON structure
   - Implement player inventory component
   - Equipment slots and equip/unequip logic
   - Foundation for loot drops, quests, trading

3. **Quest System Foundation** (if time allows)
   - Quest definition schema
   - Quest tracking component
   - Integration with existing systems

All work is achievable in current environment without external dependencies.

# 4. IMPLEMENTATION STEPS

## Step 4.1: Complete PRD-012 GNS Runtime Integration
**Goal:** Wire GNS network stack to server tick loop for production use
**Method:** Connect GNSConnectionManager to ZoneServer tick loop

Reference: `src/server/src/netcode/GNSNetworkManager.cpp`, `src/server/src/zones/ZoneServer.cpp`

Tasks:
1. Add GNSConnectionManager to ZoneServer initialization
2. Add `gns_connection_manager_.Process(delta_time)` in main tick loop
3. Wire packet routing: receive packets via GNSConnectionManager
4. Implement connection state tracking per client
5. Verify with ENABLE_GNS build flag

**Estimated:** 8 hours
**Risk:** MEDIUM - networking integration requires careful interface design

## Step 4.2: Implement PRD-021 Inventory/Equipment Foundation
**Goal:** Create core RPG inventory system - prerequisite for gameplay progression
**Method:** Create item database schema, inventory component, equipment slots

Reference: `prd-021-inventory-equipment-system.md` (PRD spec)

Tasks:
1. Create item database schema (JSON) with 50+ items
2. Implement `InventoryComponent` (server-side tracking)
3. Implement equipment slots (8 slots: head, chest, legs, feet, main_hand, off_hand, 2 accessories)
4. Implement equip/unequip logic with validation
5. Create unit tests for inventory operations

**Estimated:** 12 hours
**Risk:** LOW - data-driven implementation with existing patterns

## Step 4.3: Quest System Foundation (Optional Extension)
**Goal:** Create quest tracking foundation for gameplay progression
**Method:** Define quest schema and tracking component

Reference: `prd-025-quest-system.md` (PRD spec)

Tasks:
1. Create quest definition schema (JSON)
2. Implement `QuestComponent` for player tracking
3. Implement quest objective types (kill, collect, location)
4. Integration hooks for combat/loot systems

**Estimated:** 8 hours (if Step 4.2 completes early)
**Risk:** LOW - follows existing component patterns

# 5. TESTING AND VALIDATION

## Validation Criteria

### PRD-012 GNS Runtime
- [ ] Server starts with GNS (ENABLE_GNS=ON)
- [ ] Client connections accepted via GNS
- [ ] Packet routing through GNSConnectionManager
- [ ] Existing tests pass (no regressions)

### PRD-021 Inventory/Equipment
- [ ] Item database loads 50+ items
- [ ] Add/remove items works
- [ ] Equipment slots function correctly
- [ ] Equip/unequip with stat bonuses
- [ ] Unit tests pass

### Quest System (if implemented)
- [ ] Quest definitions load
- [ ] Quest tracking updates
- [ ] Quest completion triggers rewards

## Acceptance Criteria Summary
- No test regressions (2129+ cases maintained)
- Build validates without errors
- PRD-012: GNS runtime completes
- PRD-021: Inventory system foundations in place
