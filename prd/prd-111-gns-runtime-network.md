# PRD-111: GNS Runtime Network Integration

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical  
**Prerequisite:** PRD-017 GNS compile-time fix (COMPLETE - merged)

---

## Introduction

The GNS (GameNetworkingSockets) compile-time issues were resolved in Phase 8, but the runtime network stack has not been integrated into the server tick loop. The GNSNetworkManager.cpp implementation exists but is not wired to handle actual network traffic.

## Goals

- Integrate GNS network layer into server tick loop
- Route all UDP traffic through GNS connection manager
- Implement connection state management
- Enable server-authoritative networking
- Achieve 10,000+ concurrent connections

## User Stories

### US-001: GNS Network Tick Integration
**Description:** As a server engineer, I need GNSNetworkManager to process network events every tick so that client connections remain responsive.

**Acceptance Criteria:**
- [ ] GNSConnectionManager::Process() called in server tick loop
- [ ] All network I/O occurs within tick frame
- [ ] No dropped packets under load

### US-002: Client Connection Acceptance
**Description:** As a server engineer, I need to accept incoming client connections via GNS so that players can connect to the server.

**Acceptance Criteria:**
- [ ] GNSConnectionManager::Accept() returns valid ConnectionHandle
- [ ] Connection state tracked per client
- [ ] Maximum connections enforced

### US-003: Packet Routing
**Description:** As a server engineer, I need all UDP packets to route through GNS so that the network layer is unified.

**Acceptance Criteria:**
- [ ] Receive() routes packets via GNS
- [ ] Send() routes packets via GNS
- [ ] No fallback to raw sockets when GNS enabled

### US-004: Connection State Management
**Description:** As a server engineer, I need connection state tracking so that I can detect disconnects and timeouts.

**Acceptance Criteria:**
- [ ] ConnectionState enum tracks: Connected/Disconnecting/Disconnected
- [ ] IsConnected() returns correct state
- [ ] Disconnect() triggers state change

### US-005: Disconnect Handling
**Description:** As a server engineer, I need to handle client disconnects gracefully so that resources are cleaned up.

**Acceptance Criteria:**
- [ ] Disconnect() closes GNS connection
- [ ] Resources freed on disconnect
- [ ] No zombie connections

## Functional Requirements

- FR-1: GNSConnectionManager must implement Initialize(uint16_t port, uint32_t max_connections)
- FR-2: Process(float delta_time) must be called every tick
- FR-3: Accept() must return ConnectionHandle or invalid handle on failure
- FR-4: Disconnect(ConnectionHandle) must clean up all resources
- FR-5: Receive(ConnectionHandle, buffer, size) must return bytes received
- FR-6: Send(ConnectionHandle, data, size) must return bytes sent
- FR-7: IsConnected(ConnectionHandle) must return current state
- FR-8: GetState(ConnectionHandle) returns ConnectionState enum

## Non-Goals

- Steam authentication integration (deferred)
- Relay/matrix server support (deferred)
- Custom encryption (use GNS built-in)
- Threaded GNS processing (single-threaded for MVP)

## Technical Considerations

- Integration point: ServerTickSystem.cpp calls GNSConnectionManager::Process()
- Configuration: ENABLE_GNS cmake flag
- Existing code: GNSNetworkManager.cpp exists (conditionally compiled)
- Performance target: 10K connections, <10ms latency

## Success Metrics

- Server starts with GNS when built with -DENABLE_GNS=ON
- Client connections accepted via GNS: 100%
- Packet routing through GNS: 100%
- Disconnection handling: 100% clean
- Performance: 10,000 concurrent connections validated

## Open Questions

1. **Q: Should GNS run in a separate thread?**
   - A: No, integrate into tick loop for MVP - simplify debugging

2. **Q: Enable Steam authentication?**
   - A: Deferred - use basic UDP for now

3. **Q: Connection encryption?**
   - A: Use GNS built-in encryption

---

**PRD Status:** Proposed - Awaiting Implementation
**Author:** OpenHands Gap Analysis
**Next Step:** Wire GNSConnectionManager to server tick loop