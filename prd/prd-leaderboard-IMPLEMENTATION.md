# PRD: Leaderboard System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-leaderboard-system.md`, `prd/prd-leaderboard-ui.md`
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P3 (Polish)

## Implementation Checklist

### Server: Leaderboard.hpp
Create `src/server/src/progression/Leaderboard.hpp`:
```cpp
namespace DarkAges::progression {

enum class LeaderboardType {
  Level = 0,
  PvPKills = 1,
 Gold = 2,
  Crafting = 3,
  Achievements = 4,
};

struct LeaderboardEntry {
  uint64_t player_id;
  std::string player_name;
  uint32_t rank;
  uint64_t value;
};

class Leaderboard {
public:
  void UpdateValue(LeaderboardType type, uint64_t player_id, uint64_t value);
  auto GetTop(LeaderboardType type, uint32_t limit = 100) -> std::vector<LeaderboardEntry>;
  auto GetPlayerRank(LeaderboardType type, uint64_t player_id) -> std::optional<LeaderboardEntry>;
  void Tick(); // Recalculate ranks periodically
};

}
```

### Server: Leaderboard.cpp
- UpdateValue: called when player stats change
- GetTop: return sorted entries (paginated)
- GetPlayerRank: single player's position
- Tick: recalculate all rankings every 5 minutes

### Packet Types
- PACKET_LEADERBOARD_REQUEST = 120
- PACKET_LEADERBOARD_RESPONSE = 121
- PACKET_LEADERBOARD_UPDATE = 122 // Push update

### Integration Points
- PlayerLevel::UpdateXP: UpdateValue(Level)
- CombatSystem::OnKill: UpdateValue(PvPKills)
- EconomySystem: UpdateValue(Gold)
- CraftingSystem::OnCraft: UpdateValue(Crafting)
- AchievementSystem::Unlock: UpdateValue(Achievements)

### Client: LeaderboardPanel.tscn
Create `src/client/scenes/LeaderboardPanel.tscn`:
```
Control
├── TypeTabs (PvP, Level, Gold, Crafting)
├── RankedList (ListContainer)
│   ├── RankColumn
│   ├── NameColumn  
│   └── ValueColumn
└── PlayerRankHighlight
```

## Acceptance Criteria
- [ ] Leaderboard tracks multiple types
- [ ] Top players displayed
- [ ] Player can see their rank
- [ ] Updates periodically