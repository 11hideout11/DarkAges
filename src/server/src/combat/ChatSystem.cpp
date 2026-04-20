// Chat System implementation
// Manages message routing, validation, rate limiting, and delivery

#include "combat/ChatSystem.hpp"
#include "ecs/Components.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace DarkAges {

bool ChatSystem::sendMessage(Registry& registry, EntityID sender,
                              ChatChannel channel, const char* content,
                              uint32_t currentTimeMs, uint32_t targetId) {
    totalMessagesProcessed_++;

    // Validate content
    if (!validateContent(content)) {
        return false;
    }

    // Check if player is muted
    if (isMuted(registry, sender)) {
        return false;
    }

    // Check rate limit
    if (!checkRateLimit(registry, sender, currentTimeMs)) {
        totalRateLimited_++;
        return false;
    }

    // Check channel availability
    if (channel == ChatChannel::Global && !config_.globalChatEnabled) {
        return false;
    }

    // Build message
    ChatMessage msg{};
    msg.messageId = nextMessageId_++;
    msg.channel = channel;
    msg.senderId = static_cast<uint32_t>(sender);
    msg.targetId = targetId;
    msg.timestampMs = currentTimeMs;

    // Copy sender name from PlayerInfo
    if (const PlayerInfo* info = registry.try_get<PlayerInfo>(sender)) {
        std::strncpy(msg.senderName, info->username, CHAT_SENDER_NAME_MAX - 1);
        msg.senderName[CHAT_SENDER_NAME_MAX - 1] = '\0';
    } else {
        std::strncpy(msg.senderName, "Unknown", CHAT_SENDER_NAME_MAX - 1);
    }

    // Copy content
    std::strncpy(msg.content, content, CHAT_MESSAGE_MAX_LEN - 1);
    msg.content[CHAT_MESSAGE_MAX_LEN - 1] = '\0';

    // Update rate limit state on sender's ChatComponent
    ChatComponent* chat = registry.try_get<ChatComponent>(sender);
    if (chat) {
        chat->lastMessageTimeMs = currentTimeMs;
    }

    // Route to recipients
    routeMessage(registry, sender, msg);

    return true;
}

void ChatSystem::sendSystemMessage(Registry& registry, EntityID target,
                                    const char* content, uint32_t currentTimeMs) {
    if (!config_.systemMessagesEnabled) return;
    if (!validateContent(content)) return;

    ChatMessage msg{};
    msg.messageId = nextMessageId_++;
    msg.channel = ChatChannel::System;
    msg.senderId = 0;  // System
    msg.targetId = static_cast<uint32_t>(target);
    msg.timestampMs = currentTimeMs;
    std::strncpy(msg.senderName, "System", CHAT_SENDER_NAME_MAX - 1);
    std::strncpy(msg.content, content, CHAT_MESSAGE_MAX_LEN - 1);
    msg.content[CHAT_MESSAGE_MAX_LEN - 1] = '\0';

    deliverMessage(registry, target, msg);
}

void ChatSystem::broadcastSystemMessage(Registry& registry, const char* content,
                                         uint32_t currentTimeMs) {
    if (!config_.systemMessagesEnabled) return;
    if (!validateContent(content)) return;

    ChatMessage msg{};
    msg.messageId = nextMessageId_++;
    msg.channel = ChatChannel::System;
    msg.senderId = 0;
    msg.timestampMs = currentTimeMs;
    std::strncpy(msg.senderName, "System", CHAT_SENDER_NAME_MAX - 1);
    std::strncpy(msg.content, content, CHAT_MESSAGE_MAX_LEN - 1);
    msg.content[CHAT_MESSAGE_MAX_LEN - 1] = '\0';

    // Deliver to all entities with ChatComponent
    auto view = registry.view<ChatComponent>();
    for (auto entity : view) {
        deliverMessage(registry, entity, msg);
    }
}

void ChatSystem::deliverMessage(Registry& registry, EntityID recipient,
                                 const ChatMessage& message) {
    // Add to player's chat history
    ChatComponent* chat = registry.try_get<ChatComponent>(recipient);
    if (chat) {
        chat->addMessage(message);
    }

    totalMessagesDelivered_++;

    // Notify network callback for actual delivery
    if (messageDeliveryCallback_ && connectionLookupCallback_) {
        ConnectionID conn = connectionLookupCallback_(recipient);
        if (conn != 0) {
            messageDeliveryCallback_(conn, message);
        }
    }
}

void ChatSystem::setMuted(Registry& registry, EntityID player, bool muted) {
    ChatComponent* chat = registry.try_get<ChatComponent>(player);
    if (chat) {
        chat->muted = muted;
    }
}

bool ChatSystem::isMuted(const Registry& registry, EntityID player) const {
    const ChatComponent* chat = registry.try_get<ChatComponent>(player);
    return chat && chat->muted;
}

