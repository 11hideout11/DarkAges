# PRD-GAP-013: NPC Dialogue System Wiring

**Version:** 1.0
**Date:** 2026-05-03
**Status:** Proposed
**Priority:** P1 - High
**Category:** Gameplay - Quest System

---

## Introduction

The NPC Dialogue System PRD exists (prd-npc-dialogue-system.md), AND dialogues.json has dialogue data, BUT dialogue may not trigger from NPC interaction or properly hand off quests to players.

**Problem:** dialogues.json config exists, DialoguePanel.tscn exists, but clicking NPCs may not open dialogue or may not grant quests.

---

## Goals

- Clicking NPCs opens dialogue panel with correct lines
- Dialogue advances on player selection
- Quest handoff triggers on dialogue completion
- Quest objectives update on grant

---

## User Stories

### US-001: NPC Interaction Opens Dialogue
**Description:** As a player, I want to talk to NPCs to receive quests.

**Acceptance Criteria:**
- [ ] Clicking NPC opens DialoguePanel.tscn
- [ ] First dialogue line displays from dialogues.json
- [ ] NPC name shows in panel header

### US-002: Dialogue Advances
**Description:** As a player, I want to advance through dialogue.

**Acceptance Criteria:**
- [ ] Clicking response option advances dialogue
- [ ] Next line displays in panel
- [ ] "End Dialogue" option at conversation end

### US-003: Quest Grant
**Description:** As a player, I want to receive quests from dialogue.

**Acceptance Criteria:**
- [ ] Quest grants on dialogue node with quest_reward
- [ ] Quest appears in QuestTracker.tscn
- [ ] Quest objectives display in UI

### US-004: Dialogue Ends
**Description:** As a player, I want to close dialogue.

**Acceptance Criteria:**
- [ ] Escape key closes dialogue
- [ ] Clicking outside closes dialogue
- [ ] Panel fades out on close

---

## Functional Requirements

- FR-1: Player input triggers DialoguePanel show
- FR-2: dialogues.json parsed by DialogueSystem
- FR-3: QuestManager.AddQuest called on quest node
- FR-4: QuestTracker subscribes to quest signals
- FR-5: DialoguePanel hides on close

---

## Technical Considerations

- **Existing Code:**
  - `prd-npc-dialogue-system.md` - exists
  - `dialogues.json` - has data
  - `DialoguePanel.tscn` - exists
  - `QuestTracker.tscn` - exists

- **Integration Points:**
  - PredictedPlayer click → NetworkManager
  → DialogueSystem → DialoguePanel show
  → QuestManager.AddQuest()

---

## Success Metrics

- NPCs in tutorial.json open dialogue
- Quests grant on dialogue completion
- No duplicate quest grants

---

## Open Questions

- Q: Branching dialogue or linear?
  - A: Simple linear for MVP (future: branching)