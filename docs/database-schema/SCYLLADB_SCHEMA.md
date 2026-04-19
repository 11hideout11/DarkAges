# ScyllaDB Schema - DarkAges MMO

ScyllaDB serves as the **persistent analytics and audit** layer. It stores combat events, player statistics, session history, anti-cheat violations, and leaderboard data. All writes are async (non-blocking) using the cassandra-cpp-driver.

## Keyspace

```
CREATE KEYSPACE IF NOT EXISTS darkages
WITH replication = {
  'class': 'SimpleStrategy',
  'replication_factor': 1
};
```

> **Note**: Production should use `NetworkTopologyStrategy` with appropriate replication factors per datacenter.

## Tables

### combat_events

Stores all combat events (damage, heals, kills, deaths, assists) with time-bucketed partitioning.

```sql
CREATE TABLE IF NOT EXISTS darkages.combat_events (
  day_bucket    text,         -- YYYY-MM-DD date bucket
  timestamp     timestamp,    -- Event time (millisecond precision)
  event_id      uuid,         -- Unique event ID (auto-generated)
  attacker_id   bigint,       -- Attacking player ID (0 for environment)
  target_id     bigint,       -- Target player/entity ID
  damage        int,          -- Damage/heal amount
  hit_type      text,         -- Hit classification
  event_type    text,         -- "damage", "heal", "kill", "death", "assist"
  zone_id       int,          -- Zone where event occurred
  position_x    double,       -- Event X coordinate (float, from fixed-point)
  position_y    double,       -- Event Y coordinate
  position_z    double,       -- Event Z coordinate
  PRIMARY KEY ((day_bucket), timestamp, event_id)
) WITH CLUSTERING ORDER BY (timestamp DESC, event_id ASC)
  AND compaction = {
    'class': 'TimeWindowCompactionStrategy',
    'compaction_window_unit': 'DAYS',
    'compaction_window_size': 1
  }
  AND default_time_to_live = 2592000;  -- 30 days
```

**Partition key**: `day_bucket` (date string, e.g. "2026-04-18")
**Clustering keys**: `timestamp DESC, event_id ASC`

**Secondary indexes**:
```sql
CREATE INDEX idx_combat_attacker ON darkages.combat_events(attacker_id);
CREATE INDEX idx_combat_target   ON darkages.combat_events(target_id);
```

**Write pattern**: Inserted asynchronously via `CombatEventLogger::logCombatEvent()`. Batch writes use `CASS_BATCH_TYPE_UNLOGGED` for high throughput (up to 100 events per batch with 5s timeout).

**Read patterns**:
- Kill feed: `SELECT ... FROM combat_events WHERE zone_id = ? AND day_bucket = ? AND event_type = 'kill' ALLOW FILTERING`
- Player combat history by attacker: `SELECT ... FROM combat_events WHERE attacker_id = ? AND day_bucket = ? ALLOW FILTERING`

### player_stats

Counter table for cumulative player statistics. Uses Cassandra counter type for atomic increments.

```sql
CREATE TABLE IF NOT EXISTS darkages.player_stats (
  player_id              bigint PRIMARY KEY,
  total_kills            counter,
  total_deaths           counter,
  total_assists          counter,
  total_damage_dealt     counter,
  total_damage_taken     counter,
  total_damage_blocked   counter,
  total_healing_done     counter,
  total_playtime_minutes counter,
  total_matches          counter,
  total_wins             counter,
  last_updated           timestamp
);
```

**Partition key**: `player_id`
**No clustering keys** (single row per player)

**Write pattern**: Atomic counter increments via `CombatEventLogger::updatePlayerStats()`:
```sql
UPDATE darkages.player_stats SET
  total_kills = total_kills + ?,
  total_deaths = total_deaths + ?,
  total_damage_dealt = total_damage_dealt + ?,
  ...
  last_updated = toTimestamp(now())
WHERE player_id = ?;
```

**Read pattern**: `SELECT total_kills, total_deaths, ... FROM player_stats WHERE player_id = ?`

### player_sessions

Tracks individual play sessions with per-session statistics.

```sql
CREATE TABLE IF NOT EXISTS darkages.player_sessions (
  player_id        bigint,
  session_start    timestamp,   -- When session began
  session_end      timestamp,   -- When session ended (0 = active)
  zone_id          int,         -- Zone during session
  kills            int,         -- Kills this session
  deaths           int,         -- Deaths this session
  damage_dealt     bigint,      -- Damage dealt this session
  damage_taken     bigint,      -- Damage taken this session
  playtime_minutes int,         -- Session duration in minutes
  PRIMARY KEY ((player_id), session_start)
) WITH CLUSTERING ORDER BY (session_start DESC)
  AND default_time_to_live = 7776000;  -- 90 days
```

**Partition key**: `player_id`
**Clustering key**: `session_start DESC` (most recent sessions first)

**Write pattern**: Inserted via `CombatEventLogger::savePlayerState()` on player state checkpoints.

### leaderboard_daily

Daily leaderboard data for category-based rankings.

