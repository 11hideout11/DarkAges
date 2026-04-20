// ChatSystem unit tests
// Tests message routing, rate limiting, delivery, and channel behavior

#include <catch2/catch_test_macros.hpp>
#include "combat/ChatSystem.hpp"
#include "ecs/Components.hpp"

using namespace DarkAges;

// Helper: create a test player entity with ChatComponent and PlayerInfo
static EntityID createTestPlayer(Registry& registry, const char* name,
                                  uint64_t playerId = 0) {
    auto entity = registry.create();
    registry.emplace<Position>(entity);
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);

    PlayerInfo info{};
    info.playerId = playerId != 0 ? playerId : static_cast<uint64_t>(entity);
    info.connectionId = static_cast<uint32_t>(entity) + 1000;
    std::strncpy(info.username, name, 31);
    info.username[31] = '\0';
    registry.emplace<PlayerInfo>(entity, info);

    registry.emplace<ChatComponent>(entity);
    return entity;
}

// ============================================================================
// TEST: Chat Message Types and Components
// ============================================================================

TEST_CASE("ChatMessage default construction", "[chat]") {
    ChatMessage msg;
    CHECK(msg.messageId == 0);
    CHECK(msg.channel == ChatChannel::Local);
    CHECK(msg.senderId == 0);
    CHECK(msg.targetId == 0);
    CHECK(msg.timestampMs == 0);
    CHECK(msg.senderName[0] == '\0');
    CHECK(msg.content[0] == '\0');
}

TEST_CASE("ChatComponent ring buffer", "[chat]") {
    ChatComponent chat;

    // Add messages
    for (uint32_t i = 0; i < 10; ++i) {
        ChatMessage msg;
        msg.messageId = i + 1;
        msg.timestampMs = i * 1000;
        chat.addMessage(msg);
    }

    CHECK(chat.messageCount == 10);

    // Verify messages are stored correctly
    CHECK(chat.recentMessages[0].messageId == 1);
    CHECK(chat.recentMessages[9].messageId == 10);
}

TEST_CASE("ChatComponent ring buffer overflow", "[chat]") {
    ChatComponent chat;

    // Add more messages than buffer size
    for (uint32_t i = 0; i < CHAT_HISTORY_SIZE + 20; ++i) {
        ChatMessage msg;
        msg.messageId = i + 1;
        chat.addMessage(msg);
    }

    CHECK(chat.messageCount == CHAT_HISTORY_SIZE + 20);

    // The buffer should wrap around
    // Last CHAT_HISTORY_SIZE messages should be present
    uint32_t lastWrittenIdx = (CHAT_HISTORY_SIZE + 19) % CHAT_HISTORY_SIZE;
    CHECK(chat.recentMessages[lastWrittenIdx].messageId == CHAT_HISTORY_SIZE + 20);
}

TEST_CASE("ChatConfig default values", "[chat]") {
    ChatConfig config;
    CHECK(config.maxMessagesPerWindow == 5);
    CHECK(config.rateWindowMs == 10000);
    CHECK(config.localChatRange == 50.0f);
    CHECK(config.globalChatEnabled == true);
    CHECK(config.systemMessagesEnabled == true);
}

// ============================================================================
// TEST: Chat System Basic Operations
// ============================================================================

TEST_CASE("ChatSystem send local message", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player1 = createTestPlayer(registry, "Alice", 100);
    auto player2 = createTestPlayer(registry, "Bob", 201);

    // Track delivered messages
    uint32_t deliveryCount = 0;
    chatSystem.setMessageDeliveryCallback(
        [&](ConnectionID, const ChatMessage&) { deliveryCount++; });
    chatSystem.setConnectionLookupCallback(
        [&](EntityID entity) -> ConnectionID {
            const auto* info = registry.try_get<PlayerInfo>(entity);
            return info ? info->connectionId : 0;
        });

    bool sent = chatSystem.sendMessage(registry, player1, ChatChannel::Local,
                                        "Hello everyone!", 1000);
    CHECK(sent);

    // Both players should have the message in their history
    auto msgs1 = chatSystem.getRecentMessages(registry, player1);
    auto msgs2 = chatSystem.getRecentMessages(registry, player2);

    // Player1 gets echo, player2 gets the message
    CHECK(msgs1.size() == 1);
    CHECK(msgs2.size() == 1);
    CHECK(std::string(msgs2[0].content) == "Hello everyone!");
    CHECK(std::string(msgs2[0].senderName) == "Alice");
}

