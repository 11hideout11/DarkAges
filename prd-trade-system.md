# PRD: Trade System Implementation

## Introduction

DarkAges has a TradeSystem header but no implementation. Players cannot trade items with each other. This PRD implements player-to-player trading.

## Goals

- Trade request/accept flow
- Trade window UI
- Item exchange
- Gold exchange

## User Stories

### TRD-001: Trade Request
**Description:** As a player, I want to trade with another player.

**Acceptance Criteria:**
- [ ] Right-click player → Trade
- [ ] Other player sees request
- [ ] Accept/decline options
- [ ] Trade window opens

### TRD-002: Item Exchange
**Description:** As a player, I want to offer items.

**Acceptance Criteria:**
- [ ] Items displayed in trade window
- [ ] Drag items to trade slot
- [ ] Items locked when confirmed
- [ ] Both confirm to complete

### TRD-003: Trade Completion
**Description:** As a player, I want to complete a trade.

**Acceptance Criteria:**
- [ ] Both players confirm
- [ ] Items transferred
- [ ] Gold transferred
- [ ] Trade window closes

## Functional Requirements

- FR-1: TradeRequestPacket
- FR-2: TradeConfirmPacket
- FR-3: TradeComplete event
- FR-4: TradeWindow UI

## Non-Goals

- No trade auctions
- No trade logs
- No anti-trade scam (both confirm)

## Technical Considerations

### Trade State Machine
```cpp
enum class TradeState { None, Requested, Accepted, Confirming, Completed };
```

### Packets
- PACKET_TRADE_REQUEST (type=50)
- PACKET_TRADE_ACCEPT (type=51)
- PACKET_TRADE_CONFIRM (type=52)
- PACKET_TRADE_COMPLETE (type=53)

## Success Metrics

- Trade requested and accepted
- Items transferred correctly

## Open Questions

- Trade cooldown?
- Max trade gold?