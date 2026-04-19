# Redis Schema - DarkAges MMO

Redis serves as the **hot-state cache** layer for the DarkAges MMO server. All operations are async (non-blocking) via hiredis with connection pooling. Redis is the system of record for ephemeral game state (sessions, positions, zone membership), while ScyllaDB handles persistent analytics data.

## Key Naming Conventions

All keys follow the pattern `namespace:id:field`:

| Pattern | Example | Data Structure | Description |
|---------|---------|----------------|-------------|
| `player:{id}:session` | `player:12345:session` | String (binary blob) | Full player session state |
| `player:{id}:pos` | `player:12345:pos` | String (binary blob) | Player position (lightweight) |
| `zone:{id}:players` | `zone:1:players` | Set | Player IDs in a zone |
| `zone:{id}:entities` | `zone:1:entities` | Set | Entity IDs in a zone |
| `entity:{id}:state` | `entity:99999:state` | String (binary blob) | Entity state data |

Key generation is centralized in the `RedisKeys` namespace (see `RedisManager.hpp:163-183`):

```cpp
namespace RedisKeys {
    inline std::string playerSession(uint64_t playerId) {
        return "player:" + std::to_string(playerId) + ":session";
    }
    inline std::string playerPosition(uint64_t playerId) {
        return "player:" + std::to_string(playerId) + ":pos";
    }
    inline std::string zonePlayers(uint32_t zoneId) {
        return "zone:" + std::to_string(zoneId) + ":players";
    }
    inline std::string zoneEntities(uint32_t zoneId) {
        return "zone:" + std::to_string(zoneId) + ":entities";
    }
    inline std::string entityState(uint32_t entityId) {
        return "entity:" + std::to_string(entityId) + ":state";
    }
}
```

## Data Structures

### Player Session (String - binary blob)

**Key**: `player:{id}:session`
**TTL**: 300 seconds (5 minutes, `Constants::REDIS_KEY_TTL_SECONDS`)
**Size**: 76 bytes fixed

Binary format (little-endian):

| Offset | Size | Field | Type |
|--------|------|-------|------|
| 0 | 8 | playerId | uint64 |
| 8 | 4 | zoneId | uint32 |
| 12 | 4 | connectionId | uint32 |
| 16 | 4 | position.x | int32 (fixed-point) |
| 20 | 4 | position.y | int32 (fixed-point) |
| 24 | 4 | position.z | int32 (fixed-point) |
| 28 | 4 | position.timestamp_ms | uint32 |
| 32 | 4 | health | int32 |
| 36 | 8 | lastActivity | uint64 |
| 44 | 32 | username | char[32] (null-padded) |

**Operations**: SET/GET/DEL via `PlayerSessionManager` (see `PlayerSessionManager.cpp`)

### Player Position (String - binary blob)

**Key**: `player:{id}:pos`
**TTL**: 300 seconds (5 minutes)
**Size**: 16 bytes fixed

Binary format (little-endian):

| Offset | Size | Field | Type |
|--------|------|-------|------|
| 0 | 4 | x | int32 (fixed-point) |
| 4 | 4 | y | int32 (fixed-point) |
| 8 | 4 | z | int32 (fixed-point) |
| 12 | 4 | timestamp_ms | uint32 |

**Operations**: SET/GET via `PlayerSessionManager::updatePlayerPosition()`

### Zone Player Set (Set)

**Key**: `zone:{id}:players`
**TTL**: None (managed explicitly)
**Members**: Player IDs (uint64)

**Operations**:
- `SADD zone:{id}:players {playerId}` -- add player to zone
- `SREM zone:{id}:players {playerId}` -- remove player from zone
- `SMEMBERS zone:{id}:players` -- get all players in zone

Implemented in `ZoneManager.cpp` with hiredis `redisCommand()`.

## TTL Policies

| Key Pattern | TTL | Rationale |
|-------------|-----|-----------|
| `player:{id}:session` | 300s (5 min) | Auto-expire disconnected players |
| `player:{id}:pos` | 300s (5 min) | Positional data is highly transient |
| `zone:{id}:players` | No TTL | Explicitly managed (SADD/SREM) |
| `zone:{id}:entities` | No TTL | Explicitly managed |
| `entity:{id}:state` | 300s (5 min) | Transient game state |

Default TTL is `Constants::REDIS_KEY_TTL_SECONDS = 300`.

## Pub/Sub Channels

