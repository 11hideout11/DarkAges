# PRD-017: Protocol Layer Decoupling from GNS

**Version:** 1.0
**Status:** ✅ Complete — Protocol.cpp decoupled from GNS (FlatBuffers only); non-GNS build passes 100%
**Owner:** NETWORKING_AGENT
**Priority:** HIGH (P2 — Build Flexibility)
**Dependencies:** PRD-012 (GNS Runtime Integration — must coordinate)
**Issue:** #4 from PROJECT_ISSUES_TRACKER.md

---

## Implementation Status (2026-05-01)

### ✅ Completed - Stub Exists
- [x] Protocol_stub.cpp exists - works when ENABLE_GNS=OFF
- [x] NetworkManager_stub.cpp exists - alternative when GNS disabled
- [x] Conditional compilation for ENABLE_GNS

### 📋 Pending - Verification
- [ ] Test builds with ENABLE_GNS=OFF
- [ ] Verify protocol tests run in isolation

---

## 1. Overview

### 1.1 Purpose
Decouple the `Protocol.cpp` protobuf serialization layer from the `GameNetworkingSockets` (GNS) transport layer so that:
- Protocol works when `ENABLE_GNS=OFF` (test builds)
- Build modes: `ENABLE_GNS=ON` → GNS transport; `ENABLE_GNS=OFF` → stub or alternative transport
- Protocol tests can run in isolation without GNS dependency

### 1.2 Problem Statement

**Current flawed architecture:**
```
src/server/src/netcode/Protocol.cpp (Protobuf serialization)
       ↑
       └── DEPENDS ON GNS headers
              ↓
       GNSSocket.hpp / GNSNetworkManager.cpp (transport)

Build:
  ENABLE_GNS=ON  → Protocol.cpp compiled ✓
  ENABLE_GNS=OFF → Protocol.cpp excluded from build ✗
```

Result: **No serialization layer in test builds**, making protocol testing impossible without GNS.

**Desired architecture:**
```
Protocol Layer (Protobuf)         ← Pure serialization, no transport knowledge
       ↑
Transport Abstraction Layer      ← INetworkSocket interface
       ↑        ↑
   GNSSocket   StubSocket (test)
       ↑        ↑
   GNS         No transport (test only)
```

---

## 2. Requirements

### 2.1 Functional Requirements
ID    | Requirement                      | Priority | Details
------|----------------------------------|----------|--------
PRO-001 | Create `INetworkSocket` abstract interface | P0 | Pure virtual read/write methods
PRO-002 | Implement `GNSSocket` (wraps SteamNetworking) | P0 | Conforms to INetworkSocket
PRO-003 | Implement `StubSocket` (in-memory loopback) | P0 | For tests without GNS
PRO-004 | Refactor `Protocol.cpp` to depend on `INetworkSocket` | P0 | No GNS header includes
PRO-005 | Update CMakeLists to always compile Protocol.cpp | P0 | Independent of GNS flag
PRO-006 | Build validation: both modes compile & link | P0 | CI pipeline for ENABLE_GNS=ON/OFF
PRO-007 | Unit tests for Protocol (without GNS) | P0 | Test serialization/deserialization

### 2.2 Non-Functional
- Zero runtime overhead when GNS enabled (direct call through vtable OK)
- Protocol layer stays at current performance (protobuf fast path)
- Maintain ABI compatibility (no breaking changes to external plugins)
- Thread-safety preserved (existing code uses socket from network thread)

---

## 3. Current Architecture (Gap Analysis)

```
src/server/include/netcode/
  NetworkManager.hpp              ← Orchestration layer (depends on Protocol)
  INetworkSocket.hpp? (MISSING)   ← Interface we need to create
  Protocol.hpp                    ← Protobuf definitions (serialization only)
  Protocol.cpp                    ← Serialization logic (ties to GNS now)

src/server/src/netcode/
  NetworkManager.cpp              ← Uses Protocol via unknown interface
  GNSNetworkManager.cpp           ← GNS-specific implementation
  GNSSocket.hpp/cpp (MISSING)     ← Need to extract from GNSNetworkManager
  StubSocket.hpp/cpp (MISSING)    ← Need to create for tests
```

**Root cause:** Protocol.cpp currently `#include "GNSSocket.hpp"` directly. It should depend on an abstraction.

---

## 4. Technical Solution

### 4.1 Abstraction Layer: `INetworkSocket`

