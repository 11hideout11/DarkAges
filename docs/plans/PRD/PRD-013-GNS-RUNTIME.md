# PRD-013: GNS Runtime Integration

**Version:** 1.0  
**Status:** 🔴 Not Started  
**Owner:** NETWORK_AGENT  
**Priority:** CRITICAL  
**Dependencies:** PRD-002 (Networking)

---

## 1. Overview

### 1.1 Purpose
Enable full GameNetworkingSockets (GNS) runtime integration to replace the current UDP stub, providing production-grade reliable/ordered channels, NAT traversal, and connection quality metrics.

### 1.2 Scope
- GNSNetworkManager implementation
- Reliable/ordered channel separation
- NAT traversal (ICE/STUN/TURN)
- Connection quality metrics
- Delta compression Protocol.cpp activation

---

## 2. Current Gap Analysis

| Current State | Target State | Impact |
|--------------|--------------|--------|
| `ENABLE_GNS=OFF` in CI | `ENABLE_GNS=ON` | Real network layer |
| Stub NetworkManager in use | GNSNetworkManager in production | Production-ready |
| Protocol.cpp excluded due to GNS | Full delta compression | 80% bandwidth reduction |

---

## 3. Requirements

### 3.1 Functional Requirements

| ID | Requirement | Priority | Notes |
|----|-------------|----------|-------|
| GNS-001 | Replace stub NetworkManager with GNS implementation | P0 | Full GNS network stack |
| GNS-002 | Resolve WebRTC submodule blocking | P0 | Unblock build |
| GNS-003 | Enable ENABLE_GNS in CI pipelines | P0 | Build with GNS |
| GNS-004 | Reliable/ordered channel separation | P1 | Separate unreliable (game state) from reliable (chat/inventory) |
| GNS-005 | Connection quality metrics | P1 | RTT, jitter, packet loss |
| GNS-006 | NAT traversal (ICE) | P1 | Direct connection support |

### 3.2 Performance Requirements

| Metric | Target | Critical |
|--------|--------|----------|
| Connection latency | <100ms | <500ms |
| Throughput | >10 Mbps | >1 Mbps |
| Memory per connection | <1MB | <5MB |

---

## 4. Implementation Strategy

### 4.1 Architecture

```
NetworkManager (interface):
  - Send(packet, channel)
  - Receive() -> packet
  - GetConnectionQuality() -> metrics
  
GNSNetworkManager:
  - socket: CGameNetworkingSockets
  - channels: array<2> (unreliable, reliable)
  - connection: HSteamNetConnection
  
Channel Configuration:
  - CHANNEL_UNRELIABLE: 
      - nSendsInOrder = 0
      - nMaxDelayedPacks = 0 (no NAG)
  - CHANNEL_RELIABLE:
      - nSendsInOrder = 1
      - nMaxDelayedPacks = 8
```

### 4.2 Build Integration

```cmake
# CMakeLists.txt
option(ENABLE_GNS "Enable GameNetworkingSockets" ON)
if(ENABLE_GNS)
  find_package(GameNetworkingSockets REQUIRED)
  add_subdirectory(third_party/webrtc)
endif()
```

### 4.3 Protocol.cpp Activation

Once GNS is enabled, Protocol.cpp delta compression becomes viable:
- Reliable channel for inventory/trade (mandatory)
- Delta compression for bandwidth savings
- Binary protocol optimization

---

## 5. Dependencies

### 5.1 External Dependencies
- GameNetworkingSockets (Valve)
- WebRTC (for NAT traversal)
- libsodium (for encryption)

### 5.2 Internal Dependencies
- Protocol.cpp (currently excluded)
- NetworkClient (client-side integration)
- Packet definitions

---

## 6. Deliverables

### 6.1 Implementation Files
- `src/server/src/network/GNSNetworkManager.cpp`
- `src/server/src/network/GNSNetworkManager.hpp`

### 6.2 Build Files
- `CMakeLists.txt` (updates for GNS)
- `cmake/FindGameNetworkingSockets.cmake`

### 6.3 Test Files
- `src/server/tests/TestGNSNetworkManager.cpp`

---

## 7. Testing

| Test | Location | Criteria |
|------|----------|--------|
| Connection | TestGNSNetworkManager | Peer connects successfully |
| Reliable delivery | TestGNSNetworkManager | Ordered delivery guaranteed |
| Unreliable delivery | TestGNSNetworkManager | Best-effort delivery |
| NAT traversal | Integration | Direct connection works |
| Connection metrics | Integration | RTT reported accurately |

---

## 8. Acceptance Criteria

- [ ] GNSNetworkManager used at runtime (not stub)
- [ ] All 2097 tests pass with GNS enabled
- [ ] Delta compression Protocol.cpp active
- [ ] Reliable channel for inventory/trade works
- [ ] Connection quality metrics visible in debug UI

---

*Last Updated: 2026-05-01*