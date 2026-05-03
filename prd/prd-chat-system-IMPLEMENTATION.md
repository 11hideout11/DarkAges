# PRD: Chat System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED** - This PRD is for actual implementation.

## Gap Analysis
- **PRD File**: `prd/prd-chat-social-system.md`
- **Server Code**: PARTIAL - basic packets in NetworkManager but no chat system
- **Client Code**: PARTIAL - UI.tscn has basic chat
- **Priority**: P1 (Multiplayer Core)

## Implementation Checklist

### Server: ChatSystem.hpp
Create `src/server/src/social/ChatSystem.hpp`:
```cpp
namespace DarkAges::social {

enum class ChatChannel : uint8_t {
  Say = 0,      // Local to zone
  Shout = 1,   // Zone-wide
  Guild = 2,    // Guild channel (future)
  Party = 3,   // Party channel (future)
  Whisper = 4,  // Direct message
  System = 5,   // System announcements
};

struct ChatMessage {
  uint64_t sender_id;
  std::string sender_name;
  ChatChannel channel;
  std::string message;
  uint64_t target_id; // For whisper
  std::chrono::system_clock::time_point timestamp;
};

class ChatSystem {
public:
  void SendMessage(const ChatMessage& msg);
  void ProcessMessage(uint64_t sender_id, ChatChannel channel, 
                      const std::string& message, std::optional<uint64_t> target_id);
  auto GetChannelHistory(ChatChannel channel, uint32_t limit = 100) 
    -> std::vector<ChatMessage>;
};

}
```

### Server: ChatSystem.cpp
Create `src/server/src/social/ChatSystem.cpp`:
- ProcessMessage routing by channel
- Rate limiting per player (max 5 msg/sec)
- Profanity filter (configurable word list)
- History management (last 100 per channel)

### Packet Types (extend Protocol.cpp)
- PACKET_CHAT_MESSAGE = 25 (existing, extend)
- PACKET_CHAT_HISTORY = 26
- PACKET_CHAT_WHISPER = 27

### Client: ChatBox.tscn Enhancement
Enhance existing `src/client/scenes/UI.tscn` chat:
- Add channel tabs: All, Say, Guild, Party
- Timestamp display
- Player name click for whisper
- Chat input with channel selector

## Acceptance Criteria - IMPLEMENTATION VERIFIED

- [ ] `src/server/src/social/ChatSystem.hpp` exists
- [ ] `/say` and `/shout` commands work
- [ ] `/whisper <player> <message>` works
- [ ] Rate limiting prevents spam
- [ ] Client shows channel tabs
- [ ] Chat history scrolls properly

## Technical Notes
- Say/Shout are already partially working
- Focus on Guild/Party channels as extension
- Link with existing ReputationComponent for Shout filtering