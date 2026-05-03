# PRD: GNS Runtime Integration

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Blocked - Implementation Pending  
**Priority:** High  
**Category:** Infrastructure - Networking

---

## 1. Problem Statement

The GNS (GameNetworkingSockets) protocol was compile-time fixed but runtime integration is incomplete. The server still uses basic UDP instead of GNS for production networking. This blocks production deployment with advanced networking features.

### Current State
- ✅ Protocol.cpp stub exists
- ✅ Build compiles with GNS
- ⚠️ Runtime port to GNS API not complete
- ⚠️ No GNS message handling
- ⚠️ No connection state machine

### Impact
- Cannot deploy with GNS features
- Missing steam networking proxy
- Limited production readiness
- Advanced features unavailable

---

## 2. Goals

### Primary Goals
1. Port NetworkManager to GNS API
2. Implement connection management
3. Add GNS message handlers
4. Wire to game events

### Success Criteria
- [ ] NetworkManager uses GNSSendMessage
- [ ] Connection state transitions work
- [ ] Reliable messages handled
- [ ] disconnection/reconnection works

---

## 3. Technical Specification

### GNS Integration Points

```cpp
// GNS NetworkManager interface
class NetworkManager {
public:
    // GNS-specific initialization
    void InitializeGNS(const GNSConfig& config);
    
    // Send modes (GNS provides these)
    void SendUnreliable(Entity target, const Packet& packet);
    void SendReliable(Entity target, const Packet& packet);
    void SendOrdered(Entity target, const Packet& packet);
    
    // Connection management
    void OnConnectionRequest(Entity remote, const ConnectionRequest& req);
    void OnConnected(Entity remote);
    void OnDisconnected(Entity remote, int reason);
    
private:
    ISteamNetworkingSockets* gnsInterface_;
    HSteamNetConnection connection_;
};

// Game packet types to GNS send modes
enum SendMode {
    Unreliable = GNS_Unreliable,
    Reliable = GNS_Reliable,
    Ordered = GNS_Reliable | GNS_NoNagle,
    Sequenced = GNS_Unreliable | GNS_NoDelayedAck
};
```

### Message Handlers

```cpp
// GNS message handler registration
void RegisterGNSHandlers() {
    gnsInterface_->SetReceiveCallback(this);
    
    // Register handlers for each packet type
    RegisterHandler(PacketType::PlayerMove, HandlePlayerMove);
    RegisterHandler(PacketType::PlayerCast, HandlePlayerCast);
    RegisterHandler(PacketType::CombatEvent, HandleCombatEvent);
    // ... etc
}

// Handle incoming GNS messages
void OnGNSMessage(HSteamNetConnection conn, const void* data, size_t len) {
    auto* packet = static_cast<const Packet*>(data);
    switch (packet->type) {
        case PacketType::PlayerMove:
            HandlePlayerMove(conn, packet);
            break;
        case PacketType::CombatEvent:
            HandleCombatEvent(conn, packet);
            break;
    }
}
```

### Connection State Machine

```
┌──────────┐     Connect      ┌───────────┐
│  CLOSED   │ ──────────────→ │ CONNECTING│
└──────────┘                 └───────────┘
                                ↓
                         Challenge/Response
                                ↓
                         ┌───────────┐
            ←────────── │ CONNECTED │
           │           └───────────┘
           │               ↓
           │          Disconnect
           │              ↓
           └────── ────│ CLOSED   │
```

### Existing Protocol.cpp Mapping

Based on existing implementation:
- `serializeCombatEvent` → PacketType::CombatEvent
- `serializeChatMessage` → PacketType::Chat
- `serializeQuestUpdate` → PacketType::Quest
- Need to map to GNS send modes

---

## 4. User Stories

### US-001: GNS Initialization
**Description:** As a server operator, I want GNS to initialize so that advanced networking is available.

**Acceptance Criteria:**
- [ ] ISteamNetworkingSockets created
- [ ] Config parsing works
- [ ] Listen socket opens

### US-002: Reliable Messaging
**Description:** As a player, I want reliable message delivery so that my actions always process.

**Acceptance Criteria:**
- [ ] Reliable packets delivered
- [ ] No duplicate delivery
- [ ] Ordered packets in sequence

### US-003: Connection Handling
**Description:** As a player, I want stable connections so that I don't disconnect randomly.

**Acceptance Criteria:**
- [ ] Connection established on login
- [ ] Handles network hiccups
- [ ] Clean disconnect on logout

---

## 5. Functional Requirements

- FR-1: Initialize GNS interface on startup
- FR-2: Map existing Send() calls to GNS modes
- FR-3: Implement connection callbacks
- FR-4: Handle incoming messages
- FR-5: Implement ping measurement
- FR-6: Handle disconnection gracefully
- FR-7: Fallback to basic UDP if GNS fails

---

## 6. Non-Goals

- No Steam authentication
- No Steam matchmaking
- No P2P relay
- No Steam leaderboards

---

## 7. Implementation Plan

### Week 1: GNS Interface

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Initialize GNS | interface works |
| 3-4 | Connect/listen | sockets work |
| 5-7 | Basic messaging | messages flow |

### Week 2: Message Handling

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Handler registration | handlers work |
| 10-11 | Message routing | routes work |
| 12-14 | Reliable delivery | works |

### Week 3: Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Wire to game logic | integration works |
| 18-19 | Error handling | graceful fails |
| 20-21 | Test and polish | all works |

---

## 8. Testing Requirements

### Unit Tests
- Send mode mapping
- Message serialization
- Connection state machine

### Integration Tests
- Connect → message → disconnect
- Reliable delivery verification
- Reconnection handling

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | High |
| Time | 3 weeks |
| LOC | ~800 |
| Skills | C++, GNS API |

---

## 10. Open Questions

1. **Q: Use steam networking or standalone?**
   - A: Standalone GNS for now

2. **Q: Fallback if GNS fails?**
   - A: Yes, UDP fallback required

---

**PRD Status:** Proposed - Awaiting Implementation  
**Required:** GNS library access  
**Author:** OpenHands Analysis  
**Next Step:** GNS interface research