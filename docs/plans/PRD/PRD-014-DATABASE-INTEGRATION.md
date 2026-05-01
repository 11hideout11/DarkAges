# PRD-014: Database Integration (Redis/ScyllaDB)

**Version:** 1.0  
**Status:** 🔴 Not Started  
**Owner:** DATABASE_AGENT  
**Priority:** HIGH  
**Dependencies:** PRD-006 (Infrastructure)

---

## 1. Overview

### 1.1 Purpose
Enable Redis (hot-state caching) and ScyllaDB (persistence) integration to replace stubs, providing real player session persistence and long-term character storage.

### 1.2 Scope
- Redis driver integration (hiredis)
- ScyllaDB driver integration (cassandra-cpp-driver)
- Session persistence ( Redis hot cache)
- Character persistence (Scylla cold storage)
- Data migration tools

---

## 2. Current Gap Analysis

| Current State | Target State | Impact |
|--------------|--------------|--------|
| RedisManager stub active | Real hiredis driver | Sub-millisecond cache |
| ScyllaManager stub active | Real cassandra driver | Persistence layer |
| No data persistence | Player sessions persist | Multi-server support |

---

## 3. Requirements

### 3.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| DB-001 | Enable hiredis driver (Linux/WSL) | P0 | Redis client library |
| DB-002 | Enable cassandra driver | P0 | ScyllaDB client |
| DB-003 | Session persistence | P0 | Player state survives restart |
| DB-004 | Character save/load | P0 | Full character data |
| DB-005 | Inventory persistence | P1 | Item storage |
| DB-006 | Leaderboard integration | P2 | Rankings storage |

### 3.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| Redis latency | <1ms | <5ms |
| Scylla latency | <10ms | <50ms |
| Connection pool | 10 connections | 5 connections |
| Max queue depth | 1000 | 500 |

---

## 4. Implementation Strategy

### 4.1 Redis Integration

```
RedisManager:
  - connection: redisContext
  - pool: ConnectionPool
  
Session Cache (Redis):
  - Key: "session:{playerId}"
  - Value: JSON(player state)
  - TTL: 3600s (1 hour)
  
Cached Data:
  - Position/Rotation
  - HP/MP
  - Equipped items
  - Cooldowns
```

### 4.2 ScyllaDB Integration

```
ScyllaManager:
  - session: CassSession
  - cluster: CassCluster
  
Schema:
  - keyspace: darkages
  - table: characters
  - table: inventory
  - table: guilds
  - table: leaderboards
```

### 4.3 Caching Strategy

```
Player Login Flow:
1. Check Redis session cache
2. If miss → load from ScyllaDB
3. Store in Redis cache
4. Return to player

Player Save Flow:
1. Queue update to Redis (async)
2. Queue to ScyllaDB (periodic batch)
3. Confirm save to player

Hot/Cold Separation:
- Redis: Position, HP, cooldowns (frequent updates)
- Scylla: Inventory, quests, achievements (infrequent)
```

---

## 5. Dependencies

### 5.1 External Dependencies
- hiredis (Redis client for C)
- cassandra-cpp-driver (ScyllaDB driver)
- libuv (async I/O)

### 5.2 Internal Dependencies
- PlayerComponent (server state)
- InventoryComponent (item storage)

---

## 6. Deliverables

### 6.1 Implementation Files
- `src/server/src/persistence/RedisManager.cpp`
- `src/server/src/persistence/ScyllaManager.cpp`
- `src/server/include/persistence/RedisManager.hpp`
- `src/server/include/persistence/ScyllaManager.hpp`

### 6.2 Schema Files
- `data/schema/cassandra.cql`

### 6.3 Configuration Files
- `config/redis.yaml`
- `config/scylla.yaml`

### 6.4 Test Files
- `src/server/tests/TestRedisManager.cpp`
- `src/server/tests/TestScyllaManager.cpp`

---

## 7. Testing

| Test | Location | Criteria |
|------|----------|--------|
| Redis connection | TestRedisManager | Connects successfully |
| Session cache | TestRedisManager | Session stored/retrieved |
| Scylla connection | TestScyllaManager | Connects successfully |
| Character save | Integration | Full save/load cycle |
| Failover | Integration | Handles disconnect gracefully |

---

## 8. Acceptance Criteria

- [ ] Redis connects and caches player session
- [ ] Scylla persists character data
- [ ] Player session survives server restart
- [ ] Inventory saves correctly
- [ ] No test regressions

---

*Last Updated: 2026-05-01*