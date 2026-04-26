# DarkAges Network Protocol Specification

**Date:** 2026-04-26
**Status:** Current (UDP implementation)
**Protocol Version:** 1.0

---

## Overview

DarkAges uses a custom binary UDP protocol for client-server communication. The protocol is designed for low-latency, high-frequency updates with server-authoritative game state.

### Key Characteristics

| Property | Value |
|----------|-------|
| Transport | UDP (IPv4) |
| Tick Rate | 60Hz server, 20Hz snapshot broadcast |
| Protocol Version | 1.0 (major << 16 \| minor) |
| Max Packet Size | 2048 bytes |
| Connection Timeout | 30 seconds |
| Rate Limit | 100 packets/sec per connection |

### Architecture

```
Client (Godot C#)                         Server (C++ EnTT)
      |                                          |
      |--- UDP socket (non-blocking) <---------->|
      |                                          |
      |  Connection Flow:                        |
      |  1. Client sends [type:6, version, id]   |
      |  2. Server responds [type:7, success,    |
      |                   entity_id, zone_id]    |
      |                                          |
      |  Game Loop (bi-directional):             |
      |  1. Client -> Server: Input [type:1]     |
      |  2. Client -> Server: Ping [type:4]      |
      |  3. Server -> Client: Snapshot [type:2]  |
      |  4. Server -> Client: Pong [type:5]      |
      |  5. Server -> Client: Combat [type:11]   |
      |  6. Server -> Client: Event [type:3]     |
```

---

## Packet Types

### Packet Type IDs

| ID | Direction | Name | Reliability |
|----|-----------|------|-------------|
| 1 | C->S | `PACKET_CLIENT_INPUT` | Unreliable |
| 2 | S->C | `PACKET_SERVER_SNAPSHOT` | Unreliable |
| 3 | S->C | `PACKET_RELIABLE_EVENT` | Reliable |
| 4 | C->S | `PACKET_PING` | Unreliable |
| 5 | S->C | `PACKET_PONG` | Unreliable |
| 6 | C->S | `PACKET_CONNECTION_REQUEST` | Unreliable |
| 7 | S->C | `PACKET_CONNECTION_RESPONSE` | Unreliable |
| 8 | S->C | `PACKET_CORRECTION` | Reliable |
| 9 | C->S | `PACKET_RESPAWN_REQUEST` | Reliable |
| 10 | C->S | `PACKET_COMBAT_ACTION` | Reliable |
| 11 | S->C | `PACKET_COMBAT_RESULT` | Reliable |

---

## Client-to-Server Packets

### 1. Connection Request (type=6)

Initial connection handshake.

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 6
1       4     uint32    Protocol version (major << 16 | minor)
5       4     uint32    Player ID (client-assigned, may be 0)
```

**Total Size:** 9 bytes

**Example:**
```
06                    # type = connection request
00 00 01 00           # version 1.0
00 00 00 01           # player ID = 1
```

**Server Response:** Packet type 7 (Connection Response)

---

### 2. Client Input (type=1)

Player input state sent every tick (up to 60Hz).

```
Offset  Size  Type       Description
------  ----  ---------  -----------
0       1     uint8      Packet type = 1
1       4     uint32     Sequence number (monotonic per connection)
5       4     uint32     Client timestamp (ms since connect)
9       1     uint8      Input flags (bitfield)
10      2     int16      Yaw (fixed-point, divide by 10000 for float)
12      2     int16      Pitch (fixed-point, divide by 10000 for float)
14      4     uint32     Target entity ID
```

**Total Size:** 18 bytes

**Input Flags Bitfield:**
```
Bit 0: forward
Bit 1: backward
Bit 2: left strafe
Bit 3: right strafe
Bit 4: jump
Bit 5: attack
Bit 6: block
Bit 7: sprint
```

**Example:**
```
01                             # type = client input
01 00 00 00                    # sequence = 1
00 10 00 00                    # timestamp = 4096ms
22                             # flags: forward(1) + attack(1) + sprint(1)
68 65 00 00                    # yaw = 0.09817 rad (0x6568 / 10000)
00 00 00 00                    # pitch = 0
00 00 00 0A                    # target = entity 10
```

---

### 3. Ping (type=4)

Latency measurement request.

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 4
1       4     uint32    Client timestamp (echoed in pong)
```

**Total Size:** 5 bytes

**Server Response:** Packet type 5 (Pong)

---

### 4. Respawn Request (type=9)

