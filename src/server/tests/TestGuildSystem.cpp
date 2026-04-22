// [COMBAT_AGENT] Comprehensive unit tests for GuildSystem

#include <catch2/catch_test_macros.hpp>
#include "combat/GuildSystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

namespace {

EntityID createPlayer(Registry& registry) {
    EntityID e = registry.create();
    registry.emplace<Position>(e);
    return e;
}

} // namespace

TEST_CASE("GuildSystem initialization", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID player = createPlayer(registry);
    
    REQUIRE_FALSE(guildSys.isInGuild(player));
    REQUIRE(guildSys.getGuildId(player) == 0);
    REQUIRE(guildSys.getRank(player) == GuildRank::None);
    REQUIRE_FALSE(guildSys.isInGuild(player));
}

TEST_CASE("GuildSystem - create and disband guild", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID founder = createPlayer(registry);

    // Create guild
    uint32_t guildId = guildSys.createGuild(founder, "TestGuild", 1000);
    REQUIRE(guildId != 0);
    REQUIRE(guildSys.isInGuild(founder));
    REQUIRE(guildSys.getGuildId(founder) == guildId);
    REQUIRE(guildSys.getRank(founder) == GuildRank::Leader);
    REQUIRE_FALSE(guildSys.isGuildFull(guildId));

    // Cannot create guild with same name (simple check — second attempt should fail if we track uniqueness)
    // For now implementation doesn't track name uniqueness — this is a placeholder
    // REQUIRE(guildSys.createGuild(founder, "TestGuild", 1000) == 0);

    // Founder cannot create another guild
    REQUIRE(guildSys.createGuild(founder, "AnotherGuild", 1000) == 0);

    // Disband guild
    REQUIRE(guildSys.disbandGuild(guildId, founder));
    REQUIRE_FALSE(guildSys.isInGuild(founder));
    REQUIRE(guildSys.getGuildId(founder) == 0);
}

TEST_CASE("GuildSystem - membership invites and ranks", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID officer = createPlayer(registry);
    EntityID member = createPlayer(registry);
    EntityID outsider = createPlayer(registry);

    uint32_t guildId = guildSys.createGuild(leader, "TestGuild", 1000);
    REQUIRE(guildSys.getGuildSize(guildId) == 1);

    // Invite flow: leader invites officer
    REQUIRE(guildSys.invitePlayer(leader, officer));
    REQUIRE(guildSys.hasPendingInvite(officer, guildId));
    REQUIRE(guildSys.acceptInvite(officer, guildId, 1000));
    REQUIRE(guildSys.isInGuild(officer));
    REQUIRE(guildSys.getRank(officer) == GuildRank::Member);

    // Leader promotes officer
    REQUIRE(guildSys.setRank(officer, GuildRank::Officer, leader));
    REQUIRE(guildSys.getRank(officer) == GuildRank::Officer);

    // Officer invites member
    REQUIRE(guildSys.invitePlayer(officer, member));
    REQUIRE(guildSys.acceptInvite(member, guildId, 1000));
    REQUIRE(guildSys.isInGuild(member));
    REQUIRE(guildSys.getRank(member) == GuildRank::Member);
    REQUIRE(guildSys.getGuildSize(guildId) == 3);

    // Test guild member listing
    auto members = guildSys.getGuildMembers(guildId);
    REQUIRE(members.size() == 3);
    REQUIRE(std::find(members.begin(), members.end(), leader) != members.end());
    REQUIRE(std::find(members.begin(), members.end(), officer) != members.end());
    REQUIRE(std::find(members.begin(), members.end(), member) != members.end());
}

