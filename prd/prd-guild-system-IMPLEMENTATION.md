# PRD: Guild System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED** - This PRD is for actual implementation.

## Gap Analysis
- **PRD File**: `prd/prd-guild-system.md` (existed since 2024)
- **Server Code**: NONE in `src/server/src/`
- **Client Code**: NONE in `src/client/`
- **Priority**: P1 (Multiplayer Core)

## Implementation Checklist

### Server: GuildSystem.hpp
Create `src/server/src/social/GuildSystem.hpp`:
```cpp
namespace DarkAges::social {

struct GuildComponent {
  uint64_t guild_id;
  std::string name;
  std::string tagline;
  uint64_t leader_id;
  std::chrono::system_clock::time_point created_at;
};

struct GuildMemberComponent {
  uint64_t player_id;
  uint64_t guild_id;
  int rank; // 0=Recruit, 1=Member, 2=Officer, 3=Leader
  std::chrono::system_clock::time_point joined_at;
};

// Core operations
class GuildSystem {
public:
  void CreateGuild(uint64_t leader_id, const std::string& name, const std::string& tagline);
  void InvitePlayer(uint64_t guild_id, uint64_t player_id);
  void AcceptInvite(uint64_t player_id, uint64_t guild_id);
  void RemoveMember(uint64_t guild_id, uint64_t player_id);
  void PromoteMember(uint64_t guild_id, uint64_t player_id, int new_rank);
  auto GetGuildMembers(uint64_t guild_id) -> std::vector<GuildMemberComponent>;
  auto GetPlayerGuild(uint64_t player_id) -> std::optional<uint64_t>;
};

}
```

### Server: GuildSystem.cpp
Create `src/server/src/social/GuildSystem.cpp`:
- Implement CreateGuild with name uniqueness check
- Implement InvitePlayer with 60s timeout
- Implement AcceptInvite/RemoveMember/PromoteMember
- Emit guild events for chat integration

### Packet Types (extend Protocol.cpp)
- PACKET_GUILD_CREATE = 30
- PACKET_GUILD_INVITE = 31  
- PACKET_GUILD_ACCEPT = 32
- PACKET_GUILD_REMOVE = 33
- PACKET_GUILD_CHAT = 34

### Client: GuildUI
Create `src/client/scenes/GuildPanel.tscn`:
- Guild name/tagline display
- Member list with ranks
- Invite button (Leader/Officer only)
- Chat panel

Create `src/client/scripts/GuildPanel.cs`:
- Process PACKET_GUILD_* packets
- Display guild UI

## Acceptance Criteria - IMPLEMENTATION VERIFIED

- [ ] `src/server/src/social/GuildSystem.hpp` exists with GuildSystem class
- [ ] `src/server/src/social/GuildSystem.cpp` has 200+ lines implementation
- [ ] Guild creation works: `/create_guild <name>` command
- [ ] Member invite/accept/kick works
- [ ] Guild chat works: `/g <message>` cross-zone
- [ ] Client shows GuildPanel button in UI
- [ ] Integration test passes: 2 players can form guild

## Technical Notes
- Reuse existing ZoneServer for member online tracking
- Guild name stored in global map for uniqueness check
- Chat: extend PACKET_CHAT_MESSAGE with guild_channel
- Test: create GuildSystem test suite