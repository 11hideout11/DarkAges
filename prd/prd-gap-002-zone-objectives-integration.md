# PRD-GAP-002: Zone Objectives Integration

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P0 - High  
**Category:** Gameplay - Quest System

---

## Introduction

The ZoneObjectiveSystem exists in the codebase (ZoneObjectiveComponent.hpp, ZoneObjectiveSystem.hpp/cpp) and is wired to ZoneServer, BUT the zone JSON configuration files (tutorial.json, arena.json, boss.json) don't define objectives in a format that the system can load. This means players don't receive quest objectives when entering zones.

**Problem:** The code exists but the data format doesn't match. The zone configs have "tutorial_quest", "arena_quest", "boss_quest" sections but not "objectives" array that ZoneObjectiveSystem expects.

---

## Goals

- Zone configs define objectives in system-readable format
- Objectives appear in player UI when entering a zone
- Objective progress tracks (kill count, location reached, etc.)
- Objective completion triggers rewards
- Zone objective client replication works (PACKET_ZONE_OBJECTIVE_UPDATE)

---

## User Stories

### US-001: Zone Config Defines Objectives
**Description:** As a game designer, I want to define objectives in zone JSON so they load into the game.

**Acceptance Criteria:**
- [ ] tutorial.json defines tutorial_movement objectives in "objectives" array
- [ ] arena.json defines arena_champion objectives in "objectives" array  
- [ ] boss.json defines slay_the_chieftain objectives in "objectives" array
- [ ] Objectives parse into ZoneObjectiveComponent on zone load

### US-002: Objectives Display in Client UI
**Description:** As a player, I want to see my current objectives so I know what to do.

**Acceptance Criteria:**
- [ ] QuestTracker.tscn displays active objectives
- [ ] Objective title shows: "Slay the Chieftain"
- [ ] Objective progress shows: "Defeat Gruk: 0/1"
- [ ] Objectives update in real-time as player progresses

### US-003: Objective Progress Tracking
**Description:** As a player, I want my progress to be tracked so I know when I'm done.

**Acceptance Criteria:**
- [ ] Kill objective tracks enemy deaths (type: "kill", target: "boss")
- [ ] Location objective tracks area entry (type: "reach", target: "boss_arena")
- [ ] Custom objective tracks player actions (type: "action", action: "interact")
- [ ] Progress syncs to client via PACKET_ZONE_OBJECTIVE_UPDATE

### US-004: Objective Completion and Rewards
**Description:** As a player who completed objectives, I want to receive rewards.

**Acceptance Criteria:**
- [ ] XP reward grants on objective completion
- [ ] Gold reward grants on objective completion
- [ ] Item rewards grant on objective completion
- [ ] Title grants on quest completion (e.g., "Chieftain Slayer")

---

## Functional Requirements

- FR-1: ZoneConfigParser must parse "objectives" array from JSON
- FR-2: ZoneObjectiveComponent stores array of objectives per player
- FR-3: ZONE_ENTER handler assigns objectives to player
- FR-4: Kill tracking, location tracking, action tracking handlers update progress
- FR-5: onObjectiveComplete() triggers reward distribution
- FR-6: NetworkManager sends PACKET_ZONE_OBJECTIVE_UPDATE to client
- FR-7: QuestTracker.tscn subscribes to objective update signals

---

## Non-Goals

- Daily/weekly quests - deferred
- Quest givers (NPC dialogue) - use existing system
- Quest chains - deferred
- Shared quest progress (party) - deferred
- Quest rewards scaling - deferred

---

## Technical Considerations

- **Existing Code:**
  - ZoneObjectiveComponent.hpp - exists
  - ZoneObjectiveSystem.hpp/cpp - exists  
  - PACKET_ZONE_OBJECTIVE_UPDATE - exists in Protocol.cpp
  - QuestTracker.tscn - exists in client
  - NetworkManager::ProcessZoneObjectiveUpdate - exists

- **Data Format:**
  ```json
  "objectives": [
    {
      "id": "defeat_boss",
      "type": "kill",
      "target": "boss",
      "count": 1,
      "description": "Defeat Gruk The Unstoppable"
    },
    {
      "id": "reach_arena",
      "type": "reach", 
      "target": "boss_arena_center",
      "description": "Enter the boss arena"
    }
  ]
  ```

- **Integration Points:**
  - ZoneServer::loadZone() → parse objectives array
  - ZoneObjectiveSystem::assignObjectives() → creates player component
  - CombatSystem::onEntityKilled() → check and update kill objectives
  - PositionComponent::onEnterArea() → check location objectives
  - NetworkManager → sync to client

---

## Success Metrics

- **Functional:**
  - All 3 demo zones load objectives on entry
  - Progress tracks correctly for each objective type
  - Rewards distribute on completion

- **Demo Validation:**
  - Player sees objectives in UI in all zones
  - Objective progress updates as player plays
  - Rewards grant on completion

---

## Open Questions

- Q: Should objectives auto-start or require NPC interaction?
  - A: Auto-start for MVP (current boss_quest has auto_start: true)

- Q: What if player dies during objective?
  - A: Progress persists, no rollback for MVP

- Q: Can objectives be shared in parties?
  - A: Individual progress for MVP (defer party to later)