# PRD: GNS Runtime Integration Workaround

## Introduction

The GNS (Game Networking Socket) Runtime integration is blocked by WebRTC submodule authentication failure (`webrtc.googlesource.com` access denied). The compile-time fix is complete but runtime fails to connect. This PRD specifies workarounds to achieve GNS functionality without external WebRTC auth.

## Goals

- Document the blocking issue clearly
- Provide UDP fallback that works without GNS
- Document path to enable GNS when auth is available
- Maintain protocol compatibility for future GNS activation

## User Stories

### US-001: Issue Documentation
**Description:** As a developer, I need clear documentation of the WebRTC auth blocker so I can communicate status.

**Acceptance Criteria:**
- [ ] Root cause documented: webrtc.googlesource.com requires Google account auth
- [ ] Error message captured from git clone failure
- [ ] Workaround path documented for future developers

### US-002: UDP Fallback Operational
**Description:** As an operator, I need the UDP fallback to work so the game is playable.

**Acceptance Criteria:**
- [ ] UDP socket implementation uses native BSD sockets (PR #57 merged)
- [ ] Protocol.cpp uses FlatBuffers only (PRD-017 complete)
- [ ] Games function without GNS (current demo mode works)
- [ ] Test suite passes with GNS disabled (100% pass rate)

### US-003: GNS Readiness
**Description:** As a developer, I want GNS to be ready to enable when auth is resolved.

**Acceptance Criteria:**
- [ ] Build compiles with GNS enabled (currently passes)
- [ ] ProtobufProtocol.cpp fully implemented
- [ ] NetworkManager has GNS send wrappers
- [ ] When auth available, clone and build succeeds

### US-004: Migration Path
**Description:** As an operator, I want a documented path to switch to GNS when ready.

**Acceptance Criteria:**
- [ ] CMake flag `-DENABLE_GNS=ON` documented
- [ ] Build instructions updated with GNS path
- [ ] Release notes mention GNS as "coming soon"

## Functional Requirements

- FR-1: Document WebRTC auth requirement in README.md
- FR-2: Document UDP-only mode in deployment docs
- FR-3: Keep GNS code in tree (no deletion)
- FR-4: Test GNS build periodically (CI should build both modes)
- FR-5: Create internal ticket for WebRTC auth token request

## Non-Goals

- No bypass of WebRTC auth (respect Google's terms)
- No alternative WebRTC implementation
- No timeline for auth resolution (external)
- No changes to production setup (dev environment only)

## Technical Considerations

- Custom UDP stub fully functional (used in demo)
- FlatBuffers protocol works with both UDP and GNS
- GNS requires auth token from webrtc.googlesource.com
- Auth token request should go through project lead

## Success Metrics

- Demo runs with UDP fallback (verified in demo pipeline)
- GNS build compiles but fails at runtime with auth issue
- Documentation prevents confusion about "broken" network code

## Open Questions

- Should we explore alternative networking (like libuv, custom QUIC)?
- Is there a corporate WebRTC license available?
- Priority for GNS vs other features?

---

*Generated: 2026-05-03*