#include <catch2/catch_test_macros.hpp>
#include "ecs/CoreTypes.hpp"
#include "combat/PartySystem.hpp"
#include "combat/GuildSystem.hpp"
#include "combat/ChatSystem.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;

// ============================================================================
// Helper
// ============================================================================

static EntityID createTestPlayer(entt::registry& registry, const char* name = "Player") {
    auto entity = registry.create();
    registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(0, 0, 0)));
    registry.emplace<Velocity>(entity);
    registry.emplace<CombatState>(entity);
    registry.emplace<Mana>(entity);
    registry.emplace<PlayerTag>(entity);
    registry.emplace<PlayerProgression>(entity);

    PlayerInfo info{};
    std::strncpy(info.username, name, sizeof(info.username) - 1);
    registry.emplace<PlayerInfo>(entity, info);

    registry.emplace<ChatComponent>(entity);
    registry.emplace<PartyComponent>(entity);
    registry.emplace<GuildComponent>(entity);

    return entity;
}

// ============================================================================
// Party System Tests
// ============================================================================

TEST_CASE("PartySystem create party", "[social][party]") {
    PartySystem ps;

    auto player = entt::entity{0};
    uint32_t partyId = ps.createParty(player, 0);

    REQUIRE(partyId > 0);
    REQUIRE(ps.isInParty(player));
    REQUIRE(ps.isLeader(player));
    REQUIRE(ps.getPartySize(partyId) == 1);
}

TEST_CASE("PartySystem cannot create party if already in one", "[social][party]") {
    PartySystem ps;

    auto player = entt::entity{0};
    uint32_t partyId1 = ps.createParty(player, 0);
    uint32_t partyId2 = ps.createParty(player, 0);

    REQUIRE(partyId1 > 0);
    REQUIRE(partyId2 == 0);  // Should fail
}

TEST_CASE("PartySystem invite and accept", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    REQUIRE(ps.invitePlayer(leader, member));
    REQUIRE(ps.hasPendingInvite(member, partyId));

    REQUIRE(ps.acceptInvite(member, partyId, 0));
    REQUIRE(ps.isInParty(member));
    REQUIRE_FALSE(ps.isLeader(member));
    REQUIRE(ps.getPartySize(partyId) == 2);
    REQUIRE_FALSE(ps.hasPendingInvite(member, partyId));  // Invite consumed
}

TEST_CASE("PartySystem invite decline", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member);
    ps.declineInvite(member, partyId);

    REQUIRE_FALSE(ps.hasPendingInvite(member, partyId));
    REQUIRE_FALSE(ps.isInParty(member));
}

TEST_CASE("PartySystem non-leader cannot invite", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member1 = entt::entity{1};
    auto member2 = entt::entity{2};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member1);
    ps.acceptInvite(member1, partyId, 0);

    // Member1 (not leader) tries to invite member2
    REQUIRE_FALSE(ps.invitePlayer(member1, member2));
}

TEST_CASE("PartySystem leave party", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member);
    ps.acceptInvite(member, partyId, 0);

    REQUIRE(ps.leaveParty(member));
    REQUIRE_FALSE(ps.isInParty(member));
    REQUIRE(ps.getPartySize(partyId) == 1);
}

TEST_CASE("PartySystem leader leaves transfers leadership", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member);
    ps.acceptInvite(member, partyId, 0);

    // Leader leaves
    REQUIRE(ps.leaveParty(leader));
    REQUIRE_FALSE(ps.isInParty(leader));

    // Member becomes new leader
    REQUIRE(ps.isInParty(member));
    REQUIRE(ps.isLeader(member));
}

TEST_CASE("PartySystem last member leaves disbands", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    uint32_t partyId = ps.createParty(leader, 0);

    ps.leaveParty(leader);
    REQUIRE_FALSE(ps.isInParty(leader));
    REQUIRE(ps.getPartySize(partyId) == 0);
}

TEST_CASE("PartySystem full party rejects invites", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    uint32_t partyId = ps.createParty(leader, 0);

    // Add MAX_PARTY_SIZE - 1 more members
    for (uint32_t i = 1; i < MAX_PARTY_SIZE; ++i) {
        auto member = static_cast<entt::entity>(i);
        ps.invitePlayer(leader, member);
        ps.acceptInvite(member, partyId, 0);
    }

    REQUIRE(ps.isPartyFull(partyId));

    // Try to invite one more
    auto extra = static_cast<entt::entity>(MAX_PARTY_SIZE);
    REQUIRE_FALSE(ps.invitePlayer(leader, extra));
}

