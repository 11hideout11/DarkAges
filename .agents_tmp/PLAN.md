# DarkAges Implementation Plan - PRD Progression

**Last Updated:** 2026-05-03  
**Review Focus:** Progress critical PRDs from gap analysis  
**Selected Task:** Complete PRD-029 Client UI Integration + PRD-031 NPC AI Integration

---

# 1. OBJECTIVE

Complete meaningful PRD work that delivers player-facing gameplay impact:

1. **PRD-029: Client UI Integration** (HIGH priority) - Wire client UI panels to server data
2. **PRD-031: NPC AI Behavior System** (MEDIUM priority) - Integrate AI into zone tick loop

These are explicitly identified as gaps in PRD_GAP_ANALYSIS_SUMMARY.md and are autonomous-safe.

---

# 2. CONTEXT SUMMARY

## Verified State
| Component | Status |
|-----------|--------|
| Server Inventory System | ✅ Implemented (PRD-021) |
| Server Ability System | ✅ Implemented (PRD-022) |
| Server Quest System | ✅ Implemented (PRD-025) |
| NPCAISystem.hpp | ✅ Exists (stub implementation) |
| InventoryPanel.cs | ✅ Exists (demo data) |
| AbilityBar.cs | ✅ Exists |
| QuestTracker.cs | ✅ Exists |

## What's Missing (Gap Analysis)
| Gap | PRD | Impact |
|-----|-----|--------|
| Client UI panels not wired to server | PRD-029 | Players can't see inventory/abilities/quests |
| NPCAISystem not in tick loop | PRD-031 | NPCs don't have AI behavior |

---

# 3. APPROACH OVERVIEW

## Selected Approach
1. **Phase 1: PRD-029 Client UI Integration**
   - Wire InventoryPanel to receive server sync data
   - Wire AbilityBar to receive ability data from server
   - Wire QuestTracker to quest log data
   - Add network packet handlers for RPG data sync

2. **Phase 2: PRD-031 NPC AI Integration**
   - Add NPCAISystem::update() to ZoneServer tick loop
   - Wire AI damage callback to combat events
   - Add AI state component to NPCs

## Why This Approach
- Explicitly identified as HIGH priority gaps
- Autonomous-safe (no specialists required)
- High gameplay impact for demo
- Server components already exist

---

# 4. IMPLEMENTATION STEPS

## Step 4.1: Prepare Network Protocol for RPG Data Sync
**Goal:** Add packet types for inventory/ability/quest sync
**Method:** Extend Protocol.cpp with serialization functions

Reference: `src/server/src/netcode/Protocol.cpp`

Tasks:
- [ ] Add PACKET_INVENTORY_UPDATE serialization
- [ ] Add PACKET_ABILITY_UPDATE serialization  
- [ ] Add PACKET_QUEST_LOG_SYNC serialization
- [ ] Add corresponding packet IDs to PacketTypes.hpp

## Step 4.2: Add Client Network Handlers
**Goal:** Process server data on client
**Method:** Add handler methods in NetworkManager

Reference: `src/client/src/networking/NetworkManager.cs`

Tasks:
- [ ] Add ProcessInventoryUpdate() method
- [ ] Add ProcessAbilityUpdate() method
- [ ] Add ProcessQuestSync() method
- [ ] Emit signals for UI update

## Step 4.3: Wire Inventory Panel to Network
**Goal:** Display real inventory data
**Method:** Connect NetworkManager signals to InventoryPanel

Reference: `src/client/src/ui/InventoryPanel.cs`

Tasks:
- [ ] Add NetworkManager reference
- [ ] Connect to InventoryUpdate signal
- [ ] Add UpdateSlots() method to refresh display
- [ ] Add item icon loading (placeholder textures)

## Step 4.4: Wire Ability Bar to Network
**Goal:** Display real ability data  
**Method:** Connect NetworkManager signals to AbilityBar

Reference: `src/client/src/ui/AbilityBar.cs`

Tasks:
- [ ] Add NetworkManager reference
- [ ] Connect to AbilityUpdate signal  
- [ ] Populate ability buttons from server data
- [ ] Add cooldown visualization

## Step 4.5: Wire Quest Tracker to Quest Log
**Goal:** Display real quest data
**Method:** Connect network signals to QuestTracker

Reference: `src/client/src/ui/QuestTracker.cs`

Tasks:
- [ ] Add NetworkManager reference
- [ ] Connect to QuestSync signal
- [ ] Update objectives display from server data
- [ ] Add progress tracking UI

## Step 4.6: Integrate NPC AI into ZoneServer Tick
**Goal:** NPCs exhibit behavior (idle/wander/chase/attack)
**Method:** Call NPCAISystem::update() in zone loop

Reference: `src/server/src/zones/ZoneServer.cpp`

Tasks:
- [ ] Add npcAiSystem_ member to ZoneServer
- [ ] Call aiSystem.update() in Tick()
- [ ] Wire AI damage callback to combat events
- [ ] Add NPCAIState component to spawned NPCs

## Step 4.7: Build and Test
**Goal:** Verify no regressions
**Method:** Compile + run test suite

Reference: `tools/demo/demo --quick`

Tasks:
- [ ] Compile server with -DENABLE_GNS=OFF
- [ ] Run unit tests (1299 cases minimum)
- [ ] Verify GNS build compiles
- [ ] Verify no warnings in new code

---

# 5. TESTING AND VALIDATION

## Validation Criteria

### For PRD-029 (Client UI)
- [ ] Inventory panel shows items from server sync
- [ ] Ability bar shows abilities with cooldowns
- [ ] Quest tracker shows active quests
- [ ] Network packets deserialize correctly
- [ ] UI updates on data change

### For PRD-031 (NPC AI)
- [ ] NPCAISystem::update() compiles and runs
- [ ] NPCs transition between states (idle->chase->attack)
- [ ] AI damages players in melee range
- [ ] No compile errors in ZoneServer

### Success Indicators
- Server compiles without errors
- All 1299+ test cases pass
- GNS build compiles (no regressions)
- Client UI panels functional with real data
- NPCs exhibit basic AI behavior in demo zones
