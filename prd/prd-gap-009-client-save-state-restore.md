# PRD-GAP-009: Client Save/Load Runtime State Restore

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P1 - Medium  
**Category:** Persistence - Client

---

## Introduction

SaveManager.cs (line 230) has a TODO: "Restore player state to GameState once LocalPlayer property exists". The save/load system correctly serializes/deserializes JSON to disk, but the runtime GameState is NOT updated after loading, so players spawn at default location instead of their saved position.

**Problem:** LoadGame() parses the JSON but doesn't call GameState to restore entity position, level, gold, inventory, etc. This makes save/load useless - player always starts fresh.

---

## Goals

- After LoadGame(), player spawns at saved position
- Player level, gold, XP restore correctly
- Inventory items restore to player
- Equipped gear restores to slots
- Quest progress restores

---

## User Stories

### US-001: Position Restore
**Description:** As a player who saved my game, I want to spawn at my saved location.

**Acceptance Criteria:**
- [ ] LocalEntity position sets from saveData.player.posX/Y/Z
- [ ] Zone changes to saved zoneId
- [ ] Player position updates on scene load

### US-002: Stats Restore
**Description:** As a player, I want my stats restored.

**Acceptance Criteria:**
- [ ] Level restores from save
- [ ] Experience restores from save
- [ ] Gold restores from save

### US-003: Inventory Restore
**Description:** As a player, I want my items restored.

**Acceptance Criteria:**
- [ ] Inventory slots populate from save
- [ ] Item quantities correct
- [ ] Equipment slots populate

### US-004: Quest Progress Restore
**Description:** As a player, I want my quest progress restored.

**Acceptance Criteria:**
- [ ] Quest states restore (active/complete)
- [ ] Objective progress counts restore
- [ ] Quest rewards available

---

## Functional Requirements

- FR-1: LoadGame() calls GameState.RestorePlayerState(saveData)
- FR-2: GameState has RestorePlayerState() method
- FR-3: Position sets via EntityComponent
- FR-4: Inventory syncs via InventoryComponent
- FR-5: Quests sync via QuestComponent

---

## Technical Considerations

- **Existing Code:**
  - SaveManager.cs - serializes correctly
  - SaveData structure - has all fields
  - GameState.Instance - holds entities

- **Data Flow:**
  ```
  LoadGame() → Parse JSON
  → RestorePlayerState(saveData)
  → EntityComponent.SetPosition(x,y,z)
  → InventoryComponent.Restore(slots)
  → QuestComponent.Restore(quests)
  → Spawn at position
  ```

---

## Success Metrics

- Load game → player at saved position
- Stats match saved values
- Inventory populated correctly
- No crash on load