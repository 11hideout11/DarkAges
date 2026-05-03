# PRD: Server-Client API Contracts

## Introduction

Formalize and document all server-client communication contracts, including packet types, message formats, sequence diagrams, expected behaviors, and error handling. This ensures both server and client implementations stay synchronized and enables independent development.

## Goals

- Document all packet types in a central location
- Define request/response formats for each operation
- Create sequence diagrams for critical flows
- Document expected error codes and handling
- Add protocol versioning for backwards compatibility
- Create a reference document for developers

## User Stories

### US-001: Central packet documentation
**Description:** As a developer, I want all packet types documented so I know what to send/receive.

**Acceptance Criteria:**
- [ ] docs/NETWORK_PROTOCOL.md updated with all packets
- [ ] Each packet: ID, name, fields, direction (C→S, S→C)
- [ ] Packet field types and sizes documented
- [ ] Example packet content shown
- [ ] Version history for packet changes

### US-002: Login flow contracts
**Description:** As a developer, I want login flow contracts so I can implement authentication.

**Acceptance Criteria:**
- [ ] C->S: LOGIN_REQUEST (account, password_hash)
- [ ] S->C: LOGIN_RESPONSE (success/fail, error_code, player_list)
- [ ] C->S: CHARACTER_SELECT (character_id)
- [ ] S->C: CHARACTER_SELECT_RESPONSE (snapshot)
- [ ] Error codes: AUTH_FAILED, ACCOUNT_LOCKED, SERVER_FULL

### US-003: Movement sync contracts
**Description:** As a developer, I want movement contracts so I can implement position sync.

**Acceptance Criteria:**
- [ ] C->S: MOVE_INPUT (direction, timestamp, sequence)
- [ ] S->C: PLAYER_MOVE (entity_id, position, rotation, timestamp)
- [ ] S->C: ENTITY_SPAWN (full entity snapshot)
- [ ] S->C: ENTITY_DESPAWN (entity_id)
- [ ] Compression for bulk entity updates

### US-004: Combat event contracts
**Description:** As a developer, I want combat contracts so I can implement combat interactions.

**Acceptance Criteria:**
- [ ] C->S: ATTACK_REQUEST (target_id, ability_id)
- [ ] S->C: COMBAT_EVENT (damage, effects, attacker_id)
- [ ] S->C: DEATH_EVENT (entity_id, killer_id)
- [ ] S->C: ABILITY_CAST (caster_id, ability_id, target)
- [ ] Combat text contracts integrated

### US-005: Inventory/Equipment contracts
**Description:** As a developer, I want inventory contracts so I can implement item management.

**Acceptance Criteria:**
- [ ] C->S: INVENTORY_REQUEST
- [ ] S->C: INVENTORY_UPDATE (slot, item_id, count)
- [ ] C->S: ITEM_MOVE (from_slot, to_slot)
- [ ] S->C: ITEM_EQUIP (slot, equipment)
- [ ] S->C: CURRENCY_UPDATE (gold_amount)

## Functional Requirements

- FR-1: NetworkProtocol.md comprehensive document
- FR-2: All packet types enumerated with IDs
- FR-3: Request/response pairs documented
- FR-4: Error codes defined per operation
- FR-5: Sequence diagrams for login, movement, combat, trade
- FR-6: Version header in all packets

## Non-Goals

- No external API (third-party integrations)
- No WebSocket or HTTP endpoints
- No gRPC or REST API

## Technical Considerations

- Packets use Protocol (Protobuf or FlatBuffers)
- Sequence numbers for ordering
- Ack system for reliability

## Success Metrics

- All server sends documented
- All client sends documented
- No undocumented packets in production

## Open Questions

- FlatBuffers or Protobuf for wire format?
- Compression level for updates?
- Max packet size?