Cross-zone communication uses Redis pub/sub managed by `PubSubManager`. A dedicated listener thread processes messages asynchronously.

### Channel Patterns

| Channel | Direction | Payload | Description |
|---------|-----------|---------|-------------|
| `zone:{id}:messages` | Per-zone | `ZoneMessage` binary | Messages targeted at a specific zone |
| `zone:broadcast` | Global | `ZoneMessage` binary | Messages broadcast to all zones |

### ZoneMessage Types

The `ZoneMessage` struct (`PubSubManager.hpp:33-43`) carries cross-zone events:

```cpp
enum class ZoneMessageType : uint8_t {
    ENTITY_SYNC = 1,        // Entity state update for aura projection
    MIGRATION_REQUEST = 2,  // Initiate entity migration between zones
    MIGRATION_STATE = 3,    // Migration state update
    MIGRATION_COMPLETE = 4, // Migration finished
    BROADCAST = 5,          // Zone-wide broadcast
    CHAT = 6,               // Cross-zone chat
    ZONE_STATUS = 7         // Zone status update
};
```

**ZoneMessage binary format** (21-byte header + variable payload):

| Offset | Size | Field |
|--------|------|-------|
| 0 | 1 | type (ZoneMessageType) |
| 1 | 4 | sourceZoneId |
| 5 | 4 | targetZoneId (0 = broadcast) |
| 9 | 4 | timestamp |
| 13 | 4 | sequence |
| 17 | 4 | payload_size |
| 21 | N | payload bytes |

### Usage

```cpp
// Subscribe to zone-specific and broadcast channels
redis.subscribeToZoneChannel(myZoneId, [](const ZoneMessage& msg) {
    // Handle cross-zone message
});

// Publish to specific zone
redis.publishToZone(targetZoneId, message);

// Broadcast to all zones
redis.broadcastToAllZones(message);
```

The pub/sub listener thread (`PubSubListener` in `PubSubManager.cpp:28-57`) runs on a dedicated hiredis connection (not from the pool). Messages are queued and dispatched on the main thread via `processSubscriptions()`.

## Redis Streams

Alternative to pub/sub for non-blocking event streaming. Managed by `StreamManager`.

**Operations**:
- `XADD` -- add entry to stream
- `XREAD` -- read entries from stream (supports COUNT and BLOCK)

```cpp
// Add entry
redis.xadd("stream:combat", "*", {{"event", "kill"}, {"attacker", "123"}});

// Read entries
redis.xread("stream:combat", lastId, [](const auto& result) {
    for (const auto& entry : result.value) {
        // Process entry.id, entry.fields
    }
}, 100, 0);  // count=100, blockMs=0
```

## Connection Pooling

Redis connections are pooled via `ConnectionPool` (2-10 connections). hiredis is NOT thread-safe, so each operation acquires a connection, executes, and releases.

| Setting | Value | Source |
|---------|-------|--------|
| Pool min size | 2 | `RedisManager.cpp:60` |
| Pool max size | 10 | `RedisManager.cpp:60` |
| Default port | 6379 | `Constants::REDIS_DEFAULT_PORT` |
| Connection timeout | 100ms | `Constants::REDIS_CONNECTION_TIMEOUT_MS` |
| Command timeout | 10ms | `Constants::REDIS_COMMAND_TIMEOUT_MS` |

## Metrics

RedisManager tracks operational metrics:

| Metric | Description |
|--------|-------------|
| `commandsSent_` | Total commands dispatched |
| `commandsCompleted_` | Successfully completed commands |
| `commandsFailed_` | Failed commands |
| `latencySamples` | Rolling queue of latency samples (max 1000) |
| `getAverageLatencyMs()` | Computed average from samples |

## Implementation Files

| File | Responsibility |
|------|---------------|
| `RedisManager.hpp/cpp` | Facade: key-value ops, sub-manager orchestration, metrics |
| `PlayerSessionManager.hpp/cpp` | Session CRUD, position updates, binary serialization |
| `ZoneManager.hpp/cpp` | Zone player sets (SADD/SREM/SMEMBERS) |
| `PubSubManager.hpp/cpp` | Pub/sub channels, zone messaging, listener thread |
| `StreamManager.hpp/cpp` | Redis Streams (XADD/XREAD) |
| `RedisInternal.hpp` | Internal state: pool, callbacks, latency tracking |
| `ConnectionPool.hpp/cpp` | Thread-safe hiredis connection pool |
