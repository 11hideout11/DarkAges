// Trading System implementation
// Manages player-to-player item and gold exchanges

#include "combat/TradeSystem.hpp"
#include "combat/ItemSystem.hpp"
#include "combat/ChatSystem.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace DarkAges {

bool TradeSystem::sendTradeRequest(Registry& registry, EntityID initiator,
                                    EntityID target, uint32_t currentTimeMs) {
    // Validate entities exist and have trade components
    auto* initTrade = registry.try_get<TradeComponent>(initiator);
    auto* targetTrade = registry.try_get<TradeComponent>(target);

    if (!initTrade || !targetTrade) {
        return false;
    }

    // Neither player can be trading already
    if (initTrade->state != TradeState::None ||
        targetTrade->state != TradeState::None) {
        return false;
    }

    // Cannot trade with yourself
    if (initiator == target) {
        return false;
    }

    // Check distance
    const auto* posA = registry.try_get<Position>(initiator);
    const auto* posB = registry.try_get<Position>(target);
    if (posA && posB) {
        float distSq = posA->distanceSqTo(*posB);
        float maxDist = config_.maxTradeDistance;
        float maxDistSq = maxDist * maxDist * Constants::FIXED_TO_FLOAT * Constants::FIXED_TO_FLOAT;
        // distanceSqTo returns fixed-point, convert to compare with float
        float dx = static_cast<float>(posA->x - posB->x) * Constants::FIXED_TO_FLOAT;
        float dz = static_cast<float>(posA->z - posB->z) * Constants::FIXED_TO_FLOAT;
        float actualDistSq = dx * dx + dz * dz;
        if (actualDistSq > config_.maxTradeDistance * config_.maxTradeDistance) {
            return false;
        }
    }

    // Set up pending request
    initTrade->state = TradeState::Pending;
    initTrade->tradePartner = target;

    // Store the pending request for the target
    uint32_t targetIdx = static_cast<uint32_t>(target);
    if (targetIdx < MAX_PENDING_REQUESTS) {
        pendingRequests_[targetIdx].initiator = initiator;
        pendingRequests_[targetIdx].timestampMs = currentTimeMs;
    }

    // Fire callback
    if (tradeRequestCallback_) {
        tradeRequestCallback_(initiator, target);
    }

    return true;
}

bool TradeSystem::acceptTrade(Registry& registry, EntityID acceptor,
                               EntityID initiator, uint32_t currentTimeMs) {
    auto* acceptTradeComp = registry.try_get<TradeComponent>(acceptor);
    auto* initTrade = registry.try_get<TradeComponent>(initiator);

    if (!acceptTradeComp || !initTrade) {
        return false;
    }

    // Verify pending request exists
    uint32_t acceptorIdx = static_cast<uint32_t>(acceptor);
    if (acceptorIdx >= MAX_PENDING_REQUESTS ||
        pendingRequests_[acceptorIdx].initiator != initiator) {
        return false;
    }

    // Initiator must still be pending
    if (initTrade->state != TradeState::Pending ||
        initTrade->tradePartner != acceptor) {
        // Clear stale request
        pendingRequests_[acceptorIdx] = PendingRequest{};
        return false;
    }

    // Acceptor must not be trading
    if (acceptTradeComp->state != TradeState::None) {
        return false;
    }

    // Clear the pending request
    pendingRequests_[acceptorIdx] = PendingRequest{};

    // Activate trade for both players
    acceptTradeComp->state = TradeState::Active;
    acceptTradeComp->tradePartner = initiator;
    initTrade->state = TradeState::Active;
    initTrade->tradePartner = acceptor;

    // Record start time for timeout
    uint32_t initIdx = static_cast<uint32_t>(initiator);
    if (initIdx < MAX_PENDING_REQUESTS) {
        tradeStartTimes_[initIdx] = currentTimeMs;
    }

    // Fire state change callback
    if (tradeStateCallback_) {
        tradeStateCallback_(initiator, TradeState::Active);
        tradeStateCallback_(acceptor, TradeState::Active);
    }

    return true;
}

