# PRD: Quest System Completion

## Introduction

The DarkAges client has a QuestTracker UI component but lacks server-side quest implementation, quest definitions, and proper quest progress tracking. This PRD completes the quest system for RPG progression.

## Goals

- Server-side quest definitions
- Quest objective tracking
- Quest completion rewards
- Quest UI synchronization

## User Stories

### QST-001: Quest Acceptance
**Description:** As a player, I want to accept quests from NPCs.

**Acceptance Criteria:**
- [ ] Quest offered via dialogue
- [ ] Accept/decline options
- [ ] Quest added to tracker
- [ ] Objectives displayed

### QST-002: Quest Progress
**Description:** As a player, I want progress tracked automatically.

**Acceptance Criteria:**
- [ ] Kill objective counts
- [ ] Collection objective tracks
- [ ] Location objective tracks
- [ ] Progress updates in real-time

### QST-003: Quest Completion
**Description:** As a player, I want rewards when completing quests.

**Acceptance Criteria:**
- [ ] XP reward applied
- [ ] Item reward given
- [ ] Quest removed from tracker
- [ ] Completion dialogue shown

## Functional Requirements

- FR-1: QuestDefinition struct with objectives
- FR-2: QuestProgress component on entities
- FR-3: QuestEventPacket for updates
- FR-4: Integration with DialogueSystem

## Non-Goals

- No quest chains
- No daily quests
- No random/generated quests

## Technical Considerations

### Quest Definition
```cpp
struct QuestDefinition {
    uint32_t questId;
    std::string title;
    std::string description;
    std::vector<QuestObjective> objectives;
    uint32_t xpReward;
    std::vector<ItemReward> itemRewards;
};

enum class QuestObjectiveType { Kill, Collect, Talk, Location };
```

### Network Protocol
- PACKET_QUEST_OFFER (type=40)
- PACKET_QUEST_ACCEPT (type=41)
- PACKET_QUEST_PROGRESS (type=42)
- PACKET_QUEST_COMPLETE (type=43)

## Success Metrics

- Quests displayed in tracker
- Killing NPCs updates quest progress
- Completion grants XP

## Open Questions

- Default starting quests?
- Quest difficulty scaling?