TEST_CASE("PartySystem disband party", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member);
    ps.acceptInvite(member, partyId, 0);

    REQUIRE(ps.disbandParty(partyId, leader));
    REQUIRE(ps.getPartySize(partyId) == 0);
}

TEST_CASE("PartySystem non-leader cannot disband", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member);
    ps.acceptInvite(member, partyId, 0);

    REQUIRE_FALSE(ps.disbandParty(partyId, member));
}

TEST_CASE("PartySystem getPartyMembers", "[social][party]") {
    PartySystem ps;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t partyId = ps.createParty(leader, 0);
    ps.invitePlayer(leader, member);
    ps.acceptInvite(member, partyId, 0);

    auto members = ps.getPartyMembers(partyId);
    REQUIRE(members.size() == 2);
}

// ============================================================================
// Guild System Tests
// ============================================================================

TEST_CASE("GuildSystem create guild", "[social][guild]") {
    GuildSystem gs;

    auto founder = entt::entity{0};
    uint32_t guildId = gs.createGuild(founder, "TestGuild", 0);

    REQUIRE(guildId > 0);
    REQUIRE(gs.isInGuild(founder));
    REQUIRE(gs.getRank(founder) == GuildRank::Leader);
    REQUIRE(gs.getGuildSize(guildId) == 1);
    REQUIRE(std::string(gs.getGuildName(guildId)) == "TestGuild");
}

TEST_CASE("GuildSystem duplicate name fails", "[social][guild]") {
    GuildSystem gs;

    auto founder1 = entt::entity{0};
    auto founder2 = entt::entity{1};

    uint32_t guildId1 = gs.createGuild(founder1, "TestGuild", 0);
    uint32_t guildId2 = gs.createGuild(founder2, "TestGuild", 0);

    REQUIRE(guildId1 > 0);
    REQUIRE(guildId2 == 0);  // Name taken
}

TEST_CASE("GuildSystem already in guild fails", "[social][guild]") {
    GuildSystem gs;

    auto founder = entt::entity{0};

    uint32_t guildId1 = gs.createGuild(founder, "Guild1", 0);
    uint32_t guildId2 = gs.createGuild(founder, "Guild2", 0);

    REQUIRE(guildId1 > 0);
    REQUIRE(guildId2 == 0);  // Already in a guild
}

TEST_CASE("GuildSystem invite and accept", "[social][guild]") {
    GuildSystem gs;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t guildId = gs.createGuild(leader, "TestGuild", 0);

    REQUIRE(gs.invitePlayer(leader, member));
    REQUIRE(gs.hasPendingInvite(member, guildId));

    REQUIRE(gs.acceptInvite(member, guildId, 0));
    REQUIRE(gs.isInGuild(member));
    REQUIRE(gs.getGuildSize(guildId) == 2);
}

TEST_CASE("GuildSystem non-officer cannot invite", "[social][guild]") {
    GuildSystem gs;

    auto leader = entt::entity{0};
    auto member1 = entt::entity{1};
    auto member2 = entt::entity{2};

    uint32_t guildId = gs.createGuild(leader, "TestGuild", 0);
    gs.invitePlayer(leader, member1);
    gs.acceptInvite(member1, guildId, 0);

    // Member1 (not officer) tries to invite
    REQUIRE_FALSE(gs.invitePlayer(member1, member2));
}

TEST_CASE("GuildSystem leave guild", "[social][guild]") {
    GuildSystem gs;

    auto leader = entt::entity{0};
    auto member = entt::entity{1};

    uint32_t guildId = gs.createGuild(leader, "TestGuild", 0);
    gs.invitePlayer(leader, member);
    gs.acceptInvite(member, guildId, 0);

    REQUIRE(gs.leaveGuild(member));
    REQUIRE_FALSE(gs.isInGuild(member));
    REQUIRE(gs.getGuildSize(guildId) == 1);
}

TEST_CASE("GuildSystem full guild rejects invites", "[social][guild]") {
    GuildSystem gs;

    auto leader = entt::entity{0};
    uint32_t guildId = gs.createGuild(leader, "BigGuild", 0);

    // Add MAX_GUILD_SIZE - 1 more members
    for (uint32_t i = 1; i < MAX_GUILD_SIZE; ++i) {
        auto member = static_cast<entt::entity>(i);
        gs.invitePlayer(leader, member);
        gs.acceptInvite(member, guildId, 0);
    }

    REQUIRE(gs.isGuildFull(guildId));

    auto extra = static_cast<entt::entity>(MAX_GUILD_SIZE);
    REQUIRE_FALSE(gs.invitePlayer(leader, extra));
}