bool TradeSystem::declineTrade(Registry& registry, EntityID decliner,
                                EntityID fromPlayer) {
    uint32_t declinerIdx = static_cast<uint32_t>(decliner);
    if (declinerIdx >= MAX_PENDING_REQUESTS ||
        pendingRequests_[declinerIdx].initiator != fromPlayer) {
        return false;
    }

    // Clear pending request
    pendingRequests_[declinerIdx] = PendingRequest{};

    // Reset initiator's state if they were pending for this player
    auto* initTrade = registry.try_get<TradeComponent>(fromPlayer);
    if (initTrade && initTrade->state == TradeState::Pending &&
        initTrade->tradePartner == decliner) {
        initTrade->reset();
    }

    return true;
}

bool TradeSystem::cancelTrade(Registry& registry, EntityID player) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade || trade->state == TradeState::None) {
        return false;
    }

    EntityID partner = trade->tradePartner;
    auto* partnerTrade = registry.try_get<TradeComponent>(partner);

    // Return escrowed items to both players
    returnEscrowedItems(registry, player);
    if (partnerTrade) {
        returnEscrowedItems(registry, partner);
    }

    // Clear trade state
    clearTradeState(registry, player, partner);

    // Notify via chat
    if (chatSystem_) {
        chatSystem_->sendSystemMessage(registry, player,
                                        "Trade cancelled.", 0);
        if (partnerTrade) {
            chatSystem_->sendSystemMessage(registry, partner,
                                            "Trade cancelled.", 0);
        }
    }

    return true;
}

bool TradeSystem::addItem(Registry& registry, EntityID player,
                           uint32_t itemId, uint32_t quantity) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade || trade->state != TradeState::Active || trade->locked) {
        return false;
    }

    if (itemId == 0 || quantity == 0) {
        return false;
    }

    // Check player has the item
    if (!itemSystem_->hasInInventory(registry, player, itemId, quantity)) {
        return false;
    }

    // Find empty slot
    int32_t slotIdx = trade->findEmptySlot();
    if (slotIdx < 0) {
        return false;  // No empty slots
    }

    // Check if item is tradable
    const ItemDefinition* def = itemSystem_->getItem(itemId);
    if (def && !def->tradable) {
        return false;
    }

    // Add to trade offer
    trade->offeredItems[slotIdx].itemId = itemId;
    trade->offeredItems[slotIdx].quantity = quantity;

    // Remove from inventory (held in escrow)
    itemSystem_->removeFromInventory(registry, player, itemId, quantity);

    // Unlock if previously locked (offer changed)
    // Both sides must re-lock after any change

    return true;
}

bool TradeSystem::removeItem(Registry& registry, EntityID player,
                              uint32_t slotIndex) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade || trade->state != TradeState::Active || trade->locked) {
        return false;
    }

    if (slotIndex >= MAX_TRADE_SLOTS) {
        return false;
    }

    if (trade->offeredItems[slotIndex].isEmpty()) {
        return false;
    }

    // Return item to inventory
    uint32_t itemId = trade->offeredItems[slotIndex].itemId;
    uint32_t quantity = trade->offeredItems[slotIndex].quantity;

    itemSystem_->addToInventory(registry, player, itemId, quantity);

    // Clear slot
    trade->offeredItems[slotIndex] = TradeSlot{};

    return true;
}

bool TradeSystem::setGoldOffer(Registry& registry, EntityID player,
                                float goldAmount) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade || trade->state != TradeState::Active || trade->locked) {
        return false;
    }

    if (goldAmount < 0.0f) {
        return false;
    }

    // Check player has enough gold
    auto* inv = registry.try_get<Inventory>(player);
    if (!inv || inv->gold < goldAmount) {
        return false;
    }

    trade->offeredGold = goldAmount;
    return true;
}

bool TradeSystem::lockTrade(Registry& registry, EntityID player) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade || trade->state != TradeState::Active) {
        return false;
    }

    if (trade->locked) {
        return false;  // Already locked
    }

    // Must offer something (items or gold)
    if (trade->offeredItemCount() == 0 && trade->offeredGold <= 0.0f) {
        return false;
    }

    trade->locked = true;

    // Check if partner is also locked
    EntityID partner = trade->tradePartner;
    auto* partnerTrade = registry.try_get<TradeComponent>(partner);

    if (partnerTrade && partnerTrade->locked) {
        // Both locked — advance state
        trade->state = TradeState::BothLocked;
        partnerTrade->state = TradeState::BothLocked;

        if (tradeStateCallback_) {
            tradeStateCallback_(player, TradeState::BothLocked);
            tradeStateCallback_(partner, TradeState::BothLocked);
        }
    } else {
        trade->state = TradeState::Locked;

        if (tradeStateCallback_) {
            tradeStateCallback_(player, TradeState::Locked);
        }
    }

    return true;
}

