# PRD-113: Client UI Integration - Core Systems

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** PRD-021 Inventory, PRD-022 Abilities, PRD-025 Quest (server components exist)

---

## Introduction

The server components for RPG systems are implemented (Inventory, Abilities, Quests, Party, Guild), but the client UI panels are not integrated with these systems. The player cannot see or interact with their inventory, abilities, or quest log in-game.

## Goals

- Integrate Inventory UI panel with server data
- Integrate Abilities UI panel with server data
- Integrate Quest Tracker with server data
- Enable equipment slot interaction
- Display mana/health bars

## User Stories

### US-001: Inventory Panel Integration
**Description:** As a player, I want to see my inventory items in a UI panel so that I can manage my items.

**Acceptance Criteria:**
- [ ] InventoryPanel.tscn displays item grid
- [ ] Items loaded from server sync
- [ ] Item icons displayed (placeholder texture)
- [ ] Item count shown for stackable

### US-002: Equipment Slots
**Description:** As a player, I want to see my equipped items so that I know what I'm wearing.

**Acceptance Criteria:**
- [ ] EquipmentPanel shows 8 slots
- [ ] Equipped items displayed
- [ ] Slot icons: head, chest, legs, main_hand, off_hand, feet, ring, amulet
- [ ] Visual feedback on hover

### US-003: Abilities Panel
**Description:** As a player, I want to see my ability icons so that I can use them in combat.

**Acceptance Criteria:**
- [ ] AbilitiesPanel shows 8 ability slots
- [ ] Cooldown overlay visible on cooldown
- [ ] Mana cost displayed
- [ ] Click to cast (if ready)

### US-004: Quest Tracker
**Description:** As a player, I want to see my active quests so that I know what to do.

**Acceptance Criteria:**
- [ ] QuestTracker shows active quests
- [ ] Objective progress displayed
- [ ] Quest rewards shown
- [ ] Click to show details

### US-005: HUD Integration
**Description:** As a player, I want to see my health/mana so that I know my status.

**Acceptance Criteria:**
- [ ] Health bar updates from sync
- [ ] Mana bar updates from sync
- [ ] XP bar displayed
- [ ] Level indicator shown

## Functional Requirements

- FR-1: InventoryPanel.tscn must display 24-slot grid populated from server sync
- FR-2: EquipmentPanel must show 8 equipment slots with icons
- FR-3: AbilitiesPanel must show 8 ability slots with cooldowns
- FR-4: QuestTracker must display up to 20 active quests with progress
- FR-5: HUD must update health/mana/XP from server snapshots
- FR-6: UI must respond to server sync events (on InventoryChanged, OnAbilitiesChanged)

## Non-Goals

- Drag-and-drop inventory (deferred to Phase 2)
- Item tooltips detail (deferred)
- Ability tooltips (deferred)
- Guild/Party UI panels (deferred to Phase 2)
- Chat panel integration (already exists)

## Technical Considerations

- Integration: UI listens to sync events from NetworkManager
- Data flow: Server snapshots → Client UI update
- Panel scenes exist: scenes/ui/InventoryPanel.tscn (new), scenes/ui/QuestTracker.tscn (exists)
- Godot 4.2 signal handling: Connect to sync signals

## Success Metrics

- All 4 UI panels functional
- Data sync from server: <100ms latency
- No UI state desync
- 60fps maintained with UI

## Open Questions

1. **Q: Use Godot signals or custom event system?**
   - A: Use existing RemoteSceneTree multiplayer sync

2. **Q: Localized text or keys?**
   - A: Use string IDs (for localization later)

3. **Q: UI scale/platform support?**
   - A: Anchor UI to viewport edges

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Gap Analysis  
**Next Step:** Create InventoryPanel and wire to sync