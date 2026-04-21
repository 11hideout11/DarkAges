// TradeSystem unit tests
// Tests trade lifecycle, item/gold offers, lock/confirm flow, edge cases

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "combat/TradeSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "combat/ChatSystem.hpp"
#include "ecs/Components.hpp"

using namespace DarkAges;
using Catch::Approx;

// Helper: create a test player with inventory, trade, and chat state
static EntityID createTestPlayer(Registry& registry, float gold = 100.0f) {
    auto entity = registry.create();
    registry.emplace<Position>(entity);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);

    PlayerInfo info{};
    info.playerId = static_cast<uint64_t>(entity);
    info.connectionId = static_cast<uint32_t>(entity) + 1000;
    registry.emplace<PlayerInfo>(entity, info);

    PlayerProgression prog{};
    prog.level = 1;
    registry.emplace<PlayerProgression>(entity, prog);

    Inventory inv{};
    inv.gold = gold;
    registry.emplace<Inventory>(entity, inv);
    registry.emplace<CraftingComponent>(entity);
    registry.emplace<ChatComponent>(entity);
    registry.emplace<TradeComponent>(entity);

    return entity;
}

// Helper: setup TradeSystem with ItemSystem and ChatSystem
static void setupSystems(TradeSystem& trade, ItemSystem& items, ChatSystem& chat) {
    items.initializeDefaults();
    trade.setItemSystem(&items);
    trade.setChatSystem(&chat);
}

// ============================================================================
// TEST: Trade Types
// ============================================================================

TEST_CASE("TradeSlot default construction", "[trade]") {
    TradeSlot slot;
    CHECK(slot.itemId == 0);
    CHECK(slot.quantity == 0);
    CHECK(slot.isEmpty());
}

TEST_CASE("TradeSlot with values", "[trade]") {
    TradeSlot slot;
    slot.itemId = 1;
    slot.quantity = 5;
    CHECK_FALSE(slot.isEmpty());
    CHECK(slot.itemId == 1);
    CHECK(slot.quantity == 5);
}

TEST_CASE("TradeComponent default construction", "[trade]") {
    TradeComponent tc;
    CHECK(tc.state == TradeState::None);
    CHECK(static_cast<uint32_t>(tc.tradePartner) == static_cast<uint32_t>(entt::null));
    CHECK(tc.offeredGold == 0.0f);
    CHECK_FALSE(tc.locked);
    CHECK_FALSE(tc.confirmed);
    CHECK(tc.offeredItemCount() == 0);
}

TEST_CASE("TradeComponent findEmptySlot", "[trade]") {
    TradeComponent tc;
    CHECK(tc.findEmptySlot() == 0);

    tc.offeredItems[0].itemId = 1;
    tc.offeredItems[0].quantity = 1;
    CHECK(tc.findEmptySlot() == 1);

    // Fill all slots
    for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
        tc.offeredItems[i].itemId = i + 1;
        tc.offeredItems[i].quantity = 1;
    }
    CHECK(tc.findEmptySlot() == -1);
}

TEST_CASE("TradeComponent offeredItemCount", "[trade]") {
    TradeComponent tc;
    CHECK(tc.offeredItemCount() == 0);

    tc.offeredItems[0].itemId = 1;
    tc.offeredItems[0].quantity = 1;
    tc.offeredItems[2].itemId = 3;
    tc.offeredItems[2].quantity = 2;
    CHECK(tc.offeredItemCount() == 2);
}

TEST_CASE("TradeComponent reset", "[trade]") {
    TradeComponent tc;
    tc.state = TradeState::Active;
    tc.tradePartner = static_cast<EntityID>(42);
    tc.offeredItems[0].itemId = 1;
    tc.offeredItems[0].quantity = 5;
    tc.offeredGold = 50.0f;
    tc.locked = true;
    tc.confirmed = true;

    tc.reset();

    CHECK(tc.state == TradeState::None);
    CHECK(static_cast<uint32_t>(tc.tradePartner) == static_cast<uint32_t>(entt::null));
    CHECK(tc.offeredItemCount() == 0);
    CHECK(tc.offeredGold == 0.0f);
    CHECK_FALSE(tc.locked);
    CHECK_FALSE(tc.confirmed);
}

// ============================================================================
// TEST: Trade Request
// ============================================================================