bool TradeSystem::unlockTrade(Registry& registry, EntityID player) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade) {
        return false;
    }

    // Can unlock from Locked or BothLocked
    if (trade->state != TradeState::Locked &&
        trade->state != TradeState::BothLocked) {
        return false;
    }

    trade->locked = false;
    trade->confirmed = false;

    EntityID partner = trade->tradePartner;
    auto* partnerTrade = registry.try_get<TradeComponent>(partner);

    // Revert to Active state
    trade->state = TradeState::Active;
    if (partnerTrade) {
        partnerTrade->state = TradeState::Active;
        partnerTrade->confirmed = false;
        // Partner might need to re-lock if they were locked
        partnerTrade->locked = false;
    }

    if (tradeStateCallback_) {
        tradeStateCallback_(player, TradeState::Active);
        if (partnerTrade) {
            tradeStateCallback_(partner, TradeState::Active);
        }
    }

    return true;
}

bool TradeSystem::confirmTrade(Registry& registry, EntityID player,
                                uint32_t currentTimeMs) {
    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade || trade->state != TradeState::BothLocked) {
        return false;
    }

    if (trade->confirmed) {
        return false;  // Already confirmed
    }

    trade->confirmed = true;

    EntityID partner = trade->tradePartner;
    auto* partnerTrade = registry.try_get<TradeComponent>(partner);

    if (partnerTrade && partnerTrade->confirmed) {
        // Both confirmed — execute the trade
        bool success = executeTrade(registry, player, partner);

        // Clear trade state
        clearTradeState(registry, player, partner);

        // Notify
        if (chatSystem_) {
            if (success) {
                chatSystem_->sendSystemMessage(registry, player,
                                                "Trade completed successfully.", currentTimeMs);
                chatSystem_->sendSystemMessage(registry, partner,
                                                "Trade completed successfully.", currentTimeMs);
            } else {
                chatSystem_->sendSystemMessage(registry, player,
                                                "Trade failed — items may no longer be available.", currentTimeMs);
                chatSystem_->sendSystemMessage(registry, partner,
                                                "Trade failed — items may no longer be available.", currentTimeMs);
            }
        }

        if (tradeCompleteCallback_) {
            tradeCompleteCallback_(player, partner, success);
        }

        return success;
    }

    return true;  // Player confirmed, waiting for partner
}

void TradeSystem::update(Registry& registry, uint32_t currentTimeMs) {
    // Check for trade timeouts
    auto view = registry.view<TradeComponent>();
    for (auto entity : view) {
        auto& trade = view.get<TradeComponent>(entity);
        if (trade.state == TradeState::None ||
            trade.state == TradeState::Pending) {
            continue;
        }

        // Check timeout for active trades
        uint32_t entityIdx = static_cast<uint32_t>(entity);
        if (entityIdx < MAX_PENDING_REQUESTS && tradeStartTimes_[entityIdx] > 0) {
            uint32_t elapsed = currentTimeMs - tradeStartTimes_[entityIdx];
            if (elapsed > config_.tradeTimeoutMs) {
                // Trade timed out
                cancelTrade(registry, entity);
            }
        }
    }

    // Clean up stale pending requests
    for (uint32_t i = 0; i < MAX_PENDING_REQUESTS; ++i) {
        if (pendingRequests_[i].initiator != entt::null &&
            pendingRequests_[i].timestampMs > 0) {
            uint32_t elapsed = currentTimeMs - pendingRequests_[i].timestampMs;
            if (elapsed > config_.tradeTimeoutMs) {
                pendingRequests_[i] = PendingRequest{};
            }
        }
    }
}

bool TradeSystem::isTrading(const Registry& registry, EntityID player) const {
    const auto* trade = registry.try_get<TradeComponent>(player);
    return trade && trade->state != TradeState::None;
}

EntityID TradeSystem::getTradePartner(const Registry& registry,
                                       EntityID player) const {
    const auto* trade = registry.try_get<TradeComponent>(player);
    if (trade && trade->state != TradeState::None) {
        return trade->tradePartner;
    }
    return entt::null;
}

