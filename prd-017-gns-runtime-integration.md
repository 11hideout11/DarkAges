# PRD-017: GNS Runtime Integration - Network Stack

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Proposed  
**Priority:** Critical  
**Prerequisite:** Phase 8 compile-time fix (COMPLETE)

---

## 1. Problem Statement

GNS (GameNetworkingSockets) compile-time issues were resolved in Phase 8, but the runtime network stack has not been integrated into the server tick loop. The `GNSNetworkManager.cpp` implementation exists but is not wired to handle actual network traffic.

### Current State
- ✅ Compile-time fix merged (PR #29)
- ✅ GNSNetworkManager.cpp implementation exists (conditionally compiled)
- ⚠️ Network stack integration pending (tick loop not connected)

### Impact
- Cannot run production server with GNS networking
- UDP traffic not routed through GNS layer
- No connection state management via GNS
- Performance benchmarks not validated

---

## 2. Goals

### Primary Goals
1. Integrate GNS network layer into server tick loop
2. Route all UDP traffic through GNS connection manager
3. Implement connection state management
4. Enable server-authoritative networking

### Success Criteria
- [ ] Server starts with GNS enabled (`ENABLE_GNS=ON`)
- [ ] Client connections accepted via GNS
- [ ] Packet routing through GNSConnectionManager
- [ ] Connection state tracked per client
- [ ] Disconnection handling implemented

---

## 3. Technical Specification

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Server Tick Loop                        │
│  ┌─────────────┐    ┌─────────────────────────────────┐    │
│  │ TickUpdate │───▶│ GNSConnectionManager::Process() │    │
│  └─────────────┘    └─────────────────────────────────┘    │
│                              │                             │
│         ┌────────────────────┼────────────────────┐          │
│         ▼                    ▼                    ▼          │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │ Accept()    │    │ Receive()  │    │ Send()     │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### GNSConnectionManager Interface

```cpp
// Header: include/netcode/GNSConnectionManager.hpp
namespace DarkAges::netcode {

class GNSConnectionManager {
public:
    // Initialize GNS networking
    bool Initialize(uint16_t port, uint32_t max_connections);
    
    // Per-tick processing
    void Process(float delta_time);
    
    // Connection management
    ConnectionHandle Accept();
    void Disconnect(ConnectionHandle handle);
    
    // Message handling
    int32_t Receive(ConnectionHandle handle, void* buffer, int32_t size);
    bool Send(ConnectionHandle handle, const void* data, int32_t size);
    
    // State queries
    bool IsConnected(ConnectionHandle handle) const;
    ConnectionState GetState(ConnectionHandle handle) const;
    
private:
    // GNS sockets
    ISocket* socket_;
    std::unordered_map<ConnectionHandle, GNSConnection> connections_;
};

} // namespace DarkAges::netcode
```

### Integration Points

| Component | Integration | Location |
|-----------|------------|----------|
| ServerMain | Create GNSConnectionManager | main.cpp |
| TickLoop | Call Process() | ServerTickSystem.cpp |
| PacketHandler | Route through GNS | NetworkPacketHandler.cpp |
| ClientSession | Use GNS send/receive | ClientSession.cpp |

### Configuration

```cmake
# Enable GNS in build
option(ENABLE_GNS "Enable GameNetworkingSockets" ON)

# GNS-specific settings
cmake_dependent_option(
    WANT_GNS_VALIDATION,
    "Enable GNS validation tests",
    ON,
    "ENABLE_GNS",
    OFF
)
```

---

## 4. Implementation Plan

### Week 1: Core Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Wire GNSConnectionManager to tick loop | Integration compiles |
| 3-4 | Implement Accept() / Disconnect() | Connections work |
| 5 | Implement Receive() routing | Packets routed |
| 7 | Test basic connectivity | Single client connects |

### Week 2: Production Features

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Implement Send() path | Bidirectional works |
| 10 | Connection state management | State machine works |
| 11-12 | Handle disconnects gracefully | Clean disconnects |
| 14 | Performance benchmark | 10K connections validated |

### Dependencies
- Phase 8 compile-time fix (COMPLETE)
- NetworkPacketHandler (exists)
- ClientSession (exists)

---

## 5. Testing Requirements

### Unit Tests
- GNSConnectionManager initialization
- Accept/disconnect workflow
- Send/receive roundtrip

### Integration Tests
- Multi-client server startup
- GameNetworkingSockets validation
- 10K concurrent connections

### Test Metrics
- Target: 10,000+ connections
- Latency: <10ms average
- Packet delivery: 99.99%

---

## 6. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | High |
| Time | 2 weeks |
| LOC | ~500 |
| Skills | C++, GNS API, CMake |

---

## 7. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| GNS API complexity | High | Medium | Use existing GNSNetworkManager.cpp |
| Connection limits | Medium | High | Benchmark incrementally |
| Platform differences | Low | High | Test on Linux/macOS |

---

## 8. Open Questions

1. **Q: Should GNS run in separate thread?**
   - A: No, integrate into tick loop for now
   
2. **Q: Enable Steam integration?**
   - A: Deferred - use basic UDP for now

3. **Q: Connection encryption?**
   - A: Use GNS built-in encryption

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Begin Week 1 tasks