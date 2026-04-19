# Data Flow - Write-Through Caching Pipeline

This document describes how data flows between the game server, Redis (hot cache), and ScyllaDB (persistent store) in the DarkAges MMO architecture.

## Architecture Overview

```
                    +------------------+
                    |   Game Server    |
                    |   (60Hz tick)    |
                    +--------+---------+
                             |
                    +--------v---------+
                    |   RedisManager   |
                    |    (Facade)      |
                    +--------+---------+
                             |
              +--------------+--------------+
              |              |              |
    +---------v--+  +--------v-----+  +----v--------+
    |  Player     |  |   Zone       |  |  PubSub     |
    |  Session    |  |   Manager    |  |  Manager    |
    |  Manager    |  |              |  |             |
    +------+------+  +------+-------+  +------+------+
           |                |                 |
           +--------+-------+--------+-------+
                    |                |
           +--------v-------+  +----v----------+
           |    Redis       |  |  ScyllaDB     |
           |  (hot cache)   |  |  (persistent) |
           +----------------+  +---------------+
                    |                ^
                    +-------+--------+
                            |
                    +-------v--------+
                    | ScyllaManager  |
                    | (Facade)       |
                    +-------+--------+
                            |
              +-------------+-------------+
              |                           |
    +---------v------+          +---------v------+
    |   Combat       |          |   AntiCheat    |
    |   Event Logger |          |   Logger       |
    +----------------+          +----------------+
```

## Read Path (Cache-Aside)

For hot game state reads:

```
1. Game tick requests player session data
2. RedisManager::loadPlayerSession(playerId)
3. GET player:{playerId}:session
4. If HIT  -> Deserialize binary blob -> Return PlayerSession
5. If MISS -> Return failure (session expired or player not logged in)
```

Position reads follow the same pattern with `GET player:{playerId}:pos`.

## Write Path (Write-Through to Cache, Async to Persistent)

### Player State Updates

```
Game Tick (60Hz)
    |
    v
PlayerSessionManager::savePlayerSession(session)
    |
    v
Redis SET player:{id}:session <binary_blob> EX 300
    |
    v
[Redis updated immediately] -- synchronous to cache
    |
    v
Periodic checkpoint (every 30s):
    |
    v
ScyllaManager::savePlayerState(playerId, zoneId, timestamp)
    |
    v
INSERT INTO player_sessions (async, fire-and-forget)
```

### Position Updates

Position updates are the highest-frequency writes (every tick per player):

```
Game Tick -> updatePlayerPosition(playerId, pos)
    |
    v
Redis SET player:{id}:pos <16-byte_blob> EX 300
    |
    v
[No ScyllaDB write -- positions are ephemeral]
```

Position data is NOT persisted to ScyllaDB. It lives only in Redis with a 5-minute TTL.

### Combat Events

```
Combat occurs in game
    |
    v
ScyllaManager::logCombatEvent(event)
    |
    v
CombatEventLogger::logCombatEvent(session, event)
    |
    v
Async INSERT INTO combat_events (prepared statement)
    |
    v
[Batch variant: CASS_BATCH_TYPE_UNLOGGED for bulk writes]

In parallel:
    |
    v
CombatEventLogger::updatePlayerStats(session, stats)
    |
    v
Async UPDATE player_stats SET counter = counter + N
    |
    v
[Both writes are async, non-blocking to game thread]
```

### Zone Membership Changes

```
Player enters zone X:
    |
    v
RedisManager::addPlayerToZone(zoneId, playerId)
    |
    v
SADD zone:{zoneId}:players {playerId}

Player leaves zone X:
    |
    v
RedisManager::removePlayerFromZone(zoneId, playerId)
    |
    v
SREM zone:{zoneId}:players {playerId}
```

Zone membership is Redis-only (not persisted).

### Cross-Zone Messaging

```
Zone A needs to send event to Zone B:
    |
    v
RedisManager::publishToZone(zoneB_id, zoneMessage)
    |
    v
PUBLISH zone:{zoneB_id}:messages <binary_payload>

All zones broadcast:
    |
    v
RedisManager::broadcastToAllZones(zoneMessage)
    |
    v
PUBLISH zone:broadcast <binary_payload>
```

Pub/sub is fire-and-forget. If a zone server is down, messages are lost (acceptable for transient game events).

## Data Lifetime Summary

| Data Type | Redis TTL | ScyllaDB TTL | Notes |
|-----------|-----------|--------------|-------|
| Player session | 300s (5 min) | 90 days | Redis for hot access, ScyllaDB for history |
| Player position | 300s (5 min) | Not stored | Ephemeral, only in Redis |
| Zone membership | No TTL | Not stored | Explicitly managed (SADD/SREM) |
| Combat events | Not stored | 30 days | Write directly to ScyllaDB |
| Player stats | Not stored | No TTL | Counter table, lifetime stats |
| Anti-cheat events | Not stored | Not defined (TBD) | Write directly to ScyllaDB |
| Leaderboard daily | Not stored | 90 days | Derived from stats, rebuilt daily |

## Batch Write Configuration

| Parameter | Value | When Applied |
|-----------|-------|--------------|
| `REDIS_KEY_TTL_SECONDS` | 300 | All Redis SET operations |
| `SCYLLA_WRITE_BATCH_SIZE` | 1000 | Combat event batching threshold |
| `SCYLLA_WRITE_BATCH_INTERVAL_MS` | 5000 | Max time before flushing queued writes |
| `PLAYER_SAVE_INTERVAL_MS` | 30000 | Periodic player state checkpoint to ScyllaDB |
| `ZONE_CHECKPOINT_INTERVAL_MS` | 60000 | Periodic zone state checkpoint |
| Batch type | `CASS_BATCH_TYPE_UNLOGGED` | High-throughput combat events |
| Batch timeout | 5000ms | Per-batch execution timeout |

## Error Handling

### Redis Failures

- If Redis is unavailable, operations fail silently with metrics tracking
- Game continues with in-memory state only
- Reconnection is automatic via the connection pool
- `commandsFailed_` counter tracks failures

### ScyllaDB Failures

- Async writes queue up to `pendingOperations` limit (1000 low / 5000 high water marks)
- Failed writes are tracked in `writesFailed_`
- Shutdown waits up to 10 seconds for pending operations to drain
- Combat events lost on failure are acceptable (not re-queued)

### Connection Timeouts

| Service | Timeout | Configuration |
|---------|---------|---------------|
| Redis connect | 100ms | `Constants::REDIS_CONNECTION_TIMEOUT_MS` |
| Redis command | 10ms | `Constants::REDIS_COMMAND_TIMEOUT_MS` |
| ScyllaDB reconnect | 1000ms | `cass_cluster_set_reconnect_wait_time` |

## Performance Targets

| Metric | Target | Source |
|--------|--------|--------|
| Redis latency | <1ms | `RedisManager.cpp:2` comment |
| Redis throughput | 10k ops/sec | `RedisManager.cpp:2` comment |
| Redis pool | 2-10 connections | `RedisManager.cpp:60` |
| ScyllaDB I/O threads | 4 | `ScyllaManager.cpp:153` |
| ScyllaDB connections/host | 2-8 | `ScyllaManager.cpp:161-162` |

## Tick Integration

Both managers are updated every game tick (60Hz):

```
GameLoop::tick() {
    redisManager.update();    // Process pub/sub messages, fire callbacks
    scyllaManager.update();   // Check batch flush timers
}
```

- `RedisManager::update()` processes queued callbacks and pub/sub messages
- `ScyllaManager::update()` checks if pending batch writes should be flushed