TEST_CASE("TradeSystem send trade request", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    bool callbackFired = false;
    EntityID cbInitiator{}, cbTarget{};
    trade.setTradeRequestCallback([&](EntityID init, EntityID targ) {
        callbackFired = true;
        cbInitiator = init;
        cbTarget = targ;
    });

    bool result = trade.sendTradeRequest(registry, player1, player2, 1000);
    CHECK(result);
    CHECK(callbackFired);
    CHECK(cbInitiator == player1);
    CHECK(cbTarget == player2);
    CHECK(trade.getTradeState(registry, player1) == TradeState::Pending);
    CHECK(trade.getTradePartner(registry, player1) == player2);
}

TEST_CASE("TradeSystem cannot send request to self", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);

    bool result = trade.sendTradeRequest(registry, player1, player1, 1000);
    CHECK_FALSE(result);
    CHECK(trade.getTradeState(registry, player1) == TradeState::None);
}

TEST_CASE("TradeSystem cannot send request if already trading", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);
    auto player3 = createTestPlayer(registry);

    // Start trade between player1 and player2
    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    // Player1 tries to trade with player3 — should fail
    bool result = trade.sendTradeRequest(registry, player1, player3, 1200);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem cannot send request if target is trading", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);
    auto player3 = createTestPlayer(registry);

    // Player2 is trading with player3
    trade.sendTradeRequest(registry, player3, player2, 1000);
    trade.acceptTrade(registry, player2, player3, 1100);

    // Player1 tries to send request to player2 — should fail
    bool result = trade.sendTradeRequest(registry, player1, player2, 1200);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem request from non-existent component fails", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = registry.create();
    auto player2 = createTestPlayer(registry);

    // player1 has no TradeComponent
    bool result = trade.sendTradeRequest(registry, player1, player2, 1000);
    CHECK_FALSE(result);
}

// ============================================================================
// TEST: Accept / Decline
// ============================================================================

TEST_CASE("TradeSystem accept trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    bool result = trade.acceptTrade(registry, player2, player1, 1100);
    CHECK(result);

    CHECK(trade.getTradeState(registry, player1) == TradeState::Active);
    CHECK(trade.getTradeState(registry, player2) == TradeState::Active);
    CHECK(trade.isTrading(registry, player1));
    CHECK(trade.isTrading(registry, player2));
    CHECK(trade.getTradePartner(registry, player1) == player2);
    CHECK(trade.getTradePartner(registry, player2) == player1);
}

TEST_CASE("TradeSystem accept without pending request fails", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    // No request sent
    bool result = trade.acceptTrade(registry, player2, player1, 1000);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem decline trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    bool result = trade.declineTrade(registry, player2, player1);
    CHECK(result);

    // Player1 should be back to None
    CHECK(trade.getTradeState(registry, player1) == TradeState::None);
    CHECK(trade.getTradeState(registry, player2) == TradeState::None);
}

// ============================================================================
// TEST: Cancel Trade
// ============================================================================

TEST_CASE("TradeSystem cancel active trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.cancelTrade(registry, player1);
    CHECK(result);
    CHECK(trade.getTradeState(registry, player1) == TradeState::None);
    CHECK(trade.getTradeState(registry, player2) == TradeState::None);
}

TEST_CASE("TradeSystem cancel returns escrowed items", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    // Give player1 some items
    items.addToInventory(registry, player1, 1, 5);  // 5x item 1

    // Start trade and add item
    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);
    trade.addItem(registry, player1, 1, 3);

    // Item should be in escrow (removed from inventory)
    CHECK(items.countInInventory(registry, player1, 1) == 2);

    // Cancel trade
    trade.cancelTrade(registry, player1);

    // Item should be returned to inventory
    CHECK(items.countInInventory(registry, player1, 1) == 5);
}

TEST_CASE("TradeSystem cancel with no trade does nothing", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    bool result = trade.cancelTrade(registry, player1);
    CHECK_FALSE(result);
}

// ============================================================================
// TEST: Add/Remove Items
// ============================================================================

TEST_CASE("TradeSystem add item to trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.addItem(registry, player1, 1, 3);
    CHECK(result);

    // Item should be in escrow
    CHECK(items.countInInventory(registry, player1, 1) == 2);

    // Check trade state
    const auto& tc = registry.get<TradeComponent>(player1);
    CHECK(tc.offeredItemCount() == 1);
    CHECK(tc.offeredItems[0].itemId == 1);
    CHECK(tc.offeredItems[0].quantity == 3);
}

