#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Chat System — manages message routing, validation, and delivery.
// Supports Local (zone), Global, Whisper, Party, Guild, and System channels.
// Provides per-player rate limiting and message history.

namespace DarkAges {

class PartySystem;  // Forward declaration
class GuildSystem;  // Forward declaration

class ChatSystem {
public:
    ChatSystem() = default;

    // --- Configuration ---

    // Set chat configuration
    void setConfig(const ChatConfig& config) { config_ = config; }

    // Get current configuration
    [[nodiscard]] const ChatConfig& getConfig() const { return config_; }

    // --- Message Sending ---

    // Process a chat message from a player.
    // Validates, rate-limits, and routes the message to appropriate recipients.
    // Returns true if the message was accepted for delivery.
    bool sendMessage(Registry& registry, EntityID sender, ChatChannel channel,
                     const char* content, uint32_t currentTimeMs,
                     uint32_t targetId = 0);

    // Send a system message to a specific player (no sender).
    // Always delivered regardless of mute/rate limits.
    void sendSystemMessage(Registry& registry, EntityID target,
                           const char* content, uint32_t currentTimeMs);

    // Broadcast a system message to all players in a zone.
    void broadcastSystemMessage(Registry& registry, const char* content,
                                uint32_t currentTimeMs);

    // --- Message Delivery ---

    // Deliver a chat message to a specific entity.
    // Adds to their ChatComponent history.
    void deliverMessage(Registry& registry, EntityID recipient,
                        const ChatMessage& message);

    // --- Player Management ---

    // Mute/unmute a player
    void setMuted(Registry& registry, EntityID player, bool muted);

    // Check if a player is muted
    [[nodiscard]] bool isMuted(const Registry& registry, EntityID player) const;

    // --- Query ---

    // Get recent messages for a player
    [[nodiscard]] std::vector<ChatMessage> getRecentMessages(
        const Registry& registry, EntityID player, uint32_t count = 10) const;

    // --- Callbacks ---

    // Callback for when a message needs to be sent over the network.
    // Signature: void(ConnectionID recipient, const ChatMessage& msg)
    using MessageDeliveryCallback = std::function<void(ConnectionID, const ChatMessage&)>;
    void setMessageDeliveryCallback(MessageDeliveryCallback cb) {
        messageDeliveryCallback_ = std::move(cb);
    }

    // Callback for getting connection ID from entity
    using ConnectionLookupCallback = std::function<ConnectionID(EntityID)>;
    void setConnectionLookupCallback(ConnectionLookupCallback cb) {
        connectionLookupCallback_ = std::move(cb);
    }

    // Callback for getting nearby entities (for local chat range)
    using NearbyEntitiesCallback = std::function<std::vector<EntityID>(EntityID, float range)>;
    void setNearbyEntitiesCallback(NearbyEntitiesCallback cb) {
        nearbyEntitiesCallback_ = std::move(cb);
    }

    // Set party system for party chat routing
    void setPartySystem(PartySystem* ps) { partySystem_ = ps; }

    // Set guild system for guild chat routing
    void setGuildSystem(GuildSystem* gs) { guildSystem_ = gs; }

    // --- Statistics ---

    [[nodiscard]] uint32_t getTotalMessagesProcessed() const { return totalMessagesProcessed_; }
    [[nodiscard]] uint32_t getTotalMessagesDelivered() const { return totalMessagesDelivered_; }
    [[nodiscard]] uint32_t getTotalRateLimited() const { return totalRateLimited_; }

private:
    // Validate message content (length, empty check)
    [[nodiscard]] bool validateContent(const char* content) const;

    // Check rate limit for a player
    [[nodiscard]] bool checkRateLimit(Registry& registry, EntityID sender,
                                      uint32_t currentTimeMs);

    // Route message to appropriate recipients based on channel
    void routeMessage(Registry& registry, EntityID sender,
                      const ChatMessage& message);

    // Route local (zone) chat — sends to nearby players
    void routeLocalChat(Registry& registry, EntityID sender,
                        const ChatMessage& message);

    // Route whisper — sends to specific target
    void routeWhisper(Registry& registry, EntityID sender,
                      const ChatMessage& message);

    // Route global chat — sends to all connected players
    void routeGlobalChat(Registry& registry, const ChatMessage& message);

    // Route party chat — sends to party members
    void routePartyChat(Registry& registry, EntityID sender,
                        const ChatMessage& message);

    // Route guild chat — sends to guild members
    void routeGuildChat(Registry& registry, EntityID sender,
                        const ChatMessage& message);

    // Configuration
    ChatConfig config_;

    // Message ID counter
    uint32_t nextMessageId_{1};

    // Statistics
    uint32_t totalMessagesProcessed_{0};
    uint32_t totalMessagesDelivered_{0};
    uint32_t totalRateLimited_{0};

    // Callbacks
    MessageDeliveryCallback messageDeliveryCallback_;
    ConnectionLookupCallback connectionLookupCallback_;
    NearbyEntitiesCallback nearbyEntitiesCallback_;

    // Social systems for routing
    PartySystem* partySystem_{nullptr};
    GuildSystem* guildSystem_{nullptr};
};

} // namespace DarkAges
