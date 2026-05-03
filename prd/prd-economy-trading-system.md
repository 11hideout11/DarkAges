# PRD: Economy & Trading System

## Introduction

Implement a comprehensive economy system that handles gold/currency management, player-to-player trading, auction house/marketplace, NPC vendors with buy/sell pricing, item value schemas, and gold sinks. The system should support both player-driven and NPC-driven commerce.

## Goals

- Add CurrencyComponent storing gold, platinum (future), bind currency
- Implement player-to-player trade system with trade request/accept
- Add NPC vendor system with buy/sell prices
- Add marketplace/auction house for item listings
- Integrate with existing InventoryComponent
- Add gold sinks (repair, teleport, crafting fees)

## User Stories

### US-001: Currency management
**Description:** As a player, I want to manage my gold so I can purchase items and services.

**Acceptance Criteria:**
- [ ] Gold storage with max cap (999,999,999)
- [ ] Gold displays in UI with proper formatting (1K, 1M, 1B)
- [ ] Currency gained/lost tracked in transaction log
- [ ] Gold loss on death (if enabled in death penalties)
- [ ] Insufficient gold feedback message

### US-002: Player-to-player trading
**Description:** As a player, I want to trade items with other players so I can acquire desired items.

**Trade acceptance criteria:**
- [ ] Trade request sent → accept/decline flow
- [ ] Both players see offered items simultaneously
- [ ] Gold can be included in trade
- [ ] Trade committed only when both confirm
- [ ] Trade canceled if either logs off
- [ ] Trade window shows item stats

### US-003: NPC vendors
**Description:** As a player, I want to buy and sell items to NPCs so I can acquire gear and sell unwanted items.

**Acceptance Criteria:**
- [ ] Vendor inventory with item templates
- [ ] Buy prices: base_value × price_multiplier
- [ ] Sell prices: base_value × sell_multiplier (typically 0.5)
- [ ] Vendor UI shows available purchases
- [ ] Sell tab shows inventory with sell prices
- [ ] Stock limits per vendor (optional)

### US-004: Marketplace/Auction House
**Description:** As a player, I want to list items for sale so I can reach more buyers.

**Acceptance Criteria:**
- [ ] Auction listing: item, price, duration (12h, 24h, 48h)
- [ ] Listing fee: 1% of asking price (refundable on sale)
- [ ] Browse by category, level, price range
- [ ] Purchase completes instantly (buyout) or bid (auction)
- [ ] Expire unclaimed items → returns to seller (minus fee)
- [ ] Cross-zone listing (global)

### US-005: Gold sinks
**Description:** As a game designer, I want gold sinks so the economy stays balanced.

**Acceptance Criteria:**
- [ ] Repair costs: durability lost × repair_cost_per_durability
- [ ] Teleport costs: distance-based
- [ ] Crafting fees: material value + skill fee
- [ ] Marketplace listing fee
- [ ] Guild creation cost
- [ ] Level-up service fee (optional)

## Functional Requirements

- FR-1: CurrencyComponent with gold tracking
- FR-2: TradeStateMachine for trade workflow
- FR-3: TradeWindowComponent with offered items
- FR-4: VendorComponent with inventory, markup
- FR-5: MarketplaceComponent with listings, bids
- FR-6: TransactionLogComponent (recent transactions)
- FR-7: GoldSinkComponent for repair, teleport, etc.

## Non-Goals

- No real-money trading (RMT) - prohibited
- No player shops (individual vendor windows)
- No auction house bots
- No item rental or leases

## Technical Considerations

- Trade uses lock-based entity access
- Marketplace listings in database (Redis/Scylla)
- Vendor templates loaded from data/items.json

## Success Metrics

- Trade processes in < 1 second
- Marketplace handles 1000+ listings
- No duping exploits in trade

## Open Questions

- Item valuation: calculated or static?
- Marketplace moderation: manual or auto?
- Tax on player sales: 0%, 5%, 10%?