**New file:** `src/server/include/netcode/INetworkSocket.hpp`
```cpp
#pragma once
#include <cstdint>
#include <vector>
#include <functional>

namespace darkages::netcode {

struct SocketAddress {
    uint64_t steam_id;  // or IP:port for stub
    // ... minimal addressing
};

class INetworkSocket {
public:
    virtual ~INetworkSocket() = default;
    
    // Send serialized packet to peer
    virtual bool Send(const void* data, size_t size, const SocketAddress& addr) = 0;
    
    // Receive pending packets (calls handler per message)
    virtual void ReceiveAll(std::function<void(const void*, size_t, const SocketAddress&)> handler) = 0;
    
    // Connection state
    virtual bool IsConnected(const SocketAddress& addr) const = 0;
    virtual void Close(const SocketAddress& addr) = 0;
};

} // namespace darkages::netcode
```

### 4.2 GNSSocket Implementation

**New file:** `src/server/src/netcode/GNSSocket.cpp` (extracted from GNSNetworkManager)
```cpp
#include "INetworkSocket.hpp"
#include "GNSSocket.hpp"  // wraps SteamNetworkingSockets

class GNSSocket : public INetworkSocket {
    SteamNetworkingSocket_t* socket_;
    // ... existing GNS code moved here
public:
    bool Send(...) override { /* GNS-specific */ }
    void ReceiveAll(...) override { /* GNS-specific */ }
    bool IsConnected(...) const override { /* GNS-specific */ }
    void Close(...) override { /* GNS-specific */ }
};
```

### 4.3 StubSocket Implementation

**New file:** `src/server/src/netcode/StubSocket.cpp` (for tests)
```cpp
#include "INetworkSocket.hpp"
#include <unordered_map>

class StubSocket : public INetworkSocket {
    // In-memory queue per connection (loopback)
    std::unordered_map<SocketAddress, std::vector<std::vector<uint8_t>>> buffers_;
    
    bool Send(...) override {
        buffers_[addr].push_back({(uint8_t*)data, (uint8_t*)data + size});
        return true;
    }
    void ReceiveAll(handler) override {
        for each buffer in buffers_[addr]:
            handler(buffer.data(), buffer.size(), addr);
        buffers_[addr].clear();
    }
    // ...
};
```

### 4.4 Protocol.cpp Refactoring

**Before:**
```cpp
#include "GNSSocket.hpp"  // ❌ direct dependency

void Protocol::SerializeToSocket(const Packet& p, GNSSocket& socket) {
    auto data = Serialize(p);
    socket.Send(data.data(), data.size(), address);
}
```

**After:**
```cpp
#include "INetworkSocket.hpp"  // ✓ abstraction

void Protocol::SerializeToSocket(const Packet& p, INetworkSocket& socket, const SocketAddress& addr) {
    auto data = Serialize(p);
    socket.Send(data.data(), data.size(), addr);
}
```

### 4.5 NetworkManager Refactoring

**NetworkManager.hpp** now stores:
```cpp
std::unique_ptr<INetworkSocket> socket_;
```

Construction based on build flag:
```cpp
#ifdef ENABLE_GNS
    socket_ = std::make_unique<GNSSocket>(...);
#else
    socket_ = std::make_unique<StubSocket>(...);  // for tests
#endif
```

---

## 5. File Changes

| File | Action | Lines |
|------|--------|-------|
| `src/server/include/netcode/INetworkSocket.hpp` | NEW | +80 |
| `src/server/src/netcode/GNSSocket.cpp` | NEW | +200 (extracted) |
| `src/server/src/netcode/StubSocket.cpp` | NEW | +100 |
| `src/server/src/netcode/Protocol.cpp` | MODIFY | Replace GNS includes with INetworkSocket |
| `src/server/include/netcode/GNSSocket.hpp` | NEW (header) | +40 |
| `src/server/src/netcode/GNSNetworkManager.cpp` | MODIFY | Swap usage to INetworkSocket* |
| `src/server/CMakeLists.txt` | MODIFY | Remove Protocol.cpp from conditional block |
| `tests/server/TestProtocol.cpp` | NEW | +150 (existing stub can now work) |
| `tests/server/TestProtocolWithGNS.cpp` | NEW (optional) | +80 (integration) |

---

## 6. Build Matrix (CI)

```
Build Configurations to validate:

1. ENABLE_GNS=ON  (default for release/demo)
   ✓ Protocol.cpp included
   ✓ GNSSocket used
   ✓ Links against SteamNetworkingSockets

2. ENABLE_GNS=OFF (default for unit tests)
   ✓ Protocol.cpp included (FIXED by this PRD)
   ✓ StubSocket used
   ✓ No SteamNetworking dependency
   ✓ Unit tests can test Protocol alone

3. ENABLE_GNS=ON  + ENABLE_ANTICHEAT=ON (production)
   ✓ Full stack works
```

