# 1. OBJECTIVE

Complete PRD-012 GNS Runtime Integration to finalize the switch from custom UDP socket implementation to Valve's GameNetworkingSockets library, providing improved production reliability, encryption, and NAT traversal.

This is the primary remaining actionable item that meaningfully advances the project toward production readiness.

## Current Status (2026-05-02)
- **INetworkSocket interface**: ✅ Complete (270 lines, includes StubSocket + GNSSocket classes + factory)
- **StubSocket implementation**: ✅ Complete (in-memory stub for tests)
- **GNSSocket implementation**: ❌ STUB - returns false on all operations (lines 206-254 in INetworkSocket.cpp)
- **GNS compile**: ✅ Fixed in Phase 8
- **GNS runtime**: ❌ Pending - needs real GNS library integration

## Gap Analysis
The GNSSocket class exists but is a placeholder stub. It needs to be implemented with real GNS APIs from the games_networking_sockets library.

# 2. CONTEXT SUMMARY

## Project Overview
- **Server**: C++20, EnTT ECS, 60Hz tick, CMake build
- **Client**: Godot 4.2.4 Mono (C#)
- **Tests**: 2129 cases, 12644 assertions, 100% passing

## Key Files Already Complete
| File | Purpose | Status |
|------|---------|--------|
| `src/server/include/netcode/INetworkSocket.hpp` | Interface (270 lines) | ✅ COMPLETE |
| `src/server/src/netcode/INetworkSocket.cpp` | StubSocket + GNSSocket stubs | ⚠️ StubSocket works, GNSSocket empty |
| `CMakeLists.txt` | ENABLE_GNS=ON option | ✅ Complete |
| `Protocol.cpp` | Protocol serializers | ✅ Complete |

## Current Gap Analysis
The GNSSocket class exists but is a placeholder stub (lines 206-254 in INetworkSocket.cpp):
- `initialize()` returns false (gnsAvailable is false)
- All send/receive methods return false/empty
- Factory always returns StubSocket

The StubSocket is an in-memory test mock - it cannot actually send network packets.

## Dependencies
- ENABLE_GNS build option (CMakeLists.txt line 25)
- GNS library via FetchContent or local deps (compile fixed but runtime not wired)
- Existing NetworkManager uses socket layer but may not use INetworkSocket

## Test Baseline (Verified)
- 2129 test cases, 12644 assertions, 100% passing

# 3. APPROACH OVERVIEW

## Strategy: Real UDP Implementation First
Key insight: Currently StubSocket is an in-memory test stub that cannot send network packets. We need a real UDP implementation that matches the INetworkSocket interface.

**Primary approach**: Replace StubSocket with a real UDP implementation using BSD sockets, keeping the same interface.

**Future enhancement**: Swap in GNS library when available.

This approach:
- ✅ Works immediately with existing infrastructure
- ✅ Maintains test compatibility ( StubSocket behavior preserved via test injection)
- ✅ Provides same interface as GNS would use
- ✅ Can be tested independently
- ✅ Future GNS integration is just swapping implementation

## What's Already Done
- INetworkSocket interface (abstract class) ✅
- StubSocket class (in-memory test mock) ✅
- GNSSocket class (stub placeholder) ⚠️
- NetworkSocketFactory (returns appropriate implementation) ✅

# 4. IMPLEMENTATION STEPS

## Step 4.1: Implement Real UDP Socket
**Goal:** Replace the in-memory StubSocket with a real UDP implementation that can actually send/receive network packets
**Method:** Use BSD socket APIs (socket, bind, sendto, recvfrom) while maintaining the same INetworkSocket interface

Reference: `src/server/src/netcode/INetworkSocket.cpp` (lines 56-199 for structure)

Tasks:
1. Add socket includes: `<sys/socket.h>`, `<arpa/inet.h>`, `<unistd.h>`
2. Modify StubSocket::Impl to store file descriptor and address
3. Implement real `socket(AF_INET, SOCK_DGRAM, 0)` in initialize()
4. Implement `bind()` to bind to port
5. Use `sendto()` in sendUnreliable/sendReliable
6. Use `recvfrom()` in receive()
7. Add non-blocking I/O using poll() or fcntl(F_SETFL, O_NONBLOCK)
8. Handle EAGAIN/EWOULDBLOCK for non-blocking
9. Maintain connection tracking (UDP is stateless, track by address)
10. Keep existing test injection method (pushMessage) for backward compatibility

**Estimated:** 4 hours
**Risk:** MEDIUM - requires socket programming, but isolated to one file

## Step 4.2: Update Factory Configuration
**Goal:** Make the factory correctly detect available implementations
**Method:** Check for GNS availability and configure build flags

Reference: `src/server/src/netcode/INetworkSocket.cpp` (lines 260-283 for factory)

Tasks:
1. Update NetworkSocketFactory::isGNSAvailable() to check for GNS library
2. Add environment variable override for forcing implementation
3. Default to UDP (StubSocket renamed to UDPSocket) for development
4. Add --enable-gns command-line flag support

**Estimated:** 2 hours
**Risk:** LOW - factory code only

## Step 4.3: Integrate with NetworkManager
**Goal:** Wire INetworkSocket into the actual server network path
**Method:** Use socket layer in NetworkManager

Reference: `src/server/src/netcode/NetworkManager.cpp`

Tasks:
1. Check if NetworkManager already uses INetworkSocket
2. Add INetworkSocket* as member variable
3. Create via factory in initialize()
4. Replace direct sendto calls with socket->sendReliable()
5. Replace recvfrom loop with socket->receive()

**Estimated:** 3 hours
**Risk:** MEDIUM - need to verify backward compatibility

## Step 4.4: Test Validation
**Goal:** Verify the implementation works without regressions
**Method:** Run existing test suite

Tasks:
1. Build with ENABLE_GNS=OFF (UDP mode)
2. Run `ctest --output-on-failure`
3. Verify 2129 tests still pass
4. Validate no protocol changes

**Estimated:** 1 hour
**Risk:** LOW - validation only

# 5. TESTING AND VALIDATION

## Validation Criteria

### Step 4.1 (UDP Implementation)
- [ ] UDP socket can bind to specified port
- [ ] UDP socket can send packets to remote address
- [ ] UDP socket can receive packets
- [ ] Non-blocking mode works (poll returns ready)
- [ ] Test injection still works (pushMessage)

### Step 4.2 (Factory)
- [ ] Factory returns UDPSocket by default
- [ ] Environment variable overrides work
- [ ] GNS availability check functions

### Step 4.3 (NetworkManager Integration)
- [ ] NetworkManager uses INetworkSocket
- [ ] Packets sent via socket layer
- [ ] Packets received via socket layer

### Step 4.4 (Test Validation)
- [ ] ENABLE_GNS=OFF passes all 2129 tests
- [ ] No test regressions
- [ ] No protocol changes (wire format unchanged)
- [ ] Demo server works with UDP socket

## Test Baseline to Maintain
- 2129 test cases
- 12644 assertions
- 100% passing

---

**Plan Status:** Ready for implementation  
**Focus:** PRD-012 GNS Runtime Integration - implement real UDP socket