```sql
CREATE TABLE IF NOT EXISTS darkages.leaderboard_daily (
  category    text,     -- Leaderboard category (e.g. "kills", "damage")
  day         text,     -- YYYY-MM-DD date bucket
  player_id   bigint,   -- Player ID
  player_name text,     -- Display name
  score       bigint,   -- Score value
  PRIMARY KEY ((category, day), score, player_id)
) WITH CLUSTERING ORDER BY (score DESC, player_id ASC)
  AND default_time_to_live = 7776000;  -- 90 days
```

**Partition key**: `(category, day)` -- each category+day combination is a partition
**Clustering keys**: `score DESC, player_id ASC` -- sorted by score descending for leaderboard queries

### anti_cheat_events

Anti-cheat violation log (managed by `AntiCheatLogger`).

```sql
CREATE TABLE IF NOT EXISTS darkages.anti_cheat_events (
  event_id      bigint,
  timestamp     timestamp,
  player_id     bigint,
  zone_id       int,
  cheat_type    text,       -- "speed_hack", "teleport", "hitbox", etc.
  severity      text,       -- "critical", "suspicious", "minor"
  confidence    float,      -- Detection confidence (0.0-1.0)
  description   text,
  position_x    double,
  position_y    double,
  position_z    double,
  server_tick   int,
  PRIMARY KEY ((player_id), timestamp, event_id)
) WITH CLUSTERING ORDER BY (timestamp DESC, event_id ASC);
```

## Consistency Levels

| Operation | Consistency Level | Rationale |
|-----------|-------------------|-----------|
| Default writes | `LOCAL_QUORUM` | Durability/performance balance (`ScyllaManager.cpp:168`) |
| Combat event inserts | `LOCAL_QUORUM` | Critical data, must be durable |
| Counter updates | `LOCAL_QUORUM` | Counter operations require quorum |
| Player session inserts | `LOCAL_QUORUM` | Session persistence is important |

> **Hot-path optimization**: For highest throughput on non-critical writes, consistency can be lowered to `ONE`. The current codebase uses `LOCAL_QUORUM` as the cluster default.

## Compaction Strategies

| Table | Strategy | Rationale |
|-------|----------|-----------|
| `combat_events` | `TimeWindowCompactionStrategy` (1 day window) | Time-series data, old data expires via TTL |
| `player_stats` | Default (`SizeTieredCompactionStrategy`) | Counter table, single row per player |
| `player_sessions` | Default | Low write volume, per-player partitions |
| `leaderboard_daily` | Default | Daily partitions, natural time-based organization |

## Write Patterns

### Async Batching

Writes are batched by `ScyllaManager` with configurable thresholds:

| Setting | Value | Source |
|---------|-------|--------|
| Batch size | 1000 events | `Constants::SCYLLA_WRITE_BATCH_SIZE` |
| Batch interval | 5000ms (5s) | `Constants::SCYLLA_WRITE_BATCH_INTERVAL_MS` |
| Player save interval | 30000ms (30s) | `Constants::PLAYER_SAVE_INTERVAL_MS` |
| Zone checkpoint interval | 60000ms (60s) | `Constants::ZONE_CHECKPOINT_INTERVAL_MS` |

### Batch Type

Combat event batches use `CASS_BATCH_TYPE_UNLOGGED` for maximum throughput (logged batches add overhead). Individual writes use prepared statements bound with the cassandra-cpp-driver.

### Connection Pooling

| Setting | Value | Source |
|---------|-------|--------|
| I/O threads | 4 | `ScyllaManager.cpp:153` |
| I/O queue size | 10000 | `ScyllaManager.cpp:156` |
| Core connections/host | 2 | `ScyllaManager.cpp:161` |
| Max connections/host | 8 | `ScyllaManager.cpp:162` |
| Low water mark | 1000 pending | `ScyllaManager.cpp:157` |
| High water mark | 5000 pending | `ScyllaManager.cpp:158` |
| Reconnect wait | 1000ms | `ScyllaManager.cpp:165` |
| Default port | 9042 | `Constants::SCYLLA_DEFAULT_PORT` |

## Read Patterns

| Query | Prepared Statement | Partition | Filter |
|-------|-------------------|-----------|--------|
| Player stats | `queryPlayerStats` | `player_id = ?` | None |
| Kill feed | `queryKillFeed` | `zone_id = ? AND day_bucket = ?` | `event_type = 'kill'` (ALLOW FILTERING) |
| Combat history (attacker) | `queryCombatHistoryByAttacker` | `attacker_id = ? AND day_bucket = ?` | ALLOW FILTERING |
| Top killers | Via CombatEventLogger | zone/time range | Aggregation |

## Metrics

ScyllaManager tracks write performance:

| Metric | Description |
|--------|-------------|
| `writesQueued_` | Total writes submitted to driver |
| `writesCompleted_` | Successfully completed writes |
| `writesFailed_` | Failed writes |
| `pendingOperations` | Current in-flight async operations |

## Implementation Files

| File | Responsibility |
|------|---------------|
| `ScyllaManager.hpp/cpp` | Connection management, schema creation, statement preparation, orchestration |
| `CombatEventLogger.hpp/cpp` | Combat event logging, player stats, kill feed queries, player sessions |
| `AntiCheatLogger.hpp/cpp` | Anti-cheat violation event logging |