Player requests respawn after death.

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 9
1       4     uint32    Entity ID requesting respawn
```

**Total Size:** 5 bytes

---

### 5. Combat Action RPC (type=10)

Player initiates a combat action (melee, ranged, ability).

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 10
1       1     uint8     Action type (1=melee, 2=ranged, 3=ability)
2       4     uint32    Target entity ID
6       4     uint32    Client timestamp
```

**Total Size:** 10 bytes

**Action Types:**
```
1 = Melee attack
2 = Ranged attack
3 = Ability (4-slot loadout selection)
```

---

## Server-to-Client Packets

### 6. Connection Response (type=7)

Response to connection request.

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 7
1       1     uint8     Success (1=success, 0=failure)
2       4     uint32    Assigned entity ID
6       4     uint32    Zone ID
```

**Total Size:** 10 bytes

---

### 7. Server Snapshot (type=2)

World state broadcast at 20Hz.

```
Offset  Size  Type       Description
------  ----  ---------  -----------
0       1     uint8      Packet type = 2
1       4     uint32     Server tick number
5       4     uint32     Last processed input sequence
9       4     uint32     Entity count (N)
13     variable  Entity[]  Entity data blocks
```

**Entity Data Block (per entity):**
```
Offset  Size  Type       Description
------  ----  ---------  -----------
0       4     uint32     Entity ID
4       4     float32    Position X
8       4     float32    Position Y
12      4     float32    Position Z
16      4     float32    Velocity X
20      4     float32    Velocity Y
24      4     float32    Velocity Z
28      1     uint8      Health percent (0-100)
29      1     uint8      Animation state
```

**Total Size:** 13 + (entity_count * 30) bytes

**Example (1 entity):**
```
02                             # type = snapshot
00 00 01 2C                    # server tick = 300
00 00 00 00                    # last input seq = 0
00 00 00 01                    # entity count = 1
00 00 00 0A                    # entity ID = 10
00 00 80 3F                    # x = 1.0
00 00 00 40                    # y = 2.0
00 00 00 40                    # z = 2.0
00 00 00 00                    # vx = 0
00 00 00 00                    # vy = 0
00 00 00 00                    # vz = 0
64                             # health = 100%
00                             # anim state = idle
```

---

### 8. Pong (type=5)

Response to ping for latency measurement.

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 5
1       4     uint32    Echoed client timestamp
```

**Total Size:** 5 bytes

---

### 9. Reliable Event (type=3)

Generic reliable event (combat events, pickups, etc.).

```
Offset  Size  Type       Description
------  ----  ---------  -----------
0       1     uint8      Packet type = 3
1       1     uint8      Event subtype
2       4     uint32     Attacker/Source entity ID
6       4     uint32     Target entity ID
10      4     int32      Damage/Value
14      1     uint8      Target health percent
15      4     uint32     Timestamp (ms)
```

**Event Subtypes:**
```
1 = Damage
2 = Death
3 = Heal
```

**Total Size:** 19 bytes

---

### 10. Server Correction (type=8)

Correction for client misprediction (rare, sent on desync detection).

```
Offset  Size  Type       Description
------  ----  ---------  -----------
0       1     uint8      Packet type = 8
1       4     uint32     Server tick
5       4     float32    Corrected position X
9       4     float32    Corrected position Y
13      4     float32    Corrected position Z
17      4     float32    Corrected velocity X
21      4     float32    Corrected velocity Y
25      4     float32    Corrected velocity Z
29      4     uint32     Last processed input sequence
```

**Total Size:** 33 bytes

---

### 11. Combat Result (type=11)

Result of a combat action processed server-side.

```
Offset  Size  Type      Description
------  ----  --------  -----------
0       1     uint8     Packet type = 11
1       1     uint8     Result code
2       4     int32     Damage dealt
6       4     uint32    Target entity ID
10      1     uint8     Is critical (0 or 1)
11      4     uint32    Timestamp
```

**Total Size:** 15 bytes

**Result Codes:**
```
0 = Hit
1 = Miss
2 = Blocked
3 = Cooldown not ready
4 = GCD active
```

---

## Delta Compression

The protocol supports delta compression to reduce bandwidth. When sending snapshots, the server may:

1. **Full Snapshot:** When `baselineEntities` is empty, sends complete entity state for all visible entities
2. **Delta Snapshot:** When baseline exists, only sends entities that changed since baseline tick

### Delta Detection

Entities are compared field-by-field. Only changed fields are sent:

