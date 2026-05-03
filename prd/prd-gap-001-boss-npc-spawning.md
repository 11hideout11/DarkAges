# PRD-GAP-001: Boss Zone NPC Spawning

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P0 - Critical  
**Category:** Gameplay - Demo Completion

---

## Introduction

The boss.json zone configuration has a comprehensive boss encounter defined (Gruk The Unstoppable with 4 phases, minions, abilities) but the NPCs (boss and minions) are not spawning physically in the game. This prevents the boss zone from being playable in the demo.

**Problem:** The boss_encounter section in boss.json defines all the data (boss entity, phases, abilities, minions) but the NPC spawning system doesn't read this configuration and instantiate the boss entity in the world.

---

## Goals

- Boss entity (Gruk The Unstoppable) spawns at the correct spawn point when players enter zone 100
- Minion NPCs spawn dynamically during phase transitions per boss.json config
- Boss AI begins executing after spawn (chase, attack, abilities)
- Boss health tracks correctly and phase transitions trigger at correct thresholds (100%, 66%, 33%, 10%)
- Victory condition triggers when boss HP reaches 0
- Loot chest spawns with configuredRewards after boss defeat

---

## User Stories

### US-001: Boss Entity Spawns on Zone Entry
**Description:** As a player entering the Boss Lair zone, I want the boss to spawn in the arena so that I can fight it.

**Acceptance Criteria:**
- [ ] Boss entity "Gruk The Unstoppable" spawns at spawn_point type "boss_arena_center" (0, 0, 0)
- [ ] Boss spawns with correct initial stats: level 10, health 500, damage 25, armor 15
- [ ] Boss uses archetype "ogre_chieftain" from NPC template
- [ ] Boss spawns with "boss" behavior mode

### US-002: Phase Transitions Trigger Correctly
**Description:** As a player damaging the boss, I want phase transitions to trigger at correct health thresholds so that the encounter is dynamic.

**Acceptance Criteria:**
- [ ] Phase 2 triggers when boss health drops to 66% (health ≤ 330)
- [ ] Phase 3 triggers when boss health drops to 33% (health ≤ 165)
- [ ] Phase 4 triggers when boss health drops to 10% (health ≤ 50)
- [ ] Dialogue displays at each phase transition from boss.json "dialogue" field
- [ ] Abilities change per phase from boss.json "abilities" array

### US-003: Minions Spawn During Phase Transitions
**Description:** As a player in boss combat, I want minions to spawn during specific phases so that the fight escalates.

**Acceptance Criteria:**
- [ ] Phase 2 spawns 4 wolves at "minion_spawn_left" location (-8, 0, -5)
- [ ] Phase 3 spawns 3 bandits at "minion_spawn_right" location (8, 0, -5)
- [ ] Minions have correct archetype stats from zone config
- [ ] Minions have "aggressive" behavior toward player

### US-004: Boss Abilities Execute
**Description:** As the boss AI, I want to use abilities so that the encounter is challenging.

**Acceptance Criteria:**
- [ ] basic_melee ability fires every 2.0s cooldown when player in range 3.0
- [ ] ground_slam ability fires every 8.0s cooldown, causes knockback
- [ ] war_stomp ability fires every 10.0s cooldown, causes 2.0s stun
- [ ] blood_rage buff activates in phase 3, increases damage by 1.5x for 15s
- [ ] Abilities show telegraph warnings before execution

### US-005: Victory and Loot
**Description:** As a player who defeated the boss, I want to receive loot so that my effort is rewarded.

**Acceptance Criteria:**
- [ ] Victory triggers when boss HP reaches 0
- [ ] Victory dialogue displays: "Nooooo! I am... defeated?"
- [ ] Loot chest spawns at configured location (0, 0, -8)
- [ ] Gold drops within range 500-1000
- [ ] Legendary item "Ogre Slayer" (100% chance) drops
- [ ] Epic items drop per configured probabilities

---

## Functional Requirements

- FR-1: ZoneServer must read boss_encounter from boss.json on zone load
- FR-2: spawnBossEntity() must instantiate boss at correct spawn_point
- FR-3: Boss health component must track HP and fire PHASE_TRANSITION event at thresholds
- FR-4: PHASE_TRANSITION handler must spawn minions from spawn_minions config
- FR-5: NPCAISystem must execute boss ability cooldowns and trigger effects
- FR-6: Boss death handler must trigger loot chest spawn
- FR-7: Loot drop system must roll per configured probabilities

---

## Non-Goals

- Boss cinematics (cutscenes, camera movements) - deferred to later phase
- Boss dialogue subtitles in UI - use existing message system
- Boss entry cinematics - use existing event system
- Enrage timer - not in current boss.json config
- Raid scaling (more players = more boss HP) - deferred

---

## Technical Considerations

- **Existing Code:**
  - `ZoneServer::loadZone()` already parses JSON
  - `ZoneObjectiveComponent.hpp` - tracks objectives
  - `ZoneObjectiveSystem.hpp/cpp` - objective logic
  - `NPCAISystem.hpp` - exists as stub, needs implementation

- **Integration Points:**
  - ZoneServer spawns NPCs via NPCSpawnSystem
  - Boss uses existing CombatComponent for health/damage
  - Abilities integrate with AbilitySystem
  - Loot uses existing DropsSystem

- **Data Flow:**
  ```
  boss.json → ZoneServer::loadZone() → parse boss_encounter
  → ZoneServer::spawnBossEntity() → NPCSpawnSystem::spawn()
  → NPCAISystem::setState(Attack) → execute abilities
  → CombatComponent::onHealthChanged() → check thresholds
  → spawnMinions() → NPCSpawnSystem::spawn() x count
  → onDeath() → DropsSystem::drop() → spawn loot chest
  ```

---

## Success Metrics

- **Functional:**
  - Boss spawns within 2s of player entering boss arena
  - All 4 phase transitions trigger correctly
  - Minions spawn at correct phase transitions
  - Victory triggers on boss death
  - Loot drops per configuration

- **Performance:**
  - Boss AI update < 1ms per frame
  - Minion spawn < 100ms per spawn batch
  - No frame drops during phase transitions

- **Demo Validation:**
  - Zone 100 (Boss Lair) is playable start-to-finish
  - Player can defeat Gruk The Unstoppable
  - Player receives loot rewards

---

## Open Questions

- Q: Should boss spawn immediately or after a delay/cutscene?
  - A: Spawn immediately for MVP, cinematic later

- Q: What happens if all players leave during combat?
  - A: Boss resets after 60s timeout (defer to later)

- Q: Can boss be pulled/stacked with other zones?
  - A: Single boss instance per zone for MVP