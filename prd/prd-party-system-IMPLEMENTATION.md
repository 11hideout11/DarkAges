# PRD: Party System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED** - This PRD is for actual implementation.

## Gap Analysis
- **PRD File**: `prd/prd-party-system.md` (existed)
- **Server Code**: NONE in `src/server/src/`
- **Client Code**: NONE in `src/client/`
- **Priority**: P1 (Multiplayer Core)

## Implementation Checklist

### Server: PartySystem.hpp
Create `src/server/src/social/PartySystem.hpp`:
```cpp
namespace DarkAges::social {

struct PartyComponent {
  uint64_t party_id;
  uint64_t leader_id; // Party leader
  std::vector<uint64_t> members;
  std::chrono::system_clock::time_point created_at;
};

struct PartyMemberComponent {
  uint64_t player_id;
  uint64_t party_id;
  bool is_leader;
};

class PartySystem {
public:
  void CreateParty(uint64_t leader_id);
  void InviteToParty(uint64_t leader_id, uint64_t player_id);
  void AcceptInvite(uint64_t player_id, uint64_t party_id);
  void RemoveMember(uint64_t party_id, uint64_t player_id);
  void LeaveParty(uint64_t player_id);
  auto GetPartyMembers(uint64_t party_id) -> std::vector<uint64_t>;
  auto GetPlayerParty(uint64_t player_id) -> std::optional<uint64_t>;
};

}
```

### Server: PartySystem.cpp
Create `src/server/src/social/PartySystem.cpp`:
- CreateParty: leader becomes party member
- InviteToParty: send invitation, 60s timeout
- AcceptInvite: add to party, sync to all members
- RemoveMember: remove, promote new leader if needed
- LeaveParty: leave, hand off leadership if leader leaves
- Party buffs shared across members (future)

### Packet Types
- PACKET_PARTY_CREATE = 40
- PACKET_PARTY_INVITE = 41
- PACKET_PARTY_ACCEPT = 42
- PACKET_PARTY_REMOVE = 43
- PACKET_PARTY_SYNC = 44

### Client: PartyUI
Create `src/client/scenes/PartyFrame.tscn`:
- Party member list with health bars
- Invite button
- Leave button

Create `src/client/scripts/PartyFrame.cs`:
- Process PACKET_PARTY_* packets
- Display party UI

## Acceptance Criteria - IMPLEMENTATION VERIFIED

- [ ] `src/server/src/social/PartySystem.hpp` exists
- [ ] `src/server/src/social/PartySystem.cpp` has implementation
- [ ] Party creation works: `/party` command
- [ ] Invite/accept/kick works
- [ ] Party health bars sync in UI
- [ ] Integration test: 2 players can form party

## Technical Notes
- Party leader has invite/kick permissions
- Max party size: 4 players (configurable)
- Reuse ZoneServer for member presence