```
DeltaFieldMask:
  0x01 = Position changed
  0x02 = Rotation changed
  0x04 = Velocity changed
  0x08 = Health changed
  0x10 = Animation state changed
  0x20 = Entity type changed (new entity)
```

### Bandwidth Optimization

With 400 entities at 20Hz snapshot rate:
- **Full snapshot:** ~120 KB/s per client (unrealistic)
- **Delta compressed:** ~5-10 KB/s per client (typical)

---

## Connection Lifecycle

### 1. Connection Establishment

```
Client                          Server
  |                               |
  |--- [6] Connection Request --->|
  |                               |
  |<-- [7] Connection Response ---|
  |                               |
  |--- [1] Client Input (seq=0) ->|
  |--- [4] Ping ----------------->|
  |<-- [5] Pong ------------------|
  |                               |
```

### 2. Game Loop

```
Every 50ms (20Hz snapshot broadcast):
  Server broadcasts [2] Snapshot to all connected clients

Every tick (60Hz):
  Server receives [1] Input from each client
  Server processes game logic
  Server sends [11] Combat Result if combat occurred

Client sends [4] Ping every 5s for RTT measurement
```

### 3. Disconnection

```
Client or Server initiates:
  Server sends [6] Disconnect with reason code
  OR
  30 seconds timeout (no packets received)

  Connection cleaned up, entity removed from zone
```

---

## Protocol Versioning

The protocol version is checked on connection:

```cpp
// Protocol version format
constexpr uint32_t PROTOCOL_VERSION_MAJOR = 1;
constexpr uint32_t PROTOCOL_VERSION_MINOR = 0;
constexpr uint32_t PROTOCOL_VERSION = (MAJOR << 16) | MINOR;  // 0x00010000
```

Version compatibility:
- Major version must match exactly
- Minor version differences are backward compatible

---

## Security Considerations

### DDoS Protection

The server implements DDoS protection at the network layer:
- Connection rate limiting per IP
- Packet rate limiting per connection (>100 pps = rate limited)
- IP blacklist/whitelist support

### Anti-Cheat Integration

Input packets are validated server-side:
- Sequence numbers must be monotonic
- Timestamps must be within acceptable bounds
- Movement deltas must not exceed max speed

### Anti-Replay

Sequence numbers prevent packet replay attacks:
- Each input packet has a unique sequence
- Server tracks last processed sequence per connection
- Old sequences are rejected

---

## Future: GameNetworkingSockets (GNS) Integration

The protocol supports future migration to Steam GameNetworkingSockets for:

- Encrypted connections
- NAT traversal
- Connection quality monitoring
- Reliable ordered delivery option

When `ENABLE_GNS=ON`:
- Protocol.cpp uses Protobuf serialization
- GNSNetworkManager.cpp wraps SteamNetworkingSockets
- Protocol version check includes GNS connection state

**Current Status:** GNS implementation exists but blocked by WebRTC dependency. UDP implementation is fully functional for self-hosted deployments.

---

## Client Implementation Reference

### Godot C# Client (NetworkManager.cs)

The Godot client uses `UDPClient` for connections:

```csharp
// Connection
var request = new byte[] { 6, version, playerId };
await udpClient.SendDatagramAsync(request);

// Input sending (every tick)
var input = new byte[18];
input[0] = 1;  // type
BitConverter.GetBytes(sequence).CopyTo(input, 1);
// ... fill flags, yaw, pitch, target
await udpClient.SendDatagramAsync(input);

// Receive snapshots
var buffer = new byte[2048];
var result = await udpClient.ReceiveAsync(buffer);
```

### Snapshot Processing

```csharp
// Parse entity from snapshot
int entityId = BitConverter.ToUInt32(data, offset + 0);
float x = BitConverter.ToSingle(data, offset + 4);
float y = BitConverter.ToSingle(data, offset + 8);
float z = BitConverter.ToSingle(data, offset + 12);
byte health = data[offset + 28];
byte animState = data[offset + 29];
```

---

## Bandwidth Budget

| Resource | Budget | Notes |
|----------|--------|-------|
| Downstream (server->client) | ≤20 KB/s | 20Hz snapshot + events |
| Upstream (client->server) | ≤2 KB/s | 60Hz input + occasional ping |
| Memory per player | ≤512 KB | Entity state + connection buffers |

---

## Revision History

| Date | Version | Changes |
|------|---------|---------|
| 2026-04-26 | 1.0 | Initial documentation from protocol implementation |