TEST_CASE("ChatSystem send whisper", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player1 = createTestPlayer(registry, "Alice", 100);
    auto player2 = createTestPlayer(registry, "Bob", 201);

    chatSystem.setConnectionLookupCallback(
        [&](EntityID entity) -> ConnectionID {
            const auto* info = registry.try_get<PlayerInfo>(entity);
            return info ? info->connectionId : 0;
        });

    bool sent = chatSystem.sendMessage(registry, player1, ChatChannel::Whisper,
                                        "Secret message", 1000, 201);
    CHECK(sent);

    auto msgs2 = chatSystem.getRecentMessages(registry, player2);
    CHECK(msgs2.size() == 1);
    CHECK(msgs2[0].channel == ChatChannel::Whisper);
    CHECK(std::string(msgs2[0].content) == "Secret message");
    CHECK(msgs2[0].targetId == 201);
}

TEST_CASE("ChatSystem whisper to nonexistent player", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player1 = createTestPlayer(registry, "Alice", 100);

    chatSystem.setConnectionLookupCallback(
        [&](EntityID entity) -> ConnectionID {
            const auto* info = registry.try_get<PlayerInfo>(entity);
            return info ? info->connectionId : 0;
        });

    // Whisper to a player ID that doesn't exist
    bool sent = chatSystem.sendMessage(registry, player1, ChatChannel::Whisper,
                                        "Hello?", 1000, 999);
    CHECK(sent);

    // Sender should get a system message "Player not found or offline."
    auto msgs = chatSystem.getRecentMessages(registry, player1);
    bool foundError = false;
    for (const auto& m : msgs) {
        if (m.channel == ChatChannel::System &&
            std::string(m.content) == "Player not found or offline.") {
            foundError = true;
            break;
        }
    }
    CHECK(foundError);
}

TEST_CASE("ChatSystem system message", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player = createTestPlayer(registry, "Alice", 100);

    chatSystem.setConnectionLookupCallback(
        [&](EntityID entity) -> ConnectionID {
            const auto* info = registry.try_get<PlayerInfo>(entity);
            return info ? info->connectionId : 0;
        });

    chatSystem.sendSystemMessage(registry, player, "Welcome to DarkAges!", 1000);

    auto msgs = chatSystem.getRecentMessages(registry, player);
    CHECK(msgs.size() == 1);
    CHECK(msgs[0].channel == ChatChannel::System);
    CHECK(msgs[0].senderId == 0);
    CHECK(std::string(msgs[0].senderName) == "System");
    CHECK(std::string(msgs[0].content) == "Welcome to DarkAges!");
}

TEST_CASE("ChatSystem broadcast system message", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto p1 = createTestPlayer(registry, "Alice", 100);
    auto p2 = createTestPlayer(registry, "Bob", 201);
    auto p3 = createTestPlayer(registry, "Charlie", 302);

    chatSystem.broadcastSystemMessage(registry, "Server restart in 5 minutes", 1000);

    // All players should have the message
    auto m1 = chatSystem.getRecentMessages(registry, p1);
    auto m2 = chatSystem.getRecentMessages(registry, p2);
    auto m3 = chatSystem.getRecentMessages(registry, p3);

    CHECK(m1.size() == 1);
    CHECK(m2.size() == 1);
    CHECK(m3.size() == 1);
    CHECK(std::string(m1[0].content) == "Server restart in 5 minutes");
}

// ============================================================================
// TEST: Rate Limiting
// ============================================================================

TEST_CASE("ChatSystem rate limiting", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    ChatConfig config;
    config.maxMessagesPerWindow = 3;
    config.rateWindowMs = 10000;  // 10 second window
    chatSystem.setConfig(config);

    auto player = createTestPlayer(registry, "Spammer", 100);

    chatSystem.setConnectionLookupCallback([&](EntityID) -> ConnectionID { return 0; });

    // Send 3 messages — should all succeed
    CHECK(chatSystem.sendMessage(registry, player, ChatChannel::Local, "msg1", 1000));
    CHECK(chatSystem.sendMessage(registry, player, ChatChannel::Local, "msg2", 1100));
    CHECK(chatSystem.sendMessage(registry, player, ChatChannel::Local, "msg3", 1200));

    // 4th message should be rate limited
    CHECK_FALSE(chatSystem.sendMessage(registry, player, ChatChannel::Local, "msg4", 1300));

    // After rate window expires, should succeed again
    CHECK(chatSystem.sendMessage(registry, player, ChatChannel::Local, "msg5", 12000));

    CHECK(chatSystem.getTotalRateLimited() == 1);
}

