# PRD-027: GNS Runtime Integration - Complete

**Version:** 2.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical (1A)  
**Requires:** Phase 8 compile-time fix (COMPLETE)

---

## 1. Introduction/Overview

Integrate GameNetworkingSockets (GNS) into the server tick loop for production networking. The compile-time fix is complete (PR #57), but the runtime integration is not wired to handle actual network traffic. This PRD completes the network stack for production use.

### Problem Statement
- GNS compile-time issues resolved in Phase 8
- GNSNetworkManager.cpp implementation exists but not wired to tick loop
- Cannot run production server with GNS networking

---

## 2. Goals

- Integrate GNS network layer into server tick loop
- Route all UDP traffic through GNS connection manager
- Implement connection state management
- Enable server-authoritative networking
- Support 10,000+ concurrent connections

---

## 3. User Stories

### US-027-001: Server Starts with GNS
**Description:** As a server operator, I want the server to start with GNS networking enabled so that production traffic uses the GNS stack.

**Acceptance Criteria:**
- [ ] Server compiles with GNS_ENABLED flag
- [ ] Server binds to port and listens for connections
- [ ] GNSConnectionManager::Initialize() called on startup

### US-027-002: Client Connections Accepted
**Description:** As a player, I want to connect to the server so that I can play the game.

**Acceptance Criteria:**
- [ ] Client connection request accepted via GNS
- [ ] Connection handle created and tracked
- [ ] Connection state transitions to Connected

### US-027-003: Packet Routing Through GNS
**Description:** As a server, I want all packets routed through GNS so that traffic uses the production network stack.

**Acceptance Criteria:**
- [ ] Receive() routes incoming packets through GNS
- [ ] Send() routes outgoing packets through GNS
- [ ] Both unreliable (game) and reliable (event) channels work

### US-027-004: Connection State Management
**Description:** As a server, I want to track connection state so that I can manage client sessions.

**Acceptance Criteria:**
- [ ] Connection state tracked per client (Connecting, Connected, Disconnecting)
- [ ] Timeout detection (no packets for 30 seconds = disconnect)
- [ ] Clean disconnect handling

### US-027-005: Disconnect Handling
**Description:** As a server, I want to handle client disconnects gracefully so that resources are freed.

**Acceptance Criteria:**
- [ ] GNSConnectionManager::Disconnect() called on timeout
- [ ] Connection handle removed from tracking
- [ ] Player entity cleaned up

---

## 4. Functional Requirements

- FR-027-1: Server starts GNSConnectionManager in main.cpp with port from config
- FR-027-2: Tick loop calls GNSConnectionManager::Process() every frame
- FR-027-3: GNSConnectionManager::Accept() returns valid handle for new connections
- FR-027-4: GNSConnectionManager::Receive() delivers packets to packet handler
- FR-027-5: GNSConnectionManager::Send() sends packets to specific connection
- FR-027-6: Connection state machine tracks: Connecting → Connected → Disconnecting
- FR-027-7: Connection timeout after 30 seconds of no packets
- FR-027-8: Handle graceful and abrupt disconnects

---

## 5. Non-Goals

- No Steam integration (use basic UDP)
- No connection encryption beyond GNS built-in
- No WebSocket or HTTP endpoints
- No multiplayer relay (direct UDP only)

---

## 6. Technical Considerations

### Architecture
```
Server Tick Loop (16.67ms)
├── GNSConnectionManager::Process()
│   ├── Accept() → new connections
│   ├── Receive() → packet buffer
│   └── Send() → client responses
└── ZoneServer::tick() → game logic
```

### Integration Points
| Component | Location | Action |
|-----------|----------|--------|
| ServerMain | main.cpp | Create GNSConnectionManager |
| ServerTickSystem | tick.cpp | Call Process() each frame |
| NetworkPacketHandler | handler.cpp | Route packets through GNS |
| ClientSession | session.cpp | Use GNS send/receive |

### Performance Requirements
- Target: 10,000+ concurrent connections
- Latency: <10ms average
- Packet delivery: 99.99%

---

## 7. Success Metrics

- [ ] Server starts with GNS enabled
- [ ] 10,000 connections validated in stress test
- [ ] Connection state machine fully functional
- [ ] Disconnect handling works cleanly
- [ ] No memory leaks after 1 hour runtime

---

## 8. Open Questions

1. Q: Should GNS run in a separate thread?
   A: No, integrate into tick loop for simplicity

2. Q: Enable GNS validation suite?
   A: Yes, add test cases for connection lifecycle

3. Q: Platform-specific configuration?
   A: Default works on Linux; verify macOS/Windows

---

**PRD Status:** Proposed - Ready for Implementation  
**Author:** OpenHands Analysis  
**Filename:** prd-027-gns-runtime-integration.md

---

# PRD-028: Node-Based Combat FSM Integration

**Version:** 2.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical (1B)  
**Requires:** PRD-008 combat FSM template (COMPLETE)

---

## 1. Introduction/Overview

Wire the CombatStateMachine.tscn (created in PRD-008) to the Player.tscn and RemotePlayer.tscn for actual combat gameplay. The FSM template exists but is not integrated into the player controllers.

### Problem Statement
- CombatStateMachine.tscn exists
- CombatStateMachineController.cs exists
- Not wired to Player.tscn input
- Not wired to RemotePlayer.tscn for replay

---

## 2. Goals

- Integrate FSM into Player.tscn with input handling
- Integrate FSM into RemotePlayer.tscn for replicated combat
- Connect attack, block, dodge, and ability inputs
- Enable server-authoritative combat validation

---

## 3. User Stories

### US-028-001: Local Player Combat Input
**Description:** As a player, I want to attack, block, and dodge so that I can engage in combat.

**Acceptance Criteria:**
- [ ] Left-click triggers attack state transition
- [ ] Right-click triggers block state transition
- [ ] Space triggers dodge/roll state transition
- [ ] FSM responds to input within 1 frame

### US-028-002: Combat State Replication
**Description:** As a server, I want to replicate combat states to other players so that they see my actions.

**Acceptance Criteria:**
- [ ] Combat state changes sent to server
- [ ] Server broadcasts to nearby players
- [ ] RemotePlayer.tscn receives and displays state

### US-028-003: Server Combat Validation
**Description:** As a server, I want to validate combat actions so that cheating is prevented.

**Acceptance Criteria:**
- [ ] Attack range validated server-side
- [ ] Cooldown enforcement server-side
- [ ] Invalid attacks rejected with penalty

### US-028-004: Attack Animation Synced to State
**Description:** As a player, I want my attack animations to match my combat state so that combat looks correct.

**Acceptance Criteria:**
- [ ] Attack state triggers attack animation
- [ ] Block state triggers block animation
- [ ] Dodge state triggers dodge animation
- [ ] Recovery prevents input during transition

### US-028-005: Hit Detection
**Description:** As a player, I want my attacks to hit enemies so that combat works.

**Acceptance Criteria:**
- [ ] Hitbox activates during attack state
- [ ] Collision detection with enemies
- [ ] Damage applied on successful hit

---

## 4. Functional Requirements

- FR-028-1: PredictedPlayer.cs connects input to FSM via ProcessInput()
- FR-028-2: RemotePlayer.tscn receives combat state via snapshot
- FR-028-3: Combat state machine transitions: Idle → Attack → Recovery
- FR-028-4: Attack range check (weapon range + hitbox)
- FR-028-5: Cooldown enforcement (attack_speed from stats)
- FR-028-6: Hit detection via Area3D or PhysicsServer query
- FR-028-7: CombatEvent serialized and sent to server

---

## 5. Non-Goals

- No complex combo system (simple chain only)
- No animation blending beyond AnimationTree
- No weapon durability
- NoPvP vs PvE damage separation (deferred)

---

## 6. Technical Considerations

### Integration Architecture
```
PredictedPlayer.cs
├── input_state_ (WASD, mouse)
├── combat_fsm_ (CombatStateMachineController)
└── ProcessInput() → FSM transition

RemotePlayer.tscn
├── replicated_state_ (from server)
├── combat_fsm_ (read-only)
└── UpdateFromSnapshot() → state sync
```

### FSM State Machine
```
Idle ←→ Attack ←→ Recovery ←→ Idle
  ↑                      │
  └────── Block ←────────┘
  ↑                      │
  └────── Dodge ←────────┘
```

### Performance Requirements
- FSM transition in <1ms
- Hit detection in <2ms
- Network sync in <16ms (single tick)

---

## 7. Success Metrics

- [ ] Player.tscn responds to attack input
- [ ] Combat animations play correctly
- [ ] Remote players see combat states
- [ ] Server validates combat
- [ ] Hit detection works

---

## 8. Open Questions

1. Q: Use AnimationTree or custom blend?
   A: Use AnimationTree for simplicity

2. Q: Hit detection method?
   A: Area3D with body_entered signal

3. Q: How to handle prediction?
   A: Optimistic local, correct on server response

---

**PRD Status:** Proposed - Ready for Implementation  
**Filename:** prd-028-combat-fsm-integration.md

---

# PRD-029: Zone Objectives Integration

**Version:** 2.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical (1B)  
**Requires:** PRD-009 zone objectives (COMPLETE)

---

## 1. Introduction/Overview

Integrate ZoneObjectiveComponent and ZoneObjectiveSystem into the server tick loop to track player progress toward zone objectives (kills, collection, exploration).

### Problem Statement
- ZoneObjectiveComponent exists
- ZoneObjectiveSystem exists
- TestZoneObjectives.cpp exists
- Not integrated into tick loop

---

## 2. Goals

- Track player progress on zone objectives
- Award rewards on objective completion
- Handle objective spawning/despawning
- Support multiple objective types

---

## 3. User Stories

### US-029-001: Kill Objective Tracking
**Description:** As a player, I want my kills to count toward my active quest so that I can complete objectives.

**Acceptance Criteria:**
- [ ] Kill increments progress counter
- [ ] Progress stored per player/objective
- [ ] Counter syncs to client UI

### US-029-002: Collect Objective Tracking
**Description:** As a player, I want collected items to count toward my active quest so that I can complete objectives.

**Acceptance Criteria:**
- [ ] Item pickup increments progress
- [ ] Duplicate items handled (stack vs new)
- [ ] Progress syncs to client

### US-029-003: Objective Completion
**Description:** As a player, I want to receive rewards when I complete an objective so that I feel progress.

**Acceptance Criteria:**
- [ ] Objective complete when progress >= target
- [ ] XP awarded to player
- [ ] Item rewards added to inventory
- [ ] Completion message displayed

### US-029-004: Zone Objective Spawning
**Description:** As a server, I want zone objectives to spawn when players enter so that they have goals.

**Acceptance Criteria:**
- [ ] Objectives loaded on zone enter
- [ ] Active objectives assigned to player
- [ ] Progress tracked per player
- [ ] Completion detection works

### US-029-005: Objective Types Support
**Description:** As a designer, I want multiple objective types so that I can create varied content.

**Acceptance Criteria:**
- [ ] KillNPC objective type works
- [ ] CollectItem objective type works
- [ ] TalkToNPC objective type works
- [ ] ExploreZone objective type works

---

## 4. Functional Requirements

- FR-029-1: ZoneObjectiveSystem::Initialize() called on zone load
- FR-029-2: Player entity has ZoneObjectiveComponent for tracking
- FR-029-3: KillEvent increments kill objective progress
- FR-029-4: ItemPickupEvent increments collect objective progress
- FR-029-5: Objective complete triggers reward distribution
- FR-029-6: ZoneObjectiveComponent tracks: objective_id, progress, target, completed

---

## 5. Non-Goals

- No dynamic objectives (static only)
- No daily/weekly reset
- No objective chains (single objective only)
- No raid objectives (deferred)

---

## 6. Technical Considerations

### Data Structures
```cpp
struct ObjectiveProgress {
    uint32_t objective_id;
    uint32_t current_progress;
    uint32_t target_progress;
    bool completed;
    uint64_t completed_at;
};

struct ZoneObjectiveComponent {
    std::vector<ObjectiveProgress> active_objectives;
    std::vector<uint32_t> completed_objectives;
};
```

### Integration Points
| Component | System | Action |
|-----------|--------|--------|
| ZoneServer | tick | Call ZoneObjectiveSystem::Update() |
| KillSystem | event | Increment kill progress |
| InventorySystem | event | Increment collect progress |
| PlayerReward | event | Award XP/items |

### Performance Requirements
- 100 players with objectives: <5ms tick
- Objective check: O(n) where n = active players

---

## 7. Success Metrics

- [ ] Kill objective tracks progress
- [ ] Collect objective tracks progress
- [ ] Rewards awarded on completion
- [ ] UI displays progress
- [ ] All 4 objective types work

---

## 8. Open Questions

1. Q: How many concurrent objectives per player?
   A: 5 active max

2. Q: Objective persistence?
   A: Save to Redis on completion

3. Q: Objective display method?
   A: Add to existing quest UI

---

**PRD Status:** Proposed - Ready for Implementation  
**Filename:** prd-029-zone-objectives-integration.md

---

# PRD-030: Inventory Equipment System - Complete

**Version:** 2.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High (2C)  
**Requires:** Existing inventory foundation (EXISTS)

---

## 1. Introduction/Overview

Complete the inventory/equipment system for full RPG gameplay. The foundation exists but needs integration, item content, and UI polish.

### Problem Statement
- Inventory component exists (24 slots)
- Equipment component exists (8 slots)
- Item definitions exist (52 items in data/items.json)
- Not wired to gameplay

---

## 2. Goals

- Enable equipment slot usage
- Create inventory UI with drag-and-drop
- Build loot drop system
- Add item tooltips

---

## 3. User Stories

### US-030-001: Equipment Slots Work
**Description:** As a player, I want to equip items so that I can improve my character.

**Acceptance Criteria:**
- [ ] Equipment component accepts valid items
- [ ] Invalid items rejected (wrong slot type)
- [ ] Stats applied when equipped
- [ ] Stats removed when unequipped

### US-030-002: Inventory UI Functional
**Description:** As a player, I want to see and manage my inventory so that I can organize my items.

**Acceptance Criteria:**
- [ ] All 24 slots displayed
- [ ] Item icons shown
- [ ] Stack count displayed
- [ ] Empty slot indicator

### US-030-003: Drag and Drop
**Description:** As a player, I want to drag items to equip them so that it's intuitive.

**Acceptance Criteria:**
- [ ] Drag item from inventory
- [ ] Drop on equipment slot
- [ ] Valid drop equips item
- [ ] Invalid drop returns item

### US-030-004: Loot Drops Work
**Description:** As a player, I want to receive loot from enemies so that I can get items.

**Acceptance Criteria:**
- [ ] Enemy death triggers drop table roll
- [ ] Items spawn in world
- [ ] Player pickup adds to inventory
- [ ] Full inventory handling

### US-030-005: Item Tooltips
**Description:** As a player, I want to see item details so that I can make informed decisions.

**Acceptance Criteria:**
- [ ] Hover shows tooltip
- [ ] Shows name, type, rarity
- [ ] Shows stats
- [ ] Shows value

---

## 4. Functional Requirements

- FR-030-1: Equipment::Equip(item_id) validates slot type
- FR-030-2: Inventory::AddItem(item_id, quantity) handles stacking
- FR-030-3: Inventory::RemoveItem(item_id, quantity) removes items
- FR-030-4: Equipment applies stat bonuses to player stats
- FR-030-5: DropTable system rolls for loot
- FR-030-6: WorldItem entity spawns for dropped items
- FR-030-7: Player pickup triggers inventory add

---

## 5. Non-Goals

- No auction house (deferred to economy PRC)
- No item trading between players (deferred)
- No equipment repair
- No item enchantment/transmutation

---

## 6. Technical Considerations

### Equipment Slots
| Slot | Type | Accepted Items |
|------|------|-------------|
| MAIN_HAND | Weapon | sword, axe, bow, staff |
| OFF_HAND | Shield/Weapon | shield, dagger |
| HEAD | Helmet | helmet, hood |
| CHEST | Armor | chest, robe |
| LEGS | Pants | pants, greaves |
| FEET | Boots | boots, shoes |
| ACCESSORY_1 | Ring/Amulet | ring, amulet |
| ACCESSORY_2 | Ring/Amulet | ring, amulet |

### Data/Item Database
- 52 items in data/items.json
- Types: Weapon, Armor, Consumable, Material, Quest
- Rarities: Common, Uncommon, Rare, Epic, Legendary

### Performance Requirements
- Equipment check: O(1)
- Inventory find: O(n) where n = 24 slots
- Loot roll: O(1)

---

## 7. Success Metrics

- [ ] 7+ equipment slots work
- [ ] Items can be equipped/unequipped
- [ ] Inventory UI shows 24 slots
- [ ] Loot drops on enemy death
- [ ] Tooltips display correctly

---

## 8. Open Questions

1. Q: Equipment visual update method?
   A: Swap mesh on equipment change

2. Q: Default inventory on new character?
   A: 1 basic weapon, 1 basic armor set

3. Q: Stack limit per slot?
   A: 99 for consumables, 1 for equipment

---

**PRD Status:** Proposed - Ready for Implementation  
**Filename:** prd-030-inventory-equipment-complete.md

---

# PRD-031: Abilities System - Complete

**Version:** 2.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High (2C)  
**Requires:** Existing ability foundation (EXISTS)

---

## 1. Introduction/Overview

Complete the abilities system for spell/ability gameplay. The foundation exists but needs integration with combat FSM and casting UI.

### Problem Statement
- Ability component exists
- Abilities (8 slots) component exists
- AbilitySystem exists
- 22 abilities in data/abilities.json
- Not integrated with combat

---

## 2. Goals

- Integrate abilities with combat FSM
- Create ability casting UI
- Implement cooldown management
- Add mana/energy system

---

## 3. User Stories

### US-031-001: Ability Casting
**Description:** As a player, I want to cast abilities so that I can use special attacks.

**Acceptance Criteria:**
- [ ] Ability key press triggers cast
- [ ] Valid target required for targeted abilities
- [ ] Mana cost deducted
- [ ] Cooldown starts

### US-031-002: Ability Effects Apply
**Description:** As a player, I want abilities to have effects so that they do something.

**Acceptance Criteria:**
- [ ] Damage abilities apply damage
- [ ] Heal abilities restore health
- [ ] Buff abilities add stats
- [ ] Debuff abilities apply effects

### US-031-003: Cooldown Management
**Description:** As a player, I want cooldowns so that abilities can't be spammed.

**Acceptance Criteria:**
- [ ] Cooldown timer tracks remaining time
- [ ] Ability unusable during cooldown
- [ ] Visual cooldown indicator
- [ ] Cooldown resets on success

### US-031-004: Mana System
**Description:** As a player, I want mana so that I can't cast unlimited abilities.

**Acceptance Criteria:**
- [ ] Mana pool exists
- [ ] Casting costs mana
- [ ] Mana regenerates over time
- [ ] Cannot cast without mana

### US-031-005: Ability UI
**Description:** As a player, I want to see my abilities so that I can use them.

**Acceptance Criteria:**
- [ ] 8 ability slots displayed
- [ ] Hotkey shown
- [ ] Cooldown overlay
- [ ] Mana cost shown

---

## 4. Functional Requirements

- FR-031-1: AbilitySystem::Cast(player_id, ability_id) validates and executes
- FR-031-2: CooldownComponent tracks ability cooldowns
- FR-031-3: ManaComponent tracks mana pool and regen
- FR-031-4: Effect system applies damage/heal/buff/debuff
- FR-031-5: CombatFSM integrates ability state
- FR-031-6: UI receives ability bar updates

---

## 5. Non-Goals

- No ability chains/combos
- No ability points/skill trees
- No passive abilities
- No talent system

---

## 6. Technical Considerations

### Ability Types
| Type | Effect | Target |
|------|--------|-------|
| Damage | Deal damage to target | Self/Enemy |
| Heal | Restore health | Self/Ally |
| Buff | Add stat | Self |
| Debuff | Remove stat | Enemy |
| Status | Apply status | Enemy |

### Ability Definition
```json
{
  "id": 1,
  "name": "Fireball",
  "type": "Damage",
  "cooldown": 3000,
  "manaCost": 20,
  "range": 10.0,
  "effectValue": 50
}
```

### 22 Abilities in data/abilities.json
- Attack abilities: 8 (Fireball, Ice Bolt, etc.)
- Defense abilities: 6 (Shield, Heal, etc.)
- Mobility abilities: 4 (Dash, Teleport, etc.)
- Utility abilities: 4 (Buff, Debuff, etc.)

### Performance Requirements
- Cast check: O(1)
- Cooldown check: O(1) 
- Effect application: O(1)

---

## 7. Success Metrics

- [ ] 8 abilities can be bound to hotkeys
- [ ] Abilities cast on key press
- [ ] Cooldowns display on UI
- [ ] Mana system works
- [ ] 22 abilities functional

---

## 8. Open Questions

1. Q: Default ability bar?
   A: First 8 abilities unlocked

2. Q: Ability learning?
   A: Auto-learn at level-up

3. Q: Targeting method?
   A: Mouse cursor for targeted, self for instant

---

**PRD Status:** Proposed - Ready for Implementation  
**Filename:** prd-031-abilities-complete.md

---

# PRD-032: Quest System - Complete

**Version:** 2.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High (2C)  
**Requires:** Existing quest foundation (EXISTS)

---

## 1. Introduction/Overview

Complete the quest system for player progression. The foundation exists but needs integration with objectives and quest log UI.

### Problem Statement
- QuestDefinition exists
- QuestObjective exists  
- QuestProgress exists
- QuestLog exists (20 active, 100 completed)
- 10 quests in data/quests.json
- Not integrated with gameplay

---

## 2. Goals

- Connect quests to zone objectives
- Create quest log UI
- Implement quest rewards
- Add quest tracking

---

## 3. User Stories

### US-032-001: Quest Display
**Description:** As a player, I want to see available quests so that I can choose what to do.

**Acceptance Criteria:**
- [ ] Quest givers have indicator
- [ ] Quest list displays on interact
- [ ] Quest details shown
- [ ] Accept button works

### US-032-002: Quest Progress
**Description:** As a player, I want to see quest progress so that I know what to do.

**Acceptance Criteria:**
- [ ] Quest log shows active quests
- [ ] Objectives displayed per quest
- [ ] Progress counter shown
- [ ] Update on objective complete

### US-032-003: Quest Completion
**Description:** As a player, I want to complete quests so that I get rewards.

**Acceptance Criteria:**
- [ ] Return to quest giver
- [ ] Completion detected
- [ ] Rewards displayed
- [ ] Rewards claimed

### US-032-004: Quest Rewards
**Description:** As a player, I want to receive rewards so that completing quests is worth it.

**Acceptance Criteria:**
- [ ] XP awarded
- [ ] Items added to inventory
- [ ] Gold added towallet
- [ ] Quest removed from active

### US-032-005: Quest Tracking
**Description:** As a player, I want to track quests so that I don't forget them.

**Acceptance Criteria:**
- [ ] Active quest shown in UI
- [ ] Waypoint marker on map
- [ ] Objective highlighted
- [ ] Easy access to quest log

---

## 4. Functional Requirements

- FR-032-1: QuestAcceptEvent adds quest to QuestLog
- FR-032-2: ZoneObjectiveComponent integrates with quest objectives
- FR-032-3: QuestCompleteEvent validates and rewards
- FR-032-4: QuestComponent tracks player quests
- FR-032-5: QuestLog maintains 20 active, 100 completed

---

## 5. Non-Goals

- No quest chains (single quest only)
- No daily quests
- No raid quests
- No guild quests

---

## 6. Technical Considerations

### Quest Types
| Type | Objective | Example |
|------|----------|---------|
| KillNPC | Kill X enemies | Defeat 10 Goblins |
| CollectItem | Collect X items | Gather 5 Herbs |
| TalkToNPC | Talk to NPC | Speak to Elder |
| ExploreZone | Visit area | Enter Ruins |

### 10 Quests in data/quests.json
1. "First Blood" (KillNPC) - Tutorial combat
2. "Gathering" (CollectItem) - Tutorial resources
3. "The Elder's Request" (TalkToNPC) - Tutorial story
4. "Into the Wilds" (ExploreZone) - Tutorial exploration
5. "Goblin Menace" (KillNPC) - Combat challenge
6. "Herbal Remedy" (CollectItem) - Crafting prep
7. "Lost Artifact" - Multi-objective
8. "Defend the Village" - Wave defense
9. "The Dragon's Lair" - Boss intro
10. "Guild Initiation" - Guild tutorial

### Performance Requirements
- Quest lookup: O(1)
- Progress update: O(1)
- Quest check: O(n) where n = active quests

---

## 7. Success Metrics

- [ ] Quests display from NPCs
- [ ] Quest objectives track progress
- [ ] Quest rewards work
- [ ] Quest log UI functional
- [ ] 10 quests playable

---

## 8. Open Questions

1. Q: Max active quests per player?
   A: 20

2. Q: Quest sharing?
   A: No, individual only

3. Q: Abandom-able quests?
   A: Yes, with penalty

---

**PRD Status:** Proposed - Ready for Implementation  
**Filename:** prd-032-quest-system-complete.md