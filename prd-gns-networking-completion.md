# PRD: GNS Networking Integration Completion

## Introduction

GameNetworkingSockets (GNS) integration is critical for production-grade multiplayer networking. The current implementation is blocked by WebRTC submodule access restrictions, leaving the project dependent on a stub UDP layer that may behave differently from the production Steam Networking implementation.

**Problem Statement:** Without GNS, the server cannot leverage Steam's production-tested networking features including reliable channels, encryption, NAT punch-through, and bandwidth management.

---

## Goals

- Enable full GNS integration for production builds
- Provide alternative networking path when Steam libraries unavailable
- Maintain test compatibility with CI/CD pipelines
- Achieve <10ms connection establishment
- Support 10,000 concurrent connections with 99.99% packet delivery

---

## User Stories

### US-001: Enable Production GNS Builds
**Description:** As a DevOps engineer, I need production builds to use GNS so the server leverages Steam's production networking.

**Acceptance Criteria:**
- [ ] `ENABLE_GNS=ON` builds complete without WebRTC dependency errors
- [ ] GNSNetworkManager initializes successfully
- [ ] Connections established via SteamNetworkingSockets API
- [ ] Fallback stub used only when GNS unavailable

### US-002: Provide WebRTC Alternative
**Description:** As a developer, I need an alternative WebRTC source so GNS builds succeed without Steam access.

**Acceptance Criteria:**
- [ ] Alternative WebRTC implementation documented
- [ ] CMake fetches from mirror or bundled source
- [ ] Build succeeds with `-DENABLE_GNS=ON`
- [ ] All GNS tests pass with alternative WebRTC

### US-003: Maintain CI/CD Compatibility
**Description:** As a CI engineer, I need GNS-enabled builds to work in automated pipelines without interactive authentication.

**Acceptance Criteria:**
- [ ] CI builds with `ENABLE_GNS=ON` complete successfully
- [ ] No interactive git/hg authentication required
- [ ] Docker builds use pre-configured GNS dependencies
- [ ] Test coverage includes GNS-specific code paths

---

## Functional Requirements

- FR-1: Implement alternative WebRTC source resolution (mirror or embedded)
- FR-2: Create CMake fallback logic when WebRTC unavailable
- FR-3: Update GNSNetworkManager to use fallback gracefully
- FR-4: Document production build requirements clearly
- FR-5: Add GNS integration tests to test suite
- FR-6: Provide Docker image with GNS pre-configured

---

## Non-Goals

- No Steam SDK integration beyond GNS
- No dedicated server license management
- No Steam matchmaking implementation
- No GNS feature removal from non-GNS builds

---

## Technical Considerations

### Current Blockers
- WebRTC submodule: `webrtc.googlesource.com` access denied
- CMake FetchContent fails silently
- GNSNetworkManager.cpp excluded when `ENABLE_GNS=OFF`

### Resolution Options
1. **Embedded WebRTC:** Bundle minimal WebRTC in `deps/webrtc/`
2. **Mirror Repository:** Use GitHub mirror with public access
3. **Conditionally Stubbed:** Full fallback when WebRTC unavailable
4. **Document as Limitation:** Mark GNS as optional with clear tradeoffs

---

## Open Questions

1. Which WebRTC alternative is preferred (embedded vs mirror)?
2. Is Steam production deployment a hard requirement?
3. Should stub layer receive continued investment?
4. What latency/throughput targets are acceptable for stub?

---

*Filename: `prd-gns-networking-completion.md`*  
*Created: 2026-05-01*