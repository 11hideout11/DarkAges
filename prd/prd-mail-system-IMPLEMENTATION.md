# PRD: Mail System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-mail-system.md`
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Server: MailSystem.hpp
Create `src/server/src/social/MailSystem.hpp`:
```cpp
namespace DarkAges::social {

struct MailMessage {
  uint64_t message_id;
  uint64_t sender_id;
  std::string sender_name;
  uint64_t recipient_id;
  std::string subject;
  std::string body;
  std::vector<InventoryItem> attachments;
  bool read;
  std::chrono::system_clock::time_point sent_at;
  std::chrono::system_clock::time_point expires_at;
};

class MailSystem {
public:
  void SendMail(const MailMessage& message);
  void DeleteMail(uint64_t message_id);
  void MarkAsRead(uint64_t message_id);
  auto GetInbox(uint64_t player_id) -> std::vector<MailMessage>;
  auto GetUnreadCount(uint64_t player_id) -> uint32_t;
  void DeliverOfflineMail(); // Process queued mail for offline players
};

}
```

### Server: MailSystem.cpp
- SendMail: store message, notify recipient if online
- DeleteMail: remove message
- MarkAsRead: set read flag
- GetInbox: return all mail for player
- DeliverOfflineMail: on player login, deliver queued mail

### Packet Types
- PACKET_MAIL_SEND = 100
- PACKET_MAIL_INBOX = 101
- PACKET_MAIL_DELETE = 102
- PACKET_MAIL_NEW = 103 // Notification of new mail

### Client: MailUI.tscn
Create `src/client/scenes/MailUI.tscn`:
- Inbox list
- Mail view panel
- Compose button
- Attachment display

## Acceptance Criteria
- [ ] Send mail to another player
- [ ] Include item attachments
- [ ] Receive notification of new mail
- [ ] Inbox shows all mail
- [ ] Offline delivery on login