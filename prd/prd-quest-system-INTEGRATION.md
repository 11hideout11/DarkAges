# PRD: Quest System Integration - Implementation

## Status
**DATA EXISTS - NEEDS INTEGRATION**

## Gap Analysis
- **PRD File**: `prd/prd-quest-system.md`
- **Data File**: `data/quests.json` EXISTS with quest data
- **Server Code**: PARTIAL - basic quest data loaded, not wired
- **Client Code**: PARTIAL - QuestTracker.tscn exists
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Server: QuestSystem Integration
Modify/create `src/server/src/content/QuestSystem.hpp`:
```cpp
namespace DarkAges::content {

struct QuestProgress {
  uint64_t player_id;
  uint32_t quest_id;
  QuestStatus status; // NotStarted, InProgress, Completed, Failed
  std::vector<uint32_t> objectives_completed;
  std::chrono::system_clock::time_point started_at;
  std::chrono::system_clock::time_point completed_at;
};

class QuestSystem {
public:
  void LoadQuests(); // Load from data/quests.json
  void AssignQuest(uint64_t player_id, uint32_t quest_id);
  void UpdateObjective(uint64_t player_id, uint32_t quest_id, uint32_t objective_id);
  void CompleteQuest(uint64_t player_id, uint32_t quest_id);
  auto GetQuestProgress(uint64_t player_id, uint32_t quest_id) -> std::optional<QuestProgress>;
  auto GetActiveQuests(uint64_t player_id) -> std::vector<QuestProgress>;
  bool IsQuestComplete(uint32_t quest_id, const QuestProgress& progress);
};

}
```

### Integration Points Required
1. **CombatSystem** - On kill: check quest objective, call QuestSystem::UpdateObjective
2. **InteractionSystem** - On NPC interaction: check available quests, offer assignment
3. **ItemSystem** - On item pickup: check item collection objectives
4. **ZoneSystem** - On zone enter: check zone-specific quests

### Packet Types
- PACKET_QUEST_OFFER = 60  // Server offers quest to player
- PACKET_QUEST_ACCEPT = 61 // Player accepts quest
- PACKET_QUEST_COMPLETE = 62 // Quest completed, reward given
- PACKET_QUEST_UPDATE = 63 // Objective progress update
- PACKET_QUEST_SYNC = 64 // Full quest state on login

### Client: QuestTracker Enhancement
Existing `src/client/scenes/QuestTracker.tscn`:
- Wire up PACKET_QUEST_* packet handlers
- Display quest objectives with progress
- Quest offer dialog on interaction

## Acceptance Criteria - IMPLEMENTATION VERIFIED

- [ ] Quests load from `data/quests.json` on server start
- [ ] Quest offered when player talks to NPC
- [ ] Kill enemies update quest progress
- [ ] Quest completion triggers reward
- [ ] Client shows quest in QuestTracker
- [ ] Integration test: complete a quest

## Data Already Available
```json
// data/quests.json has structure:
{
  "quests": [
    {
      "id": 1,
      "title": "...",
      "description": "...",
      "objectives": [...],
      "rewards": {...}
    }
  ]
}
```

## Technical Notes
- Extend existing QuestData struct in Protocol.cpp
- Reuse RewardComponent from items.json
- Test with existing tutorial/arena quests