# PRD-GAP-007: Arena Zone NPC Wave Spawning

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P0 - High  
**Category:** Gameplay - Demo Completion

---

## Introduction

The arena.json zone configuration defines npc_presets (wolves, bandits, archers) AND wave_defense with 3 waves, BUT the NPCs may not spawn at wave times, preventing the wave defense gameplay from functioning.

**Problem:** The wave system exists in config but the NPC spawning system doesn't read and execute wave spawning at the correct intervals.

---

## Goals

- Wave 1 spawns 3 wolves at zone start
- Wave 2 spawns 2 wolves + 2 bandits after Wave 1 clear
- Wave 3 spawns 3 bandits + 2 archers + 3 wolves after Wave 2 clear
- Victory triggers after Wave 3 clear
- Loot chest spawns after victory

---

## User Stories

### US-001: Wave 1 Spawns
**Description:** As a player entering the arena, I want wave 1 enemies to spawn.

**Acceptance Criteria:**
- [ ] 3 wolves spawn at spawn_delay 2.0s after zone entry
- [ ] Wolves spawn at configured locations
- [ ] Wolves have aggressive behavior

### US-002: Wave 2 Spawns
**Description:** As a player, I want wave 2 to spawn after clearing wave 1.

**Acceptance Criteria:**
- [ ] Wave 2 triggers after all wave 1 enemies dead
- [ ] 2 wolves + 2 bandits spawn per config
- [ ] completion_message displays

### US-003: Wave 3 Spawns
**Description:** As a player, I want wave 3 to spawn after clearing wave 2.

**Acceptance Criteria:**
- [ ] Wave 3 triggers after all wave 2 enemies dead
- [ ] 3 bandits + 2 archers + 3 wolves spawn
- [ ] completion_message displays

### US-004: Victory and Loot
**Description:** As a player who cleared all waves, I want loot.

**Acceptance Criteria:**
- [ ] Victory triggers after all Wave 3 enemies dead
- [ ] Loot chest spawns per reward_chest config
- [ ] Gold and items drop per config

---

## Functional Requirements

- FR-1: WaveSystem reads wave_defense from arena.json
- FR-2: spawnWave(wave_number) instantiates NPCs per config
- FR-3: onAllEnemiesDead() triggers next wave
- FR-4: onFinalWaveClear() triggers victory
- FR-5: Loot chest spawns with configured rewards

---

## Technical Considerations

- **Existing Config:**
  - arena.json has "wave_defense" section
  - "waves" array with 3 entries
  - "reward_chest" spawns after wave 3

- **Integration:**
  - WaveSystem: similar to BossPhaseSystem
  - Use existing NPCSpawnSystem
  - Use existing DropsSystem for loot

---

## Success Metrics

- **Functional:**
  - All 3 waves spawn correctly
  - Victory triggers after Wave 3
  - Loot drops correctly

- **Demo Validation:**
  - Zone 99 (Arena) playable start-to-finish