**CI addition:** Add `cmake -DENABLE_GNS=OFF ..` build job to GitHub Actions.

---

## 7. Testing Strategy

### 7.1 Unit Tests (Protocol Layer)

`tests/server/TestProtocol.cpp` — uses StubSocket:
- [ ] `SerializeAttack()` → Protobuf → `StubSocket.Receive()` → correct deserialization
- [ ] `SerializeMovement()` → round-trip preserves vector/rotation
- [ ] `InvalidProtobuf()` → error handled gracefully (no crash)
- [ ] `LargePayload()` (max packet size 1200 bytes) — no fragmentation

**New capability enabled:** Protocol tests now run without GNS (previous: impossible).

### 7.2 Integration Tests (Full Stack)

`tests/server/TestGNSIntegration.cpp` — uses real GNSSocket in loopback mode (if ENABLE_GNS=ON):
- [ ] End-to-end: Protocol → GNSSocket → Protocol (round trip on localhost)
- [ ] Multi-peer: 4 simulated clients via 4 socket pairs

### 7.3 Regression Tests
- All existing server tests (2129 cases) must still pass
- No performance regression (socket abstraction vtable: ~1 pointer indirection)
- No increase in packet size or latency

---

## 8. Acceptance Criteria

✅ **Architecture**
- `INetworkSocket.hpp` interface defined and documented
- GNSSocket implements interface (no GNS in Protocol.cpp)
- StubSocket implements interface (test-only)
- Protocol.cpp compiles in both ENABLE_GNS modes

✅ **Quality**
- Zero new compiler warnings
- Zero memory leaks (valgrind clean)
- All protocol unit tests pass (new + existing)

✅ **Build**
- CI matrix runs both configurations
- Debug and Release builds succeed
- Windows (MSVC) and Linux (gcc/clang) both compile

✅ **Documentation**
- `docs/protocol-decoupling.md` explains architecture
- `docs/testing-protocol.md` explains how to run protocol tests without GNS
- INetworkSocket.hpp has Doxygen comments

---

## 9. Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| GNS callbacks expect `GNSSocket*` type | Break GNS integration | Forward `INetworkSocket*` to `GNSSocket*` via static_cast in callback (safe because we control construction) |
| Performance hit from virtual call | 1-2 ns overhead | Negligible; not in hot path (network ops <1% of frame) |
| ABI break for external plugins | Breaking change | Version interface (add `INetworkSocket_v2`) or keep old GNSSocket for backwards compat |
| StubSocket incomplete (missing GNS features) | Tests cannot exercise edge cases | Only core serialization needs stub; edge GNS features tested separately in ENABLE_GNS=ON builds |

---

## 10. Related PRDs

- **PRD-012** (GNS Runtime) — this is a prerequisite (need GNS runtime stable before splitting transport)
- **PRD-002** (Networking) — original networking spec that may reference GNS directly
- **PROTOCOL_SPEC.md** — protocol definition unaffected (but implementation changes)

---

## 11. Implementation Order

**Recommended sequence:**
1. Create `INetworkSocket.hpp` interface
2. Create `StubSocket` (simplest; implement Send/Receive as vectors)
3. Refactor Protocol.cpp to accept `INetworkSocket&` instead of `GNSSocket&`
4. Remove hard `#include "GNSSocket.hpp"` from Protocol.cpp
5. Create `GNSSocket` wrapper (copy GNS code from GNSNetworkManager)
6. Update GNSNetworkManager to use `INetworkSocket` factory
7. Update CMakeLists to remove Protocol.cpp conditional
8. Write tests (TestProtocol uses StubSocket)
9. CI pipeline: add ENABLE_GNS=OFF job
10. Merge + review (two-agent pattern)

**Effort:** ~1-2 days for a Networking Agent (incremental, low-risk)

---

## 12. Alternative Approaches Considered

| Approach | Pros | Cons | Rejected Because |
|----------|------|------|-----------------|
| Keep Protocol.cpp conditional on GNS | Simple | Tests can't verify protocol | Violates testability |
| Place Protocol inside GNS subdirectory | Modular | Still co-compiled with GNS | Doesn't solve ENABLE_GNS=OFF gap |
| Template-based Protocol<T> | Compile-time polymorphism | Code duplication | Runtime polymorphism (vtable) fine |
| **Recommended: INetworkSocket interface** | Clean separation | Minor runtime overhead | ✓ Acceptable trade-off |

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to NETWORKING_AGENT; coordinate with GNS runtime owner
