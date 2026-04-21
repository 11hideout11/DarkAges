#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>

// Trading System — manages player-to-player item and gold exchanges.
// Supports trade requests, item/gold offers, lock/confirm flow.
// Prevents scams with a two-phase commit: both players lock, then both confirm.

namespace DarkAges {

class ItemSystem;
class ChatSystem;

class TradeSystem {
public:
    TradeSystem() = default;

    // --- Configuration ---

    void setConfig(const TradeConfig& config) { config_ = config; }
    [[nodiscard]] const TradeConfig& getConfig() const { return config_; }

    // --- Dependencies ---

    void setItemSystem(ItemSystem* is) { itemSystem_ = is; }
    void setChatSystem(ChatSystem* cs) { chatSystem_ = cs; }

    // --- Trade Lifecycle ---

    // Send a trade request to another player.
    // Initiator must not be trading. Target must not be trading.
    // Returns true if request was sent.
    bool sendTradeRequest(Registry& registry, EntityID initiator,
                          EntityID target, uint32_t currentTimeMs);

    // Accept an incoming trade request.
    // Target must have a pending request from the initiator.
    // Returns true if trade was started.
    bool acceptTrade(Registry& registry, EntityID acceptor,
                     EntityID initiator, uint32_t currentTimeMs);

    // Decline an incoming trade request.
    // Returns true if request was declined.
    bool declineTrade(Registry& registry, EntityID decliner,
                      EntityID fromPlayer);

    // Cancel an active or pending trade.
    // Either player can cancel at any time before both confirm.
    // Returns true if trade was cancelled.
    bool cancelTrade(Registry& registry, EntityID player);

    // --- Negotiation ---

    // Add an item to the trade offer.
    // Player must have the item in inventory. Item is held in escrow.
    // Returns true if item was added.
    bool addItem(Registry& registry, EntityID player,
                 uint32_t itemId, uint32_t quantity);

    // Remove an item from the trade offer (returns to inventory).
    // Returns true if item was removed.
    bool removeItem(Registry& registry, EntityID player, uint32_t slotIndex);

    // Set gold amount to offer.
    // Player must have enough gold. Returns true if set.
    bool setGoldOffer(Registry& registry, EntityID player, float goldAmount);

    // --- Lock & Confirm ---

    // Lock the trade offer. No more changes allowed until unlocked.
    // If both players are locked, moves to BothLocked state.
    // Returns true if locked.
    bool lockTrade(Registry& registry, EntityID player);

    // Unlock the trade offer (allows changes again).
    // Only works when in Locked state (one side locked).
    // Returns true if unlocked.
    bool unlockTrade(Registry& registry, EntityID player);

    // Confirm the trade after both sides are locked.
    // When both confirm, the trade executes (items/gold transfer).
    // Returns true if trade was confirmed and executed.
    bool confirmTrade(Registry& registry, EntityID player,
                      uint32_t currentTimeMs);

    // --- Tick Update ---

    // Called each server tick to handle timeouts.
    void update(Registry& registry, uint32_t currentTimeMs);

    // --- Query ---

    // Check if a player is currently in a trade
    [[nodiscard]] bool isTrading(const Registry& registry, EntityID player) const;

    // Get the trade partner for a player, entt::null if not trading
    [[nodiscard]] EntityID getTradePartner(const Registry& registry,
                                           EntityID player) const;

    // Get the trade state for a player
    [[nodiscard]] TradeState getTradeState(const Registry& registry,
                                           EntityID player) const;

    // --- Callbacks ---

    // Called when a trade request is sent: (initiator, target)
    using TradeRequestCallback = std::function<void(EntityID, EntityID)>;
    void setTradeRequestCallback(TradeRequestCallback cb) {
        tradeRequestCallback_ = std::move(cb);
    }

    // Called when a trade is completed: (player1, player2, success)
    using TradeCompleteCallback = std::function<void(EntityID, EntityID, bool)>;
    void setTradeCompleteCallback(TradeCompleteCallback cb) {
        tradeCompleteCallback_ = std::move(cb);
    }

    // Called when trade state changes: (player, newState)
    using TradeStateCallback = std::function<void(EntityID, TradeState)>;
    void setTradeStateCallback(TradeStateCallback cb) {
        tradeStateCallback_ = std::move(cb);
    }

private:
    // Execute the trade: transfer items and gold between both players
    bool executeTrade(Registry& registry, EntityID player1, EntityID player2);

    // Return escrowed items to their owner (on cancel)
    void returnEscrowedItems(Registry& registry, EntityID player);

    // Validate that both players still have their offered items/gold
    bool validateOffers(const Registry& registry, EntityID player1,
                        EntityID player2) const;

    // Clear trade state for both players
    void clearTradeState(Registry& registry, EntityID player1, EntityID player2);

    TradeConfig config_;
    ItemSystem* itemSystem_{nullptr};
    ChatSystem* chatSystem_{nullptr};

    // Pending trade requests: maps target -> (initiator, timestamp)
    struct PendingRequest {
        EntityID initiator{entt::null};
        uint32_t timestampMs{0};
    };
    static constexpr uint32_t MAX_PENDING_REQUESTS = 64;
    // Using a simple array indexed by entity — works with small entity IDs
    PendingRequest pendingRequests_[MAX_PENDING_REQUESTS]{};

    // Track when each trade started (for timeout)
    uint32_t tradeStartTimes_[MAX_PENDING_REQUESTS]{};

    // Callbacks
    TradeRequestCallback tradeRequestCallback_;
    TradeCompleteCallback tradeCompleteCallback_;
    TradeStateCallback tradeStateCallback_;
};

} // namespace DarkAges