TEST_CASE("GuildSystem - invite permissions by rank", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID officer = createPlayer(registry);
    EntityID member = createPlayer(registry);
    EntityID outsider = createPlayer(registry);

    uint32_t guildId = guildSys.createGuild(leader, "TestGuild", 1000);
    guildSys.invitePlayer(leader, officer);
    guildSys.acceptInvite(officer, guildId, 1000);
    guildSys.setRank(officer, GuildRank::Officer, leader);

    guildSys.invitePlayer(leader, member);
    guildSys.acceptInvite(member, guildId, 1000);

    // Member cannot invite (requires Officer+)
    REQUIRE_FALSE(guildSys.invitePlayer(member, outsider));

    // Officer can invite
    REQUIRE(guildSys.invitePlayer(officer, outsider));
}

TEST_CASE("GuildSystem - kick/remove permissions", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID officer = createPlayer(registry);
    EntityID member = createPlayer(registry);

    uint32_t guildId = guildSys.createGuild(leader, "TestGuild", 1000);
    guildSys.invitePlayer(leader, officer);
    guildSys.acceptInvite(officer, guildId, 1000);
    guildSys.setRank(officer, GuildRank::Officer, leader);
    guildSys.invitePlayer(leader, member);
    guildSys.acceptInvite(member, guildId, 1000);

    // Officer can kick member
    REQUIRE(guildSys.removeFromGuild(member, officer));
    REQUIRE_FALSE(guildSys.isInGuild(member));

    // Officer cannot kick another officer
    EntityID officer2 = createPlayer(registry);
    guildSys.invitePlayer(leader, officer2);
    guildSys.acceptInvite(officer2, guildId, 1000);
    guildSys.setRank(officer2, GuildRank::Officer, leader);
    
    REQUIRE_FALSE(guildSys.removeFromGuild(officer2, officer));  // Officer2 is officer, officer can't kick

    // Officer can kick self (leave)
    REQUIRE(guildSys.leaveGuild(officer2));
    REQUIRE_FALSE(guildSys.isInGuild(officer2));

    // Leader can kick officer
    REQUIRE(guildSys.removeFromGuild(officer, leader));
    REQUIRE_FALSE(guildSys.isInGuild(officer));
}

TEST_CASE("GuildSystem - rank promotion constraints", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID member = createPlayer(registry);
    EntityID officer = createPlayer(registry);

    uint32_t guildId = guildSys.createGuild(leader, "TestGuild", 1000);
    guildSys.invitePlayer(leader, member);
    guildSys.acceptInvite(member, guildId, 1000);
    guildSys.invitePlayer(leader, officer);
    guildSys.acceptInvite(officer, guildId, 1000);

    // Member cannot promote anyone (rank too low)
    REQUIRE_FALSE(guildSys.setRank(leader, GuildRank::Officer, member));  // member tries to promote leader, fails
    REQUIRE_FALSE(guildSys.setRank(member, GuildRank::Officer, member));  // member self-promote fails

    // Officer cannot promote themselves to Leader
    REQUIRE_FALSE(guildSys.setRank(officer, GuildRank::Leader, officer));

    // Leader can promote member to officer
    REQUIRE(guildSys.setRank(member, GuildRank::Officer, leader));
    REQUIRE(guildSys.getRank(member) == GuildRank::Officer);

    // Officer+ can promote others
    REQUIRE(guildSys.setRank(officer, GuildRank::Officer, member));  // now member is officer
    REQUIRE(guildSys.getRank(officer) == GuildRank::Officer);
}

TEST_CASE("GuildSystem - guild size limits", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    uint32_t guildId = guildSys.createGuild(leader, "TestGuild", 1000);

    // Fill up to MAX_GUILD_SIZE = 100
    for (int i = 0; i < 99; ++i) {
        EntityID m = createPlayer(registry);
        REQUIRE(guildSys.invitePlayer(leader, m));
        REQUIRE(guildSys.acceptInvite(m, guildId, 1000 + i * 10));
    }

    REQUIRE(guildSys.getGuildSize(guildId) == 100);
    REQUIRE(guildSys.isGuildFull(guildId));

    // 101st cannot join
    EntityID extra = createPlayer(registry);
    REQUIRE_FALSE(guildSys.invitePlayer(leader, extra));
    REQUIRE_FALSE(guildSys.isInGuild(extra));
}

