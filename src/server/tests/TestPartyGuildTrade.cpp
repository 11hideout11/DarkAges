// [COMBAT_AGENT] Unit tests for Party, Guild, and Trade Systems

#include <catch2/catch_test_macros.hpp>
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <string>
#include <vector>

using namespace DarkAges;

// ============================================================================
// Party System Tests
// ============================================================================

TEST_CASE("PartyRole enum values", "[party]") {
    REQUIRE(static_cast<uint8_t>(PartyRole::None) == 0);
    REQUIRE(static_cast<uint8_t>(PartyRole::Member) == 1);
    REQUIRE(static_cast<uint8_t>(PartyRole::Leader) == 2);
}

TEST_CASE("PartyComponent initialization", "[party]") {
    PartyComponent party;
    REQUIRE(party.partyId == 0);
    REQUIRE(party.role == PartyRole::None);
    REQUIRE(party.shareXP == true);
}

TEST_CASE("PartyComponent - Join party", "[party]") {
    PartyComponent party;
    party.partyId = 1001;
    party.role = PartyRole::Member;
    party.shareXP = true;
    
    REQUIRE(party.partyId == 1001);
    REQUIRE(party.role == PartyRole::Member);
    REQUIRE(party.shareXP == true);
}

TEST_CASE("PartyComponent - Party leader", "[party]") {
    PartyComponent party;
    party.partyId = 1001;
    party.role = PartyRole::Leader;
    
    REQUIRE(party.partyId == 1001);
    REQUIRE(party.role == PartyRole::Leader);
}

TEST_CASE("Party - Leave party", "[party]") {
    PartyComponent party;
    party.partyId = 1001;
    party.role = PartyRole::Member;
    
    party.partyId = 0;
    party.role = PartyRole::None;
    
    REQUIRE(party.partyId == 0);
    REQUIRE(party.role == PartyRole::None);
}

TEST_CASE("PartyConstants", "[party]") {
    SECTION("MAX_PARTY_SIZE is 5") {
        REQUIRE(MAX_PARTY_SIZE == 5);
    }
    
    SECTION("PARTY_XP_SHARE_RANGE") {
        REQUIRE(PARTY_XP_SHARE_RANGE > 0);
    }
}

// ============================================================================
// Guild System Tests
// ============================================================================

TEST_CASE("GuildRank enum values", "[guild]") {
    REQUIRE(static_cast<uint8_t>(GuildRank::None) == 0);
    REQUIRE(static_cast<uint8_t>(GuildRank::Member) == 1);
    REQUIRE(static_cast<uint8_t>(GuildRank::Officer) == 2);
    REQUIRE(static_cast<uint8_t>(GuildRank::Leader) == 3);
}

TEST_CASE("GuildComponent initialization", "[guild]") {
    GuildComponent guild;
    REQUIRE(guild.guildId == 0);
    REQUIRE(guild.rank == GuildRank::None);
    REQUIRE(guild.joinTimeMs == 0);
}

TEST_CASE("GuildComponent - Join guild", "[guild]") {
    GuildComponent guild;
    guild.guildId = 5001;
    guild.rank = GuildRank::Member;
    guild.joinTimeMs = 1000;
    
    REQUIRE(guild.guildId == 5001);
    REQUIRE(guild.rank == GuildRank::Member);
    REQUIRE(guild.joinTimeMs == 1000);
}

TEST_CASE("GuildComponent - Guild leader", "[guild]") {
    GuildComponent guild;
    guild.guildId = 5001;
    guild.rank = GuildRank::Leader;
    
    REQUIRE(guild.guildId == 5001);
    REQUIRE(guild.rank == GuildRank::Leader);
}

TEST_CASE("GuildComponent - Officer", "[guild]") {
    GuildComponent guild;
    guild.guildId = 5001;
    guild.rank = GuildRank::Officer;
    
    REQUIRE(guild.guildId == 5001);
    REQUIRE(guild.rank == GuildRank::Officer);
}

TEST_CASE("GuildComponent - Leave guild", "[guild]") {
    GuildComponent guild;
    guild.guildId = 5001;
    guild.rank = GuildRank::Member;
    
    guild.guildId = 0;
    guild.rank = GuildRank::None;
    
    REQUIRE(guild.guildId == 0);
    REQUIRE(guild.rank == GuildRank::None);
}