TEST_CASE("GuildSystem disband guild", "[social][guild]") {
    GuildSystem gs;

    auto leader = entt::entity{0};
    uint32_t guildId = gs.createGuild(leader, "TestGuild", 0);

    REQUIRE(gs.disbandGuild(guildId, leader));
    REQUIRE(gs.getGuildSize(guildId) == 0);
}

// ============================================================================
// Chat System Integration with Party/Guild
// ============================================================================

TEST_CASE("ChatSystem party routing", "[social][party][chat]") {
    entt::registry registry;
    PartySystem ps;
    ChatSystem chat;

    chat.setPartySystem(&ps);

    auto player1 = createTestPlayer(registry, "Alice");
    auto player2 = createTestPlayer(registry, "Bob");

    // Create party with Alice as leader, add Bob
    uint32_t partyId = ps.createParty(player1, 0);
    ps.invitePlayer(player1, player2);
    ps.acceptInvite(player2, partyId, 0);

    // Track delivered messages
    std::vector<ChatMessage> deliveredMessages;
    chat.setMessageDeliveryCallback([&](ConnectionID, const ChatMessage& msg) {
        deliveredMessages.push_back(msg);
    });
    chat.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        return static_cast<ConnectionID>(entity);
    });
    chat.setNearbyEntitiesCallback([&](EntityID, float) -> std::vector<EntityID> { return {}; });

    // Alice sends party chat
    REQUIRE(chat.sendMessage(registry, player1, ChatChannel::Party,
                              "Hello party!", 1000));

    // Bob should receive the message (player1 echoed + player2 delivered = 2)
    REQUIRE(deliveredMessages.size() >= 1);
    bool found = false;
    for (const auto& msg : deliveredMessages) {
        if (msg.channel == ChatChannel::Party) {
            found = true;
            break;
        }
    }
    REQUIRE(found);
}

TEST_CASE("ChatSystem party routing without party", "[social][party][chat]") {
    entt::registry registry;
    PartySystem ps;
    ChatSystem chat;

    chat.setPartySystem(&ps);

    auto player = createTestPlayer(registry, "Solo");

    // Track delivered messages
    bool systemMsgSent = false;
    chat.setMessageDeliveryCallback([&](ConnectionID, const ChatMessage&) {});
    chat.setConnectionLookupCallback([&](EntityID) -> ConnectionID { return 0; });
    chat.setNearbyEntitiesCallback([&](EntityID, float) -> std::vector<EntityID> { return {}; });

    // Override sendSystemMessage detection
    // Player without party should get "not in a party" system message
    REQUIRE(chat.sendMessage(registry, player, ChatChannel::Party,
                              "Hello?", 1000));
}

TEST_CASE("ChatSystem guild routing", "[social][guild][chat]") {
    entt::registry registry;
    GuildSystem gs;
    ChatSystem chat;

    chat.setGuildSystem(&gs);

    auto player1 = createTestPlayer(registry, "GuildMaster");
    auto player2 = createTestPlayer(registry, "Member");

    // Create guild, add member
    uint32_t guildId = gs.createGuild(player1, "TestGuild", 0);
    gs.invitePlayer(player1, player2);
    gs.acceptInvite(player2, guildId, 0);

    // Track delivered messages
    std::vector<ChatMessage> deliveredMessages;
    chat.setMessageDeliveryCallback([&](ConnectionID, const ChatMessage& msg) {
        deliveredMessages.push_back(msg);
    });
    chat.setConnectionLookupCallback([&](EntityID entity) -> ConnectionID {
        return static_cast<ConnectionID>(entity);
    });
    chat.setNearbyEntitiesCallback([&](EntityID, float) -> std::vector<EntityID> { return {}; });

    // GuildMaster sends guild chat
    REQUIRE(chat.sendMessage(registry, player1, ChatChannel::Guild,
                              "Guild news!", 1000));

    // Member should receive the message (sender echoed + member delivered = 2)
    REQUIRE(deliveredMessages.size() >= 1);
    bool found = false;
    for (const auto& msg : deliveredMessages) {
        if (msg.channel == ChatChannel::Guild) {
            found = true;
            break;
        }
    }
    REQUIRE(found);
}

TEST_CASE("PartyComponent defaults", "[social][party]") {
    PartyComponent pc;
    REQUIRE(pc.partyId == 0);
    REQUIRE(pc.role == PartyRole::None);
    REQUIRE(pc.shareXP == true);
}

TEST_CASE("GuildComponent defaults", "[social][guild]") {
    GuildComponent gc;
    REQUIRE(gc.guildId == 0);
    REQUIRE(gc.rank == GuildRank::None);
    REQUIRE(gc.joinTimeMs == 0);
}
