# PRD: Trade System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-trade-system.md`
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Server: TradeSystem.hpp
Create `src/server/src/economy/TradeSystem.hpp`:
```cpp
namespace DarkAges::economy {

enum class TradeState {
  Idle = 0,
  Pending = 1,      // Offer sent, waiting acceptance
  Active = 2,        // Both players in trade window
  Accepted = 3,       // Both accepted
  Completed = 4,
  Cancelled = 5
};

struct TradeOffer {
  uint64_t player_id;
  std::vector<InventoryItem> items;
  uint64_t currency;
  bool accepted;
};

struct TradeSession {
  uint64_t trade_id;
  uint64_t player1_id;
  uint64_t player2_id;
  TradeOffer offer1;
  TradeOffer offer2;
  TradeState state;
  std::chrono::system_clock::time_point started_at;
};

class TradeSystem {
public:
  void SendTradeRequest(uint64_t sender_id, uint64_t target_id);
  void AcceptTradeRequest(uint64_t player_id, uint64_t trade_id);
  void AddItem(uint64_t player_id, uint32_t slot, uint32_t quantity);
  void RemoveItem(uint64_t player_id, uint32_t slot);
  void AddCurrency(uint64_t player_id, uint64_t amount);
  void AcceptTrade(uint64_t player_id);
  void CancelTrade(uint64_t player_id);
  void CompleteTrade(uint64_t trade_id);
};

}
```

### Server: TradeSystem.cpp
Implementation:
- SendTradeRequest: create pending trade, notify target
- AcceptTradeRequest: transition to Active
- AddItem: move from inventory to trade offer (not removed until complete)
- AcceptTrade: both must accept to complete
- CancelTrade: return items to inventory
- CompleteTrade: swap items between players

### Packet Types
- PACKET_TRADE_REQUEST = 90
- PACKET_TRADE_ACCEPT = 91
- PACKET_TRADE_ADD_ITEM = 92
- PACKET_TRADE_ACCEPTED = 93
- PACKET_TRADE_COMPLETE = 94
- PACKET_TRADE_CANCEL = 95

### Client: TradeWindow.tscn
Create `src/client/scenes/TradeWindow.tscn`:
- Player's offer panel (drag items here)
- Target's offer panel (view only)
- Accept button
- Cancel button
- Trade status label

## Acceptance Criteria
- [ ] Trade request sent and UI appears
- [ ] Items added to offer and visible
- [ ] Both players must accept to complete
- [ ] Items swap on completion
- [ ] Cancel returns items