TEST_CASE("GuildConstants", "[guild]") {
    SECTION("MAX_GUILD_SIZE is 100") {
        REQUIRE(MAX_GUILD_SIZE == 100);
    }
    
    SECTION("GUILD_NAME_MAX") {
        REQUIRE(GUILD_NAME_MAX == 32);
    }
}

// ============================================================================
// Trade System Tests
// ============================================================================

TEST_CASE("TradeState enum values", "[trade]") {
    REQUIRE(static_cast<uint8_t>(TradeState::None) == 0);
    REQUIRE(static_cast<uint8_t>(TradeState::Pending) == 1);
    REQUIRE(static_cast<uint8_t>(TradeState::Active) == 2);
    REQUIRE(static_cast<uint8_t>(TradeState::Locked) == 3);
    REQUIRE(static_cast<uint8_t>(TradeState::BothLocked) == 4);
    REQUIRE(static_cast<uint8_t>(TradeState::Confirmed) == 5);
}

TEST_CASE("TradeSlot initialization", "[trade]") {
    TradeSlot slot;
    REQUIRE(slot.itemId == 0);
    REQUIRE(slot.quantity == 0);
}

TEST_CASE("TradeSlot - isEmpty", "[trade]") {
    SECTION("Empty when itemId is 0") {
        TradeSlot slot;
        slot.itemId = 0;
        slot.quantity = 5;
        REQUIRE(slot.isEmpty() == true);
    }
    
    SECTION("Empty when quantity is 0") {
        TradeSlot slot;
        slot.itemId = 1001;
        slot.quantity = 0;
        REQUIRE(slot.isEmpty() == true);
    }
    
    SECTION("Not empty when both set") {
        TradeSlot slot;
        slot.itemId = 1001;
        slot.quantity = 5;
        REQUIRE(slot.isEmpty() == false);
    }
}

TEST_CASE("TradeComponent initialization", "[trade]") {
    TradeComponent trade;
    REQUIRE(trade.state == TradeState::None);
    REQUIRE(trade.tradePartner == entt::null);
    REQUIRE(trade.offeredGold == 0.0f);
    REQUIRE(trade.locked == false);
    REQUIRE(trade.confirmed == false);
}

TEST_CASE("TradeComponent - offeredItemCount", "[trade]") {
    TradeComponent trade;
    trade.offeredItems[0].itemId = 1001;
    trade.offeredItems[0].quantity = 1;
    trade.offeredItems[1].itemId = 1002;
    trade.offeredItems[1].quantity = 2;
    trade.offeredItems[2].itemId = 0;
    
    REQUIRE(trade.offeredItemCount() == 2);
}

TEST_CASE("TradeComponent - findEmptySlot", "[trade]") {
    TradeComponent trade;
    trade.offeredItems[0].itemId = 1001;
    trade.offeredItems[0].quantity = 1;
    
    REQUIRE(trade.findEmptySlot() == 1);
}

TEST_CASE("TradeComponent - findEmptySlot when full", "[trade]") {
    TradeComponent trade;
    for (uint32_t i = 0; i < MAX_TRADE_SLOTS; ++i) {
        trade.offeredItems[i].itemId = 1000 + i;
        trade.offeredItems[i].quantity = 1;
    }
    
    REQUIRE(trade.findEmptySlot() == -1);
}

TEST_CASE("TradeComponent - reset", "[trade]") {
    TradeComponent trade;
    trade.state = TradeState::Active;
    trade.tradePartner = static_cast<entt::entity>(123);
    trade.offeredItems[0].itemId = 1001;
    trade.offeredItems[0].quantity = 5;
    trade.offeredGold = 100.0f;
    trade.locked = true;
    trade.confirmed = true;
    
    trade.reset();
    
    REQUIRE(trade.state == TradeState::None);
    REQUIRE(trade.tradePartner == entt::null);
    REQUIRE(trade.offeredItems[0].isEmpty() == true);
    REQUIRE(trade.offeredGold == 0.0f);
    REQUIRE(trade.locked == false);
    REQUIRE(trade.confirmed == false);
}

