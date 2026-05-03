# PRD: Achievement System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-achievement-system.md`
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Server: AchievementSystem.hpp
Create `src/server/src/progression/AchievementSystem.hpp`:
```cpp
namespace DarkAges::progression {

struct Achievement {
  uint32_t id;
  std::string name;
  std::string description;
  AchievementCategory category; // Combat, Exploration, Crafting, Social
  AchievementCriteria criteria;
  uint32_t points;
};

enum class AchievementCategory { Combat, Exploration, Crafting, Social, Special };

struct AchievementProgress {
  uint64_t player_id;
  uint32_t achievement_id;
  bool unlocked;
  std::chrono::system_clock::time_point unlocked_at;
  uint32_t progress_value;
};

class AchievementSystem {
public:
  void LoadAchievements(); // Load achievement definitions
  void CheckAchievement(uint64_t player_id, AchievementCategory cat, uint32_t criteria_id, uint32_t value);
  void GrantAchievement(uint64_t player_id, uint32_t achievement_id);
  auto GetPlayerAchievements(uint64_t player_id) -> std::vector<AchievementProgress>;
  auto GetLeaderboard(AchievementCategory cat, uint32_t limit) -> std::vector<LeaderboardEntry>;
};

}
```

### Server: AchievementSystem.cpp
- LoadAchievements: from config/JSON
- CheckAchievement: called from various systems (kills, crafting, etc.)
- GrantAchievement: unlock achievement, notify client, add points
- GetLeaderboard: aggregate points across players

### Integration Points
1. **CombatSystem** - On kill: CheckAchievement(Combat, monster_id)
2. **CraftingSystem** - On craft: CheckAchievement(Crafting, recipe_id)
3. **QuestSystem** - On complete: CheckAchievement(Quest, quest_id)
4. **ZoneSystem** - On zone: CheckAchievement(Exploration, zone_id)
5. **GuildSystem** - On create: CheckAchievement(Social, guild)

### Packet Types
- PACKET_ACHIEVEMENT_UNLOCK = 80
- PACKET_ACHIEVEMENT_SYNC = 81

### Client: AchievementPanel.tscn
Create UI:
- Achievement list with categories
- Progress bars for in-progress
- Notification popup on unlock

## Acceptance Criteria
- [ ] Server has AchievementSystem
- [ ] Achievement tracks player progress
- [ ] Unlock notification works
- [ ] Client shows panel with all achievements
- [ ] Points counted for leaderboard