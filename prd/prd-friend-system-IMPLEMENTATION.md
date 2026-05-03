# PRD: Friend System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-friend-system.md`
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P1 (Multiplayer Core)

## Implementation Checklist

### Server: FriendSystem.hpp
Create `src/server/src/social/FriendSystem.hpp`:
```cpp
namespace DarkAges::social {

enum class FriendStatus : uint8_t {
  Pending = 0,   // Request sent, waiting
  Accepted = 1,  // Friends
  Blocked = 2,   // Blocked
};

struct FriendEntry {
  uint64_t player_id;
  uint64_t friend_id;
  FriendStatus status;
  std::chrono::system_clock::time_point created_at;
};

class FriendSystem {
public:
  void SendFriendRequest(uint64_t sender_id, uint64_t target_id);
  void AcceptFriendRequest(uint64_t player_id, uint64_t requester_id);
  void RemoveFriend(uint64_t player_id, uint64_t friend_id);
  void BlockPlayer(uint64_t player_id, uint64_t blocked_id);
  void UnblockPlayer(uint64_t player_id, uint64_t blocked_id);
  auto GetFriends(uint64_t player_id) -> std::vector<FriendEntry>;
  auto IsOnline(uint64_t player_id) -> bool;
};

}
```

### Server: FriendSystem.cpp
Create implementation:
- SendFriendRequest: create pending entry
- AcceptFriendRequest: update to Accepted
- RemoveFriend: delete entry
- Block/Unblock: manage blocked list
- GetFriends: return all accepted
- Presence tracking: online/offline status

### Packet Types
- PACKET_FRIEND_REQUEST = 50
- PACKET_FRIEND_ACCEPT = 51
- PACKET_FRIEND_REMOVE = 52
- PACKET_FRIEND_STATUS = 53 // Online/offline notification

### Client: FriendsList.tscn
Create UI panel:
- Friends list with online status
- Add friend button
- Remove/Block context menu

## Acceptance Criteria
- [ ] Server implementation exists
- [ ] Friend request/accept works
- [ ] Online status displays in client
- [ ] Blocked players can't send messages