TEST_CASE("TradeConstants", "[trade]") {
    SECTION("MAX_TRADE_SLOTS is 8") {
        REQUIRE(MAX_TRADE_SLOTS == 8);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("Party - Full party workflow", "[party][integration]") {
    PartyComponent leader;
    leader.partyId = 1001;
    leader.role = PartyRole::Leader;
    leader.shareXP = true;
    
    REQUIRE(leader.partyId == 1001);
    REQUIRE(leader.role == PartyRole::Leader);
    
    // Member joins
    PartyComponent member;
    member.partyId = 1001;
    member.role = PartyRole::Member;
    member.shareXP = true;
    
    REQUIRE(member.partyId == 1001);
    REQUIRE(member.role == PartyRole::Member);
}

TEST_CASE("Guild - Full guild workflow", "[guild][integration]") {
    GuildComponent leader;
    leader.guildId = 5001;
    leader.rank = GuildRank::Leader;
    leader.joinTimeMs = 1000;
    
    // Member joins
    GuildComponent member;
    member.guildId = 5001;
    member.rank = GuildRank::Member;
    member.joinTimeMs = 2000;
    
    REQUIRE(leader.guildId == member.guildId);
    REQUIRE(leader.rank != member.rank);
}

TEST_CASE("Trade - Full trade workflow", "[trade][integration]") {
    TradeComponent playerA;
    TradeComponent playerB;
    
    // Player A initiates trade
    playerA.state = TradeState::Pending;
    playerA.tradePartner = static_cast<entt::entity>(1);
    
    REQUIRE(playerA.state == TradeState::Pending);
    
    // Player B accepts
    playerB.state = TradeState::Active;
    playerB.tradePartner = static_cast<entt::entity>(0);
    
    REQUIRE(playerB.state == TradeState::Active);
    
    // Player A adds items
    playerA.offeredItems[0].itemId = 1001;
    playerA.offeredItems[0].quantity = 1;
    playerA.offeredGold = 50.0f;
    
    REQUIRE(playerA.offeredItemCount() == 1);
    REQUIRE(playerA.offeredGold == 50.0f);
    
    // Player A locks
    playerA.locked = true;
    playerA.state = TradeState::Locked;
    
    REQUIRE(playerA.locked == true);
    REQUIRE(playerA.state == TradeState::Locked);
}

// ============================================================================
// Permission Tests
// ============================================================================

TEST_CASE("Party - Permission check (leader vs member)", "[party]") {
    PartyComponent leader;
    leader.role = PartyRole::Leader;
    
    PartyComponent member;
    member.role = PartyRole::Member;
    
    // Only leader should be able to kick
    bool leaderCanKick = (leader.role == PartyRole::Leader);
    bool memberCanKick = (member.role == PartyRole::Leader);
    
    REQUIRE(leaderCanKick == true);
    REQUIRE(memberCanKick == false);
}

TEST_CASE("Guild - Permission ranks", "[guild]") {
    GuildComponent leader;
    leader.rank = GuildRank::Leader;
    
    GuildComponent officer;
    officer.rank = GuildRank::Officer;
    
    GuildComponent member;
    member.rank = GuildRank::Member;
    
    // Leaders can do everything
    bool leaderCanInvite = (leader.rank == GuildRank::Leader || leader.rank == GuildRank::Officer);
    bool memberCanInvite = (member.rank == GuildRank::Leader || member.rank == GuildRank::Officer);
    
    REQUIRE(leaderCanInvite == true);
    REQUIRE(memberCanInvite == false);
}

// ============================================================================
// Trade State Machine Tests
// ============================================================================

TEST_CASE("Trade - State transitions", "[trade]") {
    TradeComponent trade;
    
    // None -> Pending (request)
    trade.state = TradeState::Pending;
    REQUIRE(trade.state == TradeState::Pending);
    
    // Pending -> Active (accept)
    trade.state = TradeState::Active;
    REQUIRE(trade.state == TradeState::Active);
    
    // Active -> Locked (lock)
    trade.locked = true;
    trade.state = TradeState::Locked;
    REQUIRE(trade.state == TradeState::Locked);
    REQUIRE(trade.locked == true);
    
    // Locked -> Confirmed (both lock)
    trade.confirmed = true;
    trade.state = TradeState::Confirmed;
    REQUIRE(trade.state == TradeState::Confirmed);
}

TEST_CASE("Trade - Cancel trade", "[trade]") {
    TradeComponent trade;
    trade.state = TradeState::Active;
    trade.offeredItems[0].itemId = 1001;
    trade.offeredGold = 100.0f;
    
    trade.reset();
    
    REQUIRE(trade.state == TradeState::None);
    REQUIRE(trade.offeredItems[0].isEmpty() == true);
    REQUIRE(trade.offeredGold == 0.0f);
}