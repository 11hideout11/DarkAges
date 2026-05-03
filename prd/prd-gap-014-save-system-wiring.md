# PRD-GAP-014: Save System Wiring

**Version:** 1.0
**Date:** 2026-05-03
**Status:** Proposed
**Priority:** P1 - High
**Category:** Persistence - Player Data

---

## Introduction

The Save/Load system PRD exists (prd-save-load-system.md), AND SaveManager.cs exists in client, BUT player progress may not persist correctly between sessions.

**Problem:** SaveManager saves UI state, but character data (level, gold, inventory, quest progress) may not save/load correctly.

---

## Goals

- Character stats persist to disk on save
- Inventory persists between sessions
- Quest progress persists
- Save triggers on quit, death, or manual save
- Load restores complete state

---

## User Stories

### US-001: Auto-Save on Quit
**Description:** As a player, I want my progress saved when I quit.

**Acceptance Criteria:**
- [ ] Quit saves to disk automatically
- [ ] Save confirms with "Game Saved" toast
- [ ] Save completes in < 2 seconds

### US-002: Save on Death  
**Description:** As a player, I want my progress saved before respawn.

**Acceptance Criteria:**
- [ ] Death triggers save before respawn
- [ ] Save includes last checkpoint position
- [ ] No progress loss on death

### US-003: Manual Save
**Description:** As a player, I want to save manually.

**Acceptance Criteria:**
- [ ] Ctrl+S triggers manual save
- [ ] Keybind configurable in settings
- [ ] "Game Saved" confirms

### US-004: Load Restores State
**Description:** As a player, I want to resume my progress.

**Acceptance Criteria:**
- [ ] Load finds latest save file
- [ ] Character stats restore (HP, gold, level)
- [ ] Inventory restores from save
- [ ] Quest progress restores

---

## Functional Requirements

- FR-1: SaveManager serializes Player data to JSON
- FR-2: CharacterComponent exports to save format
- FR-3: InventoryComponent exports to save format
- FR-4: QuestComponent exports progress
- FR-5: LoadManager parses save file

---

## Technical Considerations

- **Existing Code:**
  - `prd-save-load-system.md` - exists
  - `SaveManager.cs` - exists
  - `CharacterComponent` - exists
  - `InventoryComponent` - exists

- **Integration Points:**
  - Main _ExitTree() → SaveManager.Save()
  - DeathState → SaveManager.Save()
  - Main _Ready() → LoadManager.Load()

---

## Success Metrics

- Save completes in < 2 seconds
- Load restores all player state
- No duplicate saves or corruption

---

## Open Questions

- Q: Local or cloud save?
  - A: Local for MVP, cloud later