TradeState TradeSystem::getTradeState(const Registry& registry,
                                       EntityID player) const {
    const auto* trade = registry.try_get<TradeComponent>(player);
    if (trade) {
        return trade->state;
    }
    return TradeState::None;
}

bool TradeSystem::executeTrade(Registry& registry, EntityID player1,
                                EntityID player2) {
    if (!itemSystem_) {
        return false;
    }

    auto* trade1 = registry.try_get<TradeComponent>(player1);
    auto* trade2 = registry.try_get<TradeComponent>(player2);
    auto* inv1 = registry.try_get<Inventory>(player1);
    auto* inv2 = registry.try_get<Inventory>(player2);

    if (!trade1 || !trade2 || !inv1 || !inv2) {
        return false;
    }

    // Validate offers (check nothing was removed from inventory since locking)
    if (!validateOffers(registry, player1, player2)) {
        return false;
    }

    // Transfer player1's offered items to player2
    for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
        if (!trade1->offeredItems[i].isEmpty()) {
            uint32_t overflow = itemSystem_->addToInventory(
                registry, player2,
                trade1->offeredItems[i].itemId,
                trade1->offeredItems[i].quantity);
            if (overflow > 0) {
                // Player2 inventory full — return overflow to player1
                itemSystem_->addToInventory(registry, player1,
                                            trade1->offeredItems[i].itemId, overflow);
            }
        }
    }

    // Transfer player2's offered items to player1
    for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
        if (!trade2->offeredItems[i].isEmpty()) {
            uint32_t overflow = itemSystem_->addToInventory(
                registry, player1,
                trade2->offeredItems[i].itemId,
                trade2->offeredItems[i].quantity);
            if (overflow > 0) {
                itemSystem_->addToInventory(registry, player2,
                                            trade2->offeredItems[i].itemId, overflow);
            }
        }
    }

    // Transfer gold
    float gold1 = trade1->offeredGold;
    float gold2 = trade2->offeredGold;

    inv1->gold -= gold1;
    inv1->gold += gold2;
    inv2->gold -= gold2;
    inv2->gold += gold1;

    // Clamp gold to 0 (shouldn't happen if validation passed)
    if (inv1->gold < 0.0f) inv1->gold = 0.0f;
    if (inv2->gold < 0.0f) inv2->gold = 0.0f;

    return true;
}

void TradeSystem::returnEscrowedItems(Registry& registry, EntityID player) {
    if (!itemSystem_) return;

    auto* trade = registry.try_get<TradeComponent>(player);
    if (!trade) return;

    for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
        if (!trade->offeredItems[i].isEmpty()) {
            itemSystem_->addToInventory(registry, player,
                                        trade->offeredItems[i].itemId,
                                        trade->offeredItems[i].quantity);
            trade->offeredItems[i] = TradeSlot{};
        }
    }
}

bool TradeSystem::validateOffers(const Registry& registry,
                                  EntityID player1, EntityID player2) const {
    if (!itemSystem_) return false;

    const auto* trade1 = registry.try_get<TradeComponent>(player1);
    const auto* trade2 = registry.try_get<TradeComponent>(player2);
    const auto* inv1 = registry.try_get<Inventory>(player1);
    const auto* inv2 = registry.try_get<Inventory>(player2);

    if (!trade1 || !trade2 || !inv1 || !inv2) return false;

    // Items were removed from inventory during addItem (escrow),
    // so we just need to verify gold is still available
    if (inv1->gold < trade1->offeredGold) return false;
    if (inv2->gold < trade2->offeredGold) return false;

    return true;
}

void TradeSystem::clearTradeState(Registry& registry,
                                   EntityID player1, EntityID player2) {
    auto* trade1 = registry.try_get<TradeComponent>(player1);
    auto* trade2 = registry.try_get<TradeComponent>(player2);

    if (trade1) trade1->reset();
    if (trade2) trade2->reset();

    // Clear timeout tracking
    uint32_t idx1 = static_cast<uint32_t>(player1);
    uint32_t idx2 = static_cast<uint32_t>(player2);
    if (idx1 < MAX_PENDING_REQUESTS) tradeStartTimes_[idx1] = 0;
    if (idx2 < MAX_PENDING_REQUESTS) tradeStartTimes_[idx2] = 0;
}

} // namespace DarkAges