// ============================================================================
// TEST: Muting
// ============================================================================

TEST_CASE("ChatSystem muting", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player = createTestPlayer(registry, "MutedPlayer", 100);

    chatSystem.setConnectionLookupCallback([&](EntityID) -> ConnectionID { return 0; });

    // Mute the player
    chatSystem.setMuted(registry, player, true);
    CHECK(chatSystem.isMuted(registry, player));

    // Muted player cannot send messages
    CHECK_FALSE(chatSystem.sendMessage(registry, player, ChatChannel::Local,
                                        "I am muted", 1000));

    // Unmute
    chatSystem.setMuted(registry, player, false);
    CHECK_FALSE(chatSystem.isMuted(registry, player));

    // Now can send
    CHECK(chatSystem.sendMessage(registry, player, ChatChannel::Local,
                                  "I am unmuted", 2000));
}

// ============================================================================
// TEST: Content Validation
// ============================================================================

TEST_CASE("ChatSystem rejects empty messages", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player = createTestPlayer(registry, "Alice", 100);
    chatSystem.setConnectionLookupCallback([&](EntityID) -> ConnectionID { return 0; });

    CHECK_FALSE(chatSystem.sendMessage(registry, player, ChatChannel::Local, "", 1000));
    CHECK_FALSE(chatSystem.sendMessage(registry, player, ChatChannel::Local, nullptr, 1000));
}

TEST_CASE("ChatSystem rejects overly long messages", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player = createTestPlayer(registry, "Alice", 100);
    chatSystem.setConnectionLookupCallback([&](EntityID) -> ConnectionID { return 0; });

    // Create a message that's exactly at the limit
    char longMsg[CHAT_MESSAGE_MAX_LEN + 10];
    std::memset(longMsg, 'a', sizeof(longMsg) - 1);
    longMsg[sizeof(longMsg) - 1] = '\0';

    CHECK_FALSE(chatSystem.sendMessage(registry, player, ChatChannel::Local, longMsg, 1000));
}

// ============================================================================
// TEST: Message History
// ============================================================================

TEST_CASE("ChatSystem message history", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto p1 = createTestPlayer(registry, "Alice", 100);
    auto p2 = createTestPlayer(registry, "Bob", 201);

    chatSystem.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        const auto* info = registry.try_get<PlayerInfo>(entity);
        return info ? info->connectionId : 0;
    });

    // Send several messages
    for (int i = 0; i < 5; ++i) {
        std::string content = "Message " + std::to_string(i);
        chatSystem.sendMessage(registry, p1, ChatChannel::Local, content.c_str(), 1000 + i * 100);
    }

    // Get recent messages for p2
    auto msgs = chatSystem.getRecentMessages(registry, p2, 3);
    CHECK(msgs.size() == 3);

    // Should be the last 3 messages
    CHECK(std::string(msgs[0].content) == "Message 2");
    CHECK(std::string(msgs[1].content) == "Message 3");
    CHECK(std::string(msgs[2].content) == "Message 4");
}

// ============================================================================
// TEST: Statistics
// ============================================================================

TEST_CASE("ChatSystem statistics tracking", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto p1 = createTestPlayer(registry, "Alice", 100);
    auto p2 = createTestPlayer(registry, "Bob", 201);

    chatSystem.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        const auto* info = registry.try_get<PlayerInfo>(entity);
        return info ? info->connectionId : 0;
    });

    // Send 2 messages
    chatSystem.sendMessage(registry, p1, ChatChannel::Local, "Hello", 1000);
    chatSystem.sendMessage(registry, p2, ChatChannel::Local, "Hi", 1100);

    CHECK(chatSystem.getTotalMessagesProcessed() == 2);
    CHECK(chatSystem.getTotalMessagesDelivered() > 0);
}

// ============================================================================
// TEST: Channel Behavior
// ============================================================================

TEST_CASE("ChatSystem global chat disabled", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    ChatConfig config;
    config.globalChatEnabled = false;
    chatSystem.setConfig(config);

    auto player = createTestPlayer(registry, "Alice", 100);
    chatSystem.setConnectionLookupCallback([&](EntityID) -> ConnectionID { return 0; });

    // Global chat should fail when disabled
    CHECK_FALSE(chatSystem.sendMessage(registry, player, ChatChannel::Global,
                                        "Global message", 1000));

    // But local should still work
    CHECK(chatSystem.sendMessage(registry, player, ChatChannel::Local,
                                  "Local message", 1000));
}

