# PRD: Protobuf Protocol Implementation
## Introduction
The `Protocol.cpp` implementation is excluded when `ENABLE_GNS=OFF`, preventing Protobuf message handling from being tested in standard builds. This creates a critical gap: the production protocol cannot be validated without GNS, but GNS cannot be enabled without resolving the WebRTC blocker.
**Problem Statement:** Binary protocol serialization is untested in CI, creating risk of wire format incompatibilities in production.
---


## Goals
- Enable Protobuf protocol testing in all build configurations
- Maintain backward compatibility with custom UDP wire format
- Provide clear migration path from stub to Protobuf
- Validate wire format end-to-end
- Document protocol versioning strategy
---


## User Stories
### US-001: Enable Protocol. cpp in Test Builds
**Description:** As a developer, I need Protocol.cpp to compile in test builds so I can validate the wire format.
**Acceptance Criteria:**
- [ ] Protocol.cpp compiles with `ENABLE_GNS=OFF`
- [ ] Uses GNS types only when available (via conditional compilation)
- [ ] All protocol tests pass
- [ ] No GNS dependency for basic protocol validation


### US-002: Maintain Stub Compatibility
**Description:** As a system designer, I need the stub protocol to remain compatible with Protobuf messages.
**Acceptance Criteria:**
- [ ] Stub and Protobuf produce identical wire output
- [ ] Client test validates both paths
- [ ] Migration documented with rollback option
- [ ] Version field in all protocol messages


### US-003: Document Protocol Schema
**Description:** As a developer, I need complete schema documentation so I can add new message types.
**Acceptance Criteria:**
- [ ] All proto messages documented
- [ ] Wire format specifications exist
- [ ] Backward compatibility rules defined
- [ ] Migration guide for adding new fields
---


## Functional Requirements
- FR-1: Decouple Protocol.cpp from GNS headers
- FR-2: Implement conditional Protobuf/GNS type mapping
- FR-3: Add wire format validation tests
- FR-4: Document complete schema in `docs/NETWORK_PROTOCOL.md`
- FR-5: Implement version negotiation
- FR-6: Provide protocol migration tooling
---


## Non-Goals
- No breaking changes to existing wire format
- No removal of stub layer
- No Protobuf-only production requirement
- No GNS requirement for protocol testing
---


## Technical Considerations
### Current Architecture
```
Test Builds: ENABLE_GNS=OFF → Stub UDP
GNS Builds:   ENABLE_GNS=ON  → GNSNetworkManager + Protocol.cpp
```

### Proposed Fix
```
All Builds:   Always include Protocol.cpp
             Conditionally use GNS types
             Stub layer uses same wire format
```

### Wire Format
- Position quantization: mm precision
- Rotation: 16-bit normalized quaternion
- Delta compression for entity state
- Reliable channel for events
---


## Open Questions
1. Should Protobuf be the single wire format?
2. What is the deprecation path for stub?
3. Is full protocol version negotiation required?
4. Should legacy clients be supported?
---


*Filename: `prd-protobuf-protocol-implementation.md`*  
*Created: 2026-05-01*