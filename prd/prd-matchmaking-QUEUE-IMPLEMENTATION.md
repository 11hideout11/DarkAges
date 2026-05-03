# PRD: Matchmaking Queue - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-matchmaking-queue.md`
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Server: MatchmakingSystem.hpp
Create `src/server/src/matchmaking/MatchmakingSystem.hpp`:
```cpp
namespace DarkAges::matchmaking {

enum class QueueType {
  PvP_Arena = 0,
  PvP_BattleRoyale = 1,
  PvE_Dungeon = 2,
};

struct QueueEntry {
  uint64_t player_id;
  QueueType type;
  uint32_t MMR; // Matchmaking rating
  uint32_t wait_time_seconds;
  std::chrono::system_clock::time_point joined_at;
};

struct MatchResult {
  std::vector<uint64_t> team1_players;
  std::vector<uint64_t> team2_players;
  uint32_t zone_id; // Assigned zone for match
};

class MatchmakingSystem {
public:
  void JoinQueue(uint64_t player_id, QueueType type);
  void LeaveQueue(uint64_t player_id);
  auto GetQueueStatus(uint64_t player_id) -> std::optional<QueueEntry>;
  void Tick(); // Process queue, find matches
  void CancelMatch(uint64_t player_id); // Cancel after found
};

}
```

### Server: MatchmakingSystem.cpp
- JoinQueue: add player to queue for type
- LeaveQueue: remove from queue
- Tick: every second, check for matchable groups
  - Match by MMR range (+/- 100)
  - Create match when teams balanced
  - Assign zone, notify players
- CancelMatch: decline match, penalty wait

### Packet Types
- PACKET_QUEUE_JOIN = 110
- PACKET_QUEUE_LEAVE = 111
- PACKET_QUEUE_STATUS = 112
- PACKET_QUEUE_MATCH_FOUND = 113

### Client: QueueUI.tscn
Create `src/client/scenes/QueueUI.tscn`:
- Queue type selection
- Estimated wait time
- Cancel button
- Match found dialog

## Acceptance Criteria
- [ ] Join queue for PvP
- [ ] Wait time displayed
- [ ] Match found notification
- [ ] Teleport to arena on match start