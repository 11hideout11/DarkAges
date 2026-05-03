# PRD-GAP-008: Tutorial Zone NPC Spawning

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** ✅ Complete — tutorial.json has npc_presets with training_dummy and trainer NPCs
**Priority:** P0 - High  
**Category:** Gameplay - Demo Completion

---

## Introduction

The tutorial.json zone configuration defines npc_presets (training_dummy, rabbit) and tutorial_quest, BUT:
1. Training dummies may not spawn or are immortal (correct)
2. Rabbits (passive mobs) may not spawn
3. The tutorial quest with objectives may not activate

**Problem:** Tutorial is the first zone players experience - it must work for good first impressions.

---

## Goals

- Training dummies spawn (immortal, for practice)
- Rabbits spawn (passive, for ambiance)
- Tutorial quest auto-starts on zone entry
- Objectives track player actions

---

## User Stories

### US-001: Training Dummies Spawn
**Description:** As a new player, I want training dummies to spawn so I can practice.

**Acceptance Criteria:**
- [ ] 3 training dummies spawn at radius 10
- [ ] Dummies are immortal (immortal: true in config)
- [ ] Dummies take damage but don't die
- [ ] Dummies have combat feedback (particles, sound)

### US-002: Ambient Mobs Spawn
**Description:** As a new player, I want to see ambient wildlife.

**Acceptance Criteria:**
- [ ] 5 rabbits spawn at radius 20
- [ ] Rabbits have passive behavior
- [ ] Rabbits flee when player gets close
- [ ] Rabbits respawn after leaving area

### US-003: Tutorial Quest Activates
**Description:** As a new player, I want the tutorial to guide me.

**Acceptance Criteria:**
- [ ] Quest "First Steps" auto-starts on zone entry
- [ ] Objective: "Move using WASD" tracks movement
- [ ] Objective: "Look around using mouse" tracks camera
- [ ] Objective: "Attack the dummy" tracks damage
- [ ] Rewards grant on objective completion

---

## Technical Considerations

- **Existing Config:**
  - tutorial.json has "npc_presets"
  - "tutorial_quest" with auto_start: true
  - "immortal": true for dummies
  - "passive": true for rabbits

- **Integration:**
  - NPCSpawnSystem::spawnZoneNPCs()
  - ZoneObjectiveSystem::assignQuest()
  - Tutorial objectives use custom tracking

---

## Success Metrics

- **Functional:**
  - All NPCs spawn on zone entry
  - Tutorial quest shows in UI
  - Movement/combat tracking works

- **UX:**
  - Player learns controls in < 2 minutes
  - Clear feedback on actions