TEST_CASE("ChatSystem party chat not yet implemented", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player = createTestPlayer(registry, "Alice", 100);
    chatSystem.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        const auto* info = registry.try_get<PlayerInfo>(entity);
        return info ? info->connectionId : 0;
    });

    // Party chat should still accept the message (but echo + system warning)
    bool sent = chatSystem.sendMessage(registry, player, ChatChannel::Party,
                                        "Party message", 1000);
    CHECK(sent);

    // Should have a system message about party not implemented
    auto msgs = chatSystem.getRecentMessages(registry, player);
    bool foundWarning = false;
    for (const auto& m : msgs) {
        if (m.channel == ChatChannel::System &&
            std::string(m.content) == "Party system not yet implemented.") {
            foundWarning = true;
            break;
        }
    }
    CHECK(foundWarning);
}

TEST_CASE("ChatSystem guild chat not yet implemented", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto player = createTestPlayer(registry, "Alice", 100);
    chatSystem.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        const auto* info = registry.try_get<PlayerInfo>(entity);
        return info ? info->connectionId : 0;
    });

    bool sent = chatSystem.sendMessage(registry, player, ChatChannel::Guild,
                                        "Guild message", 1000);
    CHECK(sent);

    auto msgs = chatSystem.getRecentMessages(registry, player);
    bool foundWarning = false;
    for (const auto& m : msgs) {
        if (m.channel == ChatChannel::System &&
            std::string(m.content) == "Guild system not yet implemented.") {
            foundWarning = true;
            break;
        }
    }
    CHECK(foundWarning);
}

// ============================================================================
// TEST: Multiple Players Local Chat
// ============================================================================

TEST_CASE("ChatSystem local chat reaches multiple players", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto sender = createTestPlayer(registry, "Sender", 100);
    auto p1 = createTestPlayer(registry, "Player1", 201);
    auto p2 = createTestPlayer(registry, "Player2", 302);
    auto p3 = createTestPlayer(registry, "Player3", 403);

    chatSystem.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        const auto* info = registry.try_get<PlayerInfo>(entity);
        return info ? info->connectionId : 0;
    });

    chatSystem.sendMessage(registry, sender, ChatChannel::Local,
                           "Hello zone!", 1000);

    // All players should have the message
    auto m1 = chatSystem.getRecentMessages(registry, p1);
    auto m2 = chatSystem.getRecentMessages(registry, p2);
    auto m3 = chatSystem.getRecentMessages(registry, p3);

    CHECK(m1.size() == 1);
    CHECK(m2.size() == 1);
    CHECK(m3.size() == 1);
    CHECK(std::string(m1[0].content) == "Hello zone!");
    CHECK(std::string(m2[0].content) == "Hello zone!");
    CHECK(std::string(m3[0].content) == "Hello zone!");
}

// ============================================================================
// TEST: Whisper Bidirectional
// ============================================================================

TEST_CASE("ChatSystem whisper shows in both sender and receiver history", "[chat]") {
    Registry registry;
    ChatSystem chatSystem;

    auto alice = createTestPlayer(registry, "Alice", 100);
    auto bob = createTestPlayer(registry, "Bob", 201);

    chatSystem.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        const auto* info = registry.try_get<PlayerInfo>(entity);
        return info ? info->connectionId : 0;
    });

    // Alice whispers to Bob
    chatSystem.sendMessage(registry, alice, ChatChannel::Whisper,
                           "Hey Bob, want to party?", 1000, 201);

    // Bob whispers back
    chatSystem.sendMessage(registry, bob, ChatChannel::Whisper,
                           "Sure, meet at the tavern!", 2000, 100);

    // Both should have 2 messages in history
    auto aliceMsgs = chatSystem.getRecentMessages(registry, alice);
    auto bobMsgs = chatSystem.getRecentMessages(registry, bob);

    CHECK(aliceMsgs.size() == 2);
    CHECK(bobMsgs.size() == 2);

    // Alice's first message is her whisper, second is Bob's reply
    CHECK(std::string(aliceMsgs[0].content) == "Hey Bob, want to party?");
    CHECK(std::string(aliceMsgs[1].content) == "Sure, meet at the tavern!");

    // Bob's first message is Alice's whisper, second is his reply
    CHECK(std::string(bobMsgs[0].content) == "Hey Bob, want to party?");
    CHECK(std::string(bobMsgs[1].content) == "Sure, meet at the tavern!");
}