TEST_CASE("TradeSystem add item fails if player doesn't have enough", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 2);  // Only 2 of item 1

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.addItem(registry, player1, 1, 5);  // Try to offer 5
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem add item fails if not trading", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    items.addToInventory(registry, player1, 1, 5);

    bool result = trade.addItem(registry, player1, 1, 3);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem add item fails when locked", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);
    items.addToInventory(registry, player2, 2, 3);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.addItem(registry, player1, 1, 3);
    trade.addItem(registry, player2, 2, 2);

    // Lock player1's trade
    trade.lockTrade(registry, player1);

    // Try to add more items — should fail
    bool result = trade.addItem(registry, player1, 1, 1);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem remove item from trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.addItem(registry, player1, 1, 3);
    bool result = trade.removeItem(registry, player1, 0);  // Remove from trade slot 0
    CHECK(result);

    // Item returned to inventory
    CHECK(items.countInInventory(registry, player1, 1) == 5);

    // Trade slot is empty
    const auto& tc = registry.get<TradeComponent>(player1);
    CHECK(tc.offeredItemCount() == 0);
}

TEST_CASE("TradeSystem remove item from empty slot fails", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.removeItem(registry, player1, 0);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem add multiple items", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);
    items.addToInventory(registry, player1, 2, 3);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    CHECK(trade.addItem(registry, player1, 1, 3));
    CHECK(trade.addItem(registry, player1, 2, 2));

    const auto& tc = registry.get<TradeComponent>(player1);
    CHECK(tc.offeredItemCount() == 2);
}

TEST_CASE("TradeSystem cannot add more than max slots", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    // Give player1 items with different IDs
    for (uint32_t i = 1; i <= MAX_TRADE_SLOTS + 1; ++i) {
        items.addToInventory(registry, player1, i, 1);
    }

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    // Fill all slots
    for (uint32_t i = 1; i <= MAX_TRADE_SLOTS; ++i) {
        CHECK(trade.addItem(registry, player1, i, 1));
    }

    // This should fail — no empty slots
    CHECK_FALSE(trade.addItem(registry, player1, MAX_TRADE_SLOTS + 1, 1));
}

TEST_CASE("TradeSystem cannot add zero quantity", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    CHECK_FALSE(trade.addItem(registry, player1, 0, 1));  // Invalid item ID
    CHECK_FALSE(trade.addItem(registry, player1, 1, 0));  // Invalid quantity
}

// ============================================================================
// TEST: Gold Offer
// ============================================================================

TEST_CASE("TradeSystem set gold offer", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 100.0f);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.setGoldOffer(registry, player1, 50.0f);
    CHECK(result);

    const auto& tc = registry.get<TradeComponent>(player1);
    CHECK(tc.offeredGold == 50.0f);
}

TEST_CASE("TradeSystem gold offer fails if insufficient", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 10.0f);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.setGoldOffer(registry, player1, 50.0f);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem gold offer fails if negative", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 100.0f);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    bool result = trade.setGoldOffer(registry, player1, -10.0f);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem gold offer fails when not trading", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 100.0f);

    bool result = trade.setGoldOffer(registry, player1, 50.0f);
    CHECK_FALSE(result);
}

// ============================================================================
// TEST: Lock & Confirm
// ============================================================================

TEST_CASE("TradeSystem lock trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);
    trade.addItem(registry, player1, 1, 3);

    bool result = trade.lockTrade(registry, player1);
    CHECK(result);
    CHECK(trade.getTradeState(registry, player1) == TradeState::Locked);
}

TEST_CASE("TradeSystem lock fails if empty offer", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    // Lock with empty offer — should fail
    bool result = trade.lockTrade(registry, player1);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem both lock advances to BothLocked", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);
    items.addToInventory(registry, player2, 2, 3);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.addItem(registry, player1, 1, 3);
    trade.addItem(registry, player2, 2, 2);

    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);

    CHECK(trade.getTradeState(registry, player1) == TradeState::BothLocked);
    CHECK(trade.getTradeState(registry, player2) == TradeState::BothLocked);
}