std::vector<ChatMessage> ChatSystem::getRecentMessages(
    const Registry& registry, EntityID player, uint32_t count) const {

    std::vector<ChatMessage> result;
    const ChatComponent* chat = registry.try_get<ChatComponent>(player);
    if (!chat) return result;

    uint32_t available = std::min(count, std::min(chat->messageCount, CHAT_HISTORY_SIZE));
    // Start from the most recent messages
    uint32_t startIdx;
    if (chat->messageCount <= CHAT_HISTORY_SIZE) {
        // Buffer not yet wrapped — start from (messageCount - available)
        startIdx = chat->messageCount - available;
    } else {
        // Buffer has wrapped — oldest is at write position
        startIdx = (chat->messageCount - available) % CHAT_HISTORY_SIZE;
    }

    for (uint32_t i = 0; i < available; ++i) {
        uint32_t idx = (startIdx + i) % CHAT_HISTORY_SIZE;
        result.push_back(chat->recentMessages[idx]);
    }

    return result;
}

bool ChatSystem::validateContent(const char* content) const {
    if (!content) return false;
    size_t len = std::strlen(content);
    return len > 0 && len < CHAT_MESSAGE_MAX_LEN;
}

bool ChatSystem::checkRateLimit(Registry& registry, EntityID sender,
                                 uint32_t currentTimeMs) {
    ChatComponent* chat = registry.try_get<ChatComponent>(sender);
    if (!chat) return true;  // No component = no rate limit

    // Check if rate window has expired
    if (currentTimeMs - chat->rateWindowStartMs >= config_.rateWindowMs) {
        // New window
        chat->rateWindowStartMs = currentTimeMs;
        chat->messagesThisWindow = 1;
        return true;
    }

    // Within current window
    if (chat->messagesThisWindow >= config_.maxMessagesPerWindow) {
        return false;  // Rate limited
    }

    chat->messagesThisWindow++;
    return true;
}

void ChatSystem::routeMessage(Registry& registry, EntityID sender,
                               const ChatMessage& message) {
    switch (message.channel) {
        case ChatChannel::Local:
            routeLocalChat(registry, sender, message);
            break;
        case ChatChannel::Global:
            routeGlobalChat(registry, message);
            break;
        case ChatChannel::Whisper:
            routeWhisper(registry, sender, message);
            break;
        case ChatChannel::Party:
            routePartyChat(registry, sender, message);
            break;
        case ChatChannel::Guild:
            routeGuildChat(registry, sender, message);
            break;
        case ChatChannel::System:
            // System messages use sendSystemMessage/broadcastSystemMessage
            break;
    }
}

void ChatSystem::routeLocalChat(Registry& registry, EntityID sender,
                                 const ChatMessage& message) {
    // If we have a nearby entities callback, use spatial range
    if (nearbyEntitiesCallback_) {
        auto nearby = nearbyEntitiesCallback_(sender, config_.localChatRange);
        for (EntityID entity : nearby) {
            if (entity != sender) {  // Don't echo back to sender
                deliverMessage(registry, entity, message);
            }
        }
    } else {
        // Fallback: deliver to all entities with ChatComponent (zone-wide)
        auto view = registry.view<ChatComponent>();
        for (auto entity : view) {
            if (entity != sender) {
                deliverMessage(registry, entity, message);
            }
        }
    }

    // Also deliver to sender (local echo for their own history)
    deliverMessage(registry, sender, message);
}

void ChatSystem::routeWhisper(Registry& registry, EntityID sender,
                               const ChatMessage& message) {
    // Find target entity by player ID
    EntityID targetEntity = entt::null;
    auto view = registry.view<PlayerInfo>();
    for (auto entity : view) {
        const auto& info = view.get<PlayerInfo>(entity);
        if (info.playerId == message.targetId) {
            targetEntity = entity;
            break;
        }
    }

    if (targetEntity == entt::null) {
        // Target not found — send system message back to sender
        sendSystemMessage(registry, sender, "Player not found or offline.", message.timestampMs);
        return;
    }

    // Deliver to target
    deliverMessage(registry, targetEntity, message);

    // Echo back to sender so they see their own whisper
    deliverMessage(registry, sender, message);
}

void ChatSystem::routeGlobalChat(Registry& registry, const ChatMessage& message) {
    // Deliver to all entities with ChatComponent
    auto view = registry.view<ChatComponent>();
    for (auto entity : view) {
        deliverMessage(registry, entity, message);
    }
}

void ChatSystem::routePartyChat(Registry& registry, EntityID sender,
                                 const ChatMessage& message) {
    // TODO: Implement when party system is added
    // For now, deliver to sender only
    deliverMessage(registry, sender, message);

    // Notify sender that party system is not yet implemented
    sendSystemMessage(registry, sender, "Party system not yet implemented.",
                      message.timestampMs);
}

void ChatSystem::routeGuildChat(Registry& registry, EntityID sender,
                                 const ChatMessage& message) {
    // TODO: Implement when guild system is added
    // For now, deliver to sender only
    deliverMessage(registry, sender, message);

    // Notify sender that guild system is not yet implemented
    sendSystemMessage(registry, sender, "Guild system not yet implemented.",
                      message.timestampMs);
}

} // namespace DarkAges
