# PRD: Production Database Integration

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Infrastructure - Data Persistence

---

## 1. Problem Statement

The server uses in-memory data structures, but persistent player data (inventory, quests, achievements) is not saved. For a production MMO, player progress must persist across server restarts and sessions.

### Current State
- ⚠️ In-memory player data only
- ⚠️ docker-compose.dev.yml exists with Redis + Scylla config
- ⚠️ No DB client integration code
- ⚠️ No persistence layer

### Impact
- Player progress lost on restart
- No cross-server state sharing
- Cannot scale horizontally
- Missing production requirement

---

## 2. Goals

### Primary Goals
1. Connect to Redis for session/cache
2. Connect to Scylla for persistent data
3. Implement player data serialization
4. Create auto-save system

### Success Criteria
- [ ] Redis connection established
- [ ] Scylla connection established
- [ ] Player data persists across restarts
- [ ] Auto-save triggers on key events

---

## 3. Technical Specification

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Game Server                          │
├─────────────────────────────────────────────────────────────┤
│  PlayerComponent → PersistenceManager → Redis (cache)      │
│                            ↓                            │
│                      ScyllaDB (persistent)               │
└─────────────────────────────────────────────────────────────┘
```

### Redis Data (Session/Cache)

| Key Pattern | Data | TTL |
|-----------|------|-----|
| session:{playerId} | Session data, position | 60 min |
| global:leaderboard | Sorted set | None |
| rate_limit:{playerId} | Request counts | 60 sec |
| matchmaker:queue | Queue of players | 5 min |

### Scylla Tables (Persistent)

```sql
-- Player data
CREATE TABLE players (
    id uuid PRIMARY KEY,
    username text,
    level int,
    exp int,
    gold int,
    create_time timestamp,
    last_login timestamp
);

-- Player inventory  
CREATE TABLE inventory (
    player_id uuid,
    slot_id int,
    item_id int,
    quantity int,
    PRIMARY KEY (player_id, slot_id)
);

-- Player quests
CREATE TABLE player_quests (
    player_id uuid,
    quest_id int,
    status text,
    progress map<text, int>,
    PRIMARY KEY (player_id, quest_id)
);

-- Player abilities
CREATE TABLE player_abilities (
    player_id uuid,
    ability_id int,
    cooldown_end timestamp,
    PRIMARY KEY (player_id, ability_id)
);
```

### Server Components

```cpp
// PersistenceManager.hpp
class PersistenceManager {
public:
    void Initialize();
    void Shutdown();
    
    // Async operations
    Task<> SavePlayerAsync(Entity player);
    Task<PlayerData> LoadPlayerAsync(uint64_t playerId);
    
    // Cache operations
    void CacheSession(PlayerSession session);
    Optional<PlayerSession> GetSession(uint64_t playerId);
    
private:
    RedisClient redis_;
    ScyllaClient scylla_;
};

// PlayerComponent additions
struct PlayerComponent {
    uint64_t playerId;
    std::string username;
    bool isDirty;  // Needs save
    timestamp lastSaved;
};
```

### Auto-Save Triggers

```cpp
// Auto-save on significant events
void OnPlayerLevelUp(Entity player) { ScheduleSave(player, Priority::High); }
void OnQuestComplete(Entity player) { ScheduleSave(player, Priority::High); }
void OnInventoryChange(Entity player) { ScheduleSave(player, Priority::Medium); }
void OnAbilityUsed(Entity player) { ScheduleSave(player, Priority::Low); }

// Periodic save
void OnTimerTick() {
    if (now - lastAutoSave > 5min) {
        SaveAllDirtyPlayers();
    }
}
```

---

## 4. User Stories

### US-001: Player Login Load
**Description:** As a player, I want my progress to load when I log in so that I can continue playing.

**Acceptance Criteria:**
- [ ] Player data loads from database on connect
- [ ] Position, inventory, quests all restored
- [ ] Graceful handling if no save exists (new player)

### US-002: Progress Auto-Save
**Description:** As a player, I want my progress to save automatically so that I don't lose work.

**Acceptance Criteria:**
- [ ] Save triggers on level up
- [ ] Save triggers on quest complete
- [ ] Save triggers every 5 minutes
- [ ] Save triggers on logout

### US-003: Session Cache
**Description:** As a server, I want to cache session data in Redis so that lookups are fast.

**Acceptance Criteria:**
- [ ] Session stored in Redis on login
- [ ] Session retrieved from Redis first
- [ ] Redis failure falls back to Scylla

---

## 5. Functional Requirements

- FR-1: Connect to Redis at config address
- FR-2: Connect to Scylla at config address
- FR-3: Serialize PlayerComponent to JSON/binary
- FR-4: Deserialize player data on login
- FR-5: Auto-save on level up
- FR-6: Auto-save on quest complete
- FR-7: Periodic auto-save (5 min interval)
- FR-8: Save on disconnect
- FR-9: Handle connection failures gracefully
- FR-10: New player created if no save exists

---

## 6. Non-Goals

- No account creation UI (existing)
- No database admin tools
- No migration scripts
- No cross-datacenter sync

---

## 7. Implementation Plan

### Week 1: Database Connection

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Install Redis client lib | RedisClient.hpp |
| 3-4 | Install Scylla client lib | ScyllaClient.hpp |
| 5-7 | Implement connections | Connected |

### Week 2: Data Layer

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-10 | Player serialization | Serialize works |
| 11-12 | Load/save operations | CRUD works |
| 13-14 | Auto-save triggers | Triggers fire |

### Week 3: Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Integrate with PlayerSystem | Flow works |
| 18-19 | Error handling | Graceful fails |
| 20-21 | Test and document | All works |

---

## 8. Testing Requirements

### Unit Tests
- Serialization round-trip
- Cache miss handling
- Connection failure handling

### Integration Tests
- Login with save data
- Login with new player
- Disconnect saves data

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | High |
| Time | 3 weeks |
| LOC | ~600 |
| Skills | C++, Redis, Scylla |

---

## 10. Open Questions

1. **Q: Redis/Scylla available?**
   - A: Requires Docker (not in current env)

2. **Q: Sync or async saves?**
   - A: Async with foreground timeout

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Install client libraries