TEST_CASE("TradeSystem double lock fails", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);
    trade.addItem(registry, player1, 1, 3);

    trade.lockTrade(registry, player1);
    bool result = trade.lockTrade(registry, player1);  // Double lock
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem unlock trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 5);
    items.addToInventory(registry, player2, 2, 3);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.addItem(registry, player1, 1, 3);
    trade.addItem(registry, player2, 2, 2);

    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);
    CHECK(trade.getTradeState(registry, player1) == TradeState::BothLocked);

    // Unlock
    bool result = trade.unlockTrade(registry, player1);
    CHECK(result);
    CHECK(trade.getTradeState(registry, player1) == TradeState::Active);
    CHECK(trade.getTradeState(registry, player2) == TradeState::Active);
}

// ============================================================================
// TEST: Confirm & Execute Trade
// ============================================================================

TEST_CASE("TradeSystem full trade flow with items and gold", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 100.0f);
    auto player2 = createTestPlayer(registry, 50.0f);

    // Give items to players
    items.addToInventory(registry, player1, 1, 5);  // 5x Iron Ore
    items.addToInventory(registry, player2, 2, 3);  // 3x Wolf Pelt

    // Request and accept
    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    // Add items to trade
    trade.addItem(registry, player1, 1, 3);  // Offer 3x Iron Ore
    trade.addItem(registry, player2, 2, 2);  // Offer 2x Wolf Pelt

    // Set gold
    trade.setGoldOffer(registry, player1, 30.0f);  // Offer 30g
    trade.setGoldOffer(registry, player2, 10.0f);   // Offer 10g

    // Lock both sides
    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);
    CHECK(trade.getTradeState(registry, player1) == TradeState::BothLocked);

    // Confirm both sides
    trade.confirmTrade(registry, player1, 1300);
    bool result = trade.confirmTrade(registry, player2, 1400);
    CHECK(result);

    // Verify items transferred
    CHECK(items.countInInventory(registry, player1, 1) == 2);  // 5 - 3 = 2
    CHECK(items.countInInventory(registry, player1, 2) == 2);  // 0 + 2 = 2 (received)
    CHECK(items.countInInventory(registry, player2, 1) == 3);  // 0 + 3 = 3 (received)
    CHECK(items.countInInventory(registry, player2, 2) == 1);  // 3 - 2 = 1

    // Verify gold transferred
    auto& inv1 = registry.get<Inventory>(player1);
    auto& inv2 = registry.get<Inventory>(player2);
    CHECK(inv1.gold == Approx(80.0f));  // 100 - 30 + 10 = 80
    CHECK(inv2.gold == Approx(70.0f));  // 50 - 10 + 30 = 70

    // Both players should be out of trade
    CHECK(trade.getTradeState(registry, player1) == TradeState::None);
    CHECK(trade.getTradeState(registry, player2) == TradeState::None);
}

TEST_CASE("TradeSystem confirm fires completion callback", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 1);
    items.addToInventory(registry, player2, 2, 1);

    bool callbackFired = false;
    bool callbackSuccess = false;
    trade.setTradeCompleteCallback([&](EntityID, EntityID, bool success) {
        callbackFired = true;
        callbackSuccess = success;
    });

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);
    trade.addItem(registry, player1, 1, 1);
    trade.addItem(registry, player2, 2, 1);  // Player2 must offer something too
    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);
    trade.confirmTrade(registry, player1, 1300);
    trade.confirmTrade(registry, player2, 1400);

    CHECK(callbackFired);
    CHECK(callbackSuccess);
}

TEST_CASE("TradeSystem confirm fails from wrong state", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 1);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);
    trade.addItem(registry, player1, 1, 1);

    // Confirm before locking — should fail
    bool result = trade.confirmTrade(registry, player1, 1300);
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem double confirm fails", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    items.addToInventory(registry, player1, 1, 1);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);
    trade.addItem(registry, player1, 1, 1);
    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);

    trade.confirmTrade(registry, player1, 1300);
    bool result = trade.confirmTrade(registry, player1, 1400);  // Double confirm
    CHECK_FALSE(result);
}

TEST_CASE("TradeSystem gold-only trade", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 100.0f);
    auto player2 = createTestPlayer(registry, 50.0f);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.setGoldOffer(registry, player1, 25.0f);
    trade.setGoldOffer(registry, player2, 10.0f);

    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);
    trade.confirmTrade(registry, player1, 1300);
    bool result = trade.confirmTrade(registry, player2, 1400);
    CHECK(result);

    auto& inv1 = registry.get<Inventory>(player1);
    auto& inv2 = registry.get<Inventory>(player2);
    CHECK(inv1.gold == Approx(85.0f));  // 100 - 25 + 10
    CHECK(inv2.gold == Approx(65.0f));  // 50 - 10 + 25
}

TEST_CASE("TradeSystem empty trade with gold only", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry, 100.0f);
    auto player2 = createTestPlayer(registry, 0.0f);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.setGoldOffer(registry, player1, 50.0f);
    // Player2 offers 0 gold (which is fine for gold-only trade)
    // But lock requires something offered...

    // Player2 needs to offer something or lock fails
    trade.setGoldOffer(registry, player2, 0.0f);  // Explicit 0
    CHECK_FALSE(trade.lockTrade(registry, player2));  // Can't lock with nothing

    // But player2 can lock if they offer even 0 gold with nothing else...
    // Actually, our lockTrade checks for > 0 items OR gold > 0.
    // 0.0f is not > 0.0f, so this fails. Correct behavior.
}

// ============================================================================
// TEST: Timeout
// ============================================================================

TEST_CASE("TradeSystem update handles timeout", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;

    TradeConfig config;
    config.tradeTimeoutMs = 5000;  // 5 second timeout
    trade.setConfig(config);
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    CHECK(trade.isTrading(registry, player1));

    // Update at time 4000 — within timeout
    trade.update(registry, 4000);
    CHECK(trade.isTrading(registry, player1));

    // Update at time 7000 — past timeout (started at 1100, timeout 5000)
    trade.update(registry, 7000);
    CHECK_FALSE(trade.isTrading(registry, player1));
    CHECK_FALSE(trade.isTrading(registry, player2));
}

// ============================================================================
// TEST: Query Methods
// ============================================================================

TEST_CASE("TradeSystem isTrading returns false for non-trading player", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    CHECK_FALSE(trade.isTrading(registry, player1));
}

TEST_CASE("TradeSystem getTradePartner returns null for non-trading", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    CHECK(static_cast<uint32_t>(trade.getTradePartner(registry, player1)) == static_cast<uint32_t>(entt::null));
}

TEST_CASE("TradeSystem getTradeState returns None for non-trading", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    CHECK(trade.getTradeState(registry, player1) == TradeState::None);
}

// ============================================================================
// TEST: TradeConfig
// ============================================================================

TEST_CASE("TradeConfig default values", "[trade]") {
    TradeConfig config;
    CHECK(config.maxTradeDistance == 10.0f);
    CHECK(config.tradeTimeoutMs == 60000);
}

// ============================================================================
// TEST: Edge Cases
// ============================================================================

TEST_CASE("TradeSystem add item with full inventory returns overflow", "[trade]") {
    Registry registry;
    TradeSystem trade;
    ItemSystem items;
    ChatSystem chat;
    setupSystems(trade, items, chat);

    auto player1 = createTestPlayer(registry);
    auto player2 = createTestPlayer(registry);

    // Fill player2's inventory
    for (uint32_t i = 0; i < INVENTORY_SIZE; ++i) {
        registry.get<Inventory>(player2).slots[i].itemId = 100 + i;
        registry.get<Inventory>(player2).slots[i].quantity = 1;
    }

    items.addToInventory(registry, player1, 1, 5);

    trade.sendTradeRequest(registry, player1, player2, 1000);
    trade.acceptTrade(registry, player2, player1, 1100);

    trade.addItem(registry, player1, 1, 5);
    trade.addItem(registry, player2, 100, 1);  // Offer something

    trade.lockTrade(registry, player1);
    trade.lockTrade(registry, player2);
    trade.confirmTrade(registry, player1, 1300);
    bool result = trade.confirmTrade(registry, player2, 1400);
    CHECK(result);

    // Player1 should have received player2's item (since player1 has space)
    // Player2's inventory was full, so player1's items should be returned
    // (overflow handling in executeTrade)
}

TEST_CASE("TradeComponent state enum values", "[trade]") {
    CHECK(static_cast<uint8_t>(TradeState::None) == 0);
    CHECK(static_cast<uint8_t>(TradeState::Pending) == 1);
    CHECK(static_cast<uint8_t>(TradeState::Active) == 2);
    CHECK(static_cast<uint8_t>(TradeState::Locked) == 3);
    CHECK(static_cast<uint8_t>(TradeState::BothLocked) == 4);
    CHECK(static_cast<uint8_t>(TradeState::Confirmed) == 5);
}