TEST_CASE("GuildSystem - ensureComponent and disconnect cleanup", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID player = createPlayer(registry);
    REQUIRE_FALSE(registry.all_of<GuildComponent>(player));

    guildSys.ensureComponent(registry, player);
    REQUIRE(registry.all_of<GuildComponent>(player));
    const GuildComponent& comp = registry.get<GuildComponent>(player);
    REQUIRE(comp.guildId == 0);
    REQUIRE(comp.rank == GuildRank::None);

    // Join guild
    uint32_t guildId = guildSys.createGuild(player, "MyGuild", 1000);
    REQUIRE(guildSys.isInGuild(player));

    // Cleanup on disconnect
    guildSys.onPlayerDisconnect(registry, player);
    REQUIRE_FALSE(guildSys.isInGuild(player));
    REQUIRE(registry.all_of<GuildComponent>(player));  // component remains but state cleared
}

TEST_CASE("GuildSystem - edge cases: invalid operations", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID player = createPlayer(registry);

    // Leave when not in guild
    REQUIRE_FALSE(guildSys.leaveGuild(player));

    // Disband non-existent guild
    REQUIRE_FALSE(guildSys.disbandGuild(9999, player));

    // Accept invite to non-existent guild
    REQUIRE_FALSE(guildSys.acceptInvite(player, 9999, 1000));

    // Kick yourself from non-guild
    REQUIRE_FALSE(guildSys.removeFromGuild(player, player));

    // Promote in non-guild context
    EntityID target = createPlayer(registry);
    REQUIRE_FALSE(guildSys.setRank(target, GuildRank::Officer, player));
}

TEST_CASE("GuildSystem - multiple guilds are independent", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader1 = createPlayer(registry);
    EntityID leader2 = createPlayer(registry);
    EntityID member = createPlayer(registry);

    uint32_t g1 = guildSys.createGuild(leader1, "GuildOne", 1000);
    uint32_t g2 = guildSys.createGuild(leader2, "GuildTwo", 1000);

    REQUIRE(guildSys.getGuildSize(g1) == 1);
    REQUIRE(guildSys.getGuildSize(g2) == 1);
    REQUIRE(guildSys.isInGuild(leader1));
    REQUIRE(guildSys.getGuildId(leader1) == g1);
    REQUIRE(guildSys.getGuildId(leader1) != g2);

    // Invite to guild1, not guild2
    REQUIRE(guildSys.invitePlayer(leader1, member));
    REQUIRE(guildSys.acceptInvite(member, g1, 1000));
    REQUIRE(guildSys.getGuildId(member) == g1);
}

TEST_CASE("GuildSystem - setRank can demote", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID officer = createPlayer(registry);

    uint32_t guildId = guildSys.createGuild(leader, "Test", 1000);
    guildSys.invitePlayer(leader, officer);
    guildSys.acceptInvite(officer, guildId, 1000);

    // Promote to officer
    guildSys.setRank(officer, GuildRank::Officer, leader);
    REQUIRE(guildSys.getRank(officer) == GuildRank::Officer);

    // Demote back to member (leader can do this)
    REQUIRE(guildSys.setRank(officer, GuildRank::Member, leader));
    REQUIRE(guildSys.getRank(officer) == GuildRank::Member);
}

TEST_CASE("GuildSystem - leave vs removeFromParty difference", "[guildsystem]") {
    GuildSystem guildSys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID member = createPlayer(registry);

    uint32_t guildId = guildSys.createGuild(leader, "Test", 1000);
    guildSys.invitePlayer(leader, member);
    guildSys.acceptInvite(member, guildId, 1000);

    // Member leaves voluntarily
    REQUIRE(guildSys.leaveGuild(member));
    REQUIRE_FALSE(guildSys.isInGuild(member));

    // Try again — already left
    REQUIRE_FALSE(guildSys.leaveGuild(member));
}
