// [COMBAT_AGENT] Comprehensive unit tests for PartySystem

#include <catch2/catch_test_macros.hpp>
#include "combat/PartySystem.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>

using namespace DarkAges;

namespace {

// Helper: create an entity with minimum required components
EntityID createPlayer(Registry& registry) {
    EntityID e = registry.create();
    registry.emplace<Position>(e);
    // PartySystem doesn't require stats, just Position for XP sharing range checks
    return e;
}

// Helper: set two players at a specific distance apart (x/z plane)
void setPositions(Registry& registry, EntityID a, EntityID b, float dx, float dz) {
    auto& posA = registry.get<Position>(a);
    auto& posB = registry.get<Position>(b);
    // Fixed-point conversion: Fixed = int32_t scaled by FIXED_PRECISION (1000)
    posA.x = 0; posA.z = 0;
    posB.x = static_cast<Constants::Fixed>(dx * Constants::FLOAT_TO_FIXED);
    posB.z = static_cast<Constants::Fixed>(dz * Constants::FLOAT_TO_FIXED);
}

} // namespace

TEST_CASE("PartySystem initialization", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID player = createPlayer(registry);
    
    REQUIRE_FALSE(partySys.isInParty(player));
    REQUIRE_FALSE(partySys.isLeader(player));
    REQUIRE(partySys.getPartyId(player) == 0);
    REQUIRE(partySys.getPartySize(partySys.getPartyId(player)) == 0);
}

TEST_CASE("PartySystem - creating and disbanding parties", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID member = createPlayer(registry);

    // Create a party
    uint32_t partyId = partySys.createParty(leader, 1000);
    REQUIRE(partyId != 0);
    REQUIRE(partySys.isInParty(leader));
    REQUIRE(partySys.isLeader(leader));
    REQUIRE(partySys.getPartyId(leader) == partyId);
    REQUIRE(partySys.getPartySize(partyId) == 1);
    REQUIRE(partySys.getPartyLeader(partyId) == leader);

    // Leader cannot create another party
    REQUIRE(partySys.createParty(leader, 1000) == 0);

    // Member joins
    REQUIRE(partySys.invitePlayer(leader, member));
    REQUIRE(partySys.acceptInvite(member, partyId, 1000));
    REQUIRE(partySys.isInParty(member));
    REQUIRE(partySys.getPartySize(partyId) == 2);
    REQUIRE(partySys.getPartyMembers(partyId).size() == 2);

    // Disband as leader
    REQUIRE(partySys.disbandParty(partyId, leader));
    REQUIRE_FALSE(partySys.isInParty(leader));
    REQUIRE_FALSE(partySys.isInParty(member));
    REQUIRE(partySys.getPartyId(leader) == 0);
    REQUIRE(partySys.getPartyId(member) == 0);
}

TEST_CASE("PartySystem - non-leader cannot invite", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID member = createPlayer(registry);
    EntityID outsider = createPlayer(registry);

    uint32_t partyId = partySys.createParty(leader, 1000);
    REQUIRE(partySys.invitePlayer(leader, member));
    REQUIRE(partySys.acceptInvite(member, partyId, 1000));

    // Member tries to invite outsider
    REQUIRE_FALSE(partySys.invitePlayer(member, outsider));
    REQUIRE(partySys.getPartyId(outsider) == 0);
}

TEST_CASE("PartySystem - party full limits", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID members[4]; // MAX_PARTY_SIZE = 5, so 1 + 4 = 5
    for (int i = 0; i < 4; ++i) {
        members[i] = createPlayer(registry);
    }

    uint32_t partyId = partySys.createParty(leader, 1000);
    REQUIRE(partyId != 0);

    for (int i = 0; i < 4; ++i) {
        REQUIRE(partySys.invitePlayer(leader, members[i]));
        REQUIRE(partySys.acceptInvite(members[i], partyId, 1000 + i * 100));
    }

    REQUIRE(partySys.getPartySize(partyId) == 5);
    REQUIRE(partySys.isPartyFull(partyId));

    // 6th member cannot join
    EntityID extra = createPlayer(registry);
    REQUIRE_FALSE(partySys.invitePlayer(leader, extra));
    REQUIRE_FALSE(partySys.isInParty(extra));
}

TEST_CASE("PartySystem - invitation flow and expiry", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID target = createPlayer(registry);

    uint32_t partyId = partySys.createParty(leader, 1000);
    
    // Send invite
    REQUIRE(partySys.invitePlayer(leader, target));
    REQUIRE(partySys.hasPendingInvite(target, partyId));

    // Decline
    partySys.declineInvite(target, partyId);
    REQUIRE_FALSE(partySys.hasPendingInvite(target, partyId));

    // Another invite, then expire it
    REQUIRE(partySys.invitePlayer(leader, target));
    REQUIRE(partySys.hasPendingInvite(target, partyId));
    
    // Manually call decline (expiry not automatic in current impl — handled by system tick later)
    partySys.declineInvite(target, partyId);
    REQUIRE_FALSE(partySys.hasPendingInvite(target, partyId));
}

TEST_CASE("PartySystem - membership removal (kick and leave)", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID member = createPlayer(registry);

    uint32_t partyId = partySys.createParty(leader, 1000);
    REQUIRE(partySys.invitePlayer(leader, member));
    REQUIRE(partySys.acceptInvite(member, partyId, 1000));

    // Leader kicks member
    REQUIRE(partySys.removeFromParty(member, leader));  // leader kicks member
    REQUIRE_FALSE(partySys.isInParty(member));
    REQUIRE(partySys.getPartySize(partyId) == 1);

    // Member leaves (self)
    EntityID solo = createPlayer(registry);
    uint32_t soloParty = partySys.createParty(solo, 1000);
    REQUIRE(partySys.leaveParty(solo));
    REQUIRE_FALSE(partySys.isInParty(solo));
    REQUIRE(partySys.getPartySize(soloParty) == 0);
}

TEST_CASE("PartySystem - XP sharing range logic", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader = createPlayer(registry);
    EntityID nearMember = createPlayer(registry);
    EntityID farMember = createPlayer(registry);

    uint32_t partyId = partySys.createParty(leader, 1000);
    REQUIRE(partySys.invitePlayer(leader, nearMember));
    REQUIRE(partySys.invitePlayer(leader, farMember));
    REQUIRE(partySys.acceptInvite(nearMember, partyId, 1000));
    REQUIRE(partySys.acceptInvite(farMember, partyId, 1000));

    // Place leader at origin
    setPositions(registry, leader, leader, 0, 0);

    // Test getXPParticipants
    // nearMember at (10, 0) — within 100m range
    setPositions(registry, nearMember, nearMember, 10, 0);
    // farMember at (200, 0) — beyond 100m range
    setPositions(registry, farMember, farMember, 200, 0);

    auto participants = partySys.getXPParticipants(registry, leader, partyId);
    // getXPParticipants includes the killer (leader) automatically, plus any members within range
    REQUIRE(participants.size() == 2);  // leader + nearMember
    REQUIRE(std::find(participants.begin(), participants.end(), leader) != participants.end());
    REQUIRE(std::find(participants.begin(), participants.end(), nearMember) != participants.end());
    REQUIRE(std::find(participants.begin(), participants.end(), farMember) == participants.end());
}

TEST_CASE("PartySystem - ensureComponent and cleanup on disconnect", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID player = createPlayer(registry);

    // Ensure component added
    REQUIRE_FALSE(registry.all_of<PartyComponent>(player));
    partySys.ensureComponent(registry, player);
    REQUIRE(registry.all_of<PartyComponent>(player));
    const PartyComponent& comp = registry.get<PartyComponent>(player);
    REQUIRE(comp.partyId == 0);
    REQUIRE(comp.role == PartyRole::None);

    // Join a party
    uint32_t partyId = partySys.createParty(player, 1000);
    REQUIRE(partySys.isInParty(player));

    // Disconnect cleanup
    partySys.onPlayerDisconnect(registry, player);
    REQUIRE_FALSE(partySys.isInParty(player));
    // Component remains (doesn't remove it)
    REQUIRE(registry.all_of<PartyComponent>(player));
}

TEST_CASE("PartySystem - multiple parties are independent", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID leader1 = createPlayer(registry);
    EntityID leader2 = createPlayer(registry);
    EntityID member1 = createPlayer(registry);
    EntityID member2 = createPlayer(registry);

    uint32_t p1 = partySys.createParty(leader1, 1000);
    uint32_t p2 = partySys.createParty(leader2, 1000);

    // leader1 invites member1 to p1
    REQUIRE(partySys.invitePlayer(leader1, member1));
    REQUIRE(partySys.acceptInvite(member1, p1, 1000));
    REQUIRE(partySys.isInParty(member1));
    REQUIRE(partySys.getPartyId(member1) == p1);
    REQUIRE(partySys.getPartySize(p1) == 2);

    // leader2 cannot invite member1 because they're already in another party
    REQUIRE_FALSE(partySys.invitePlayer(leader2, member1));

    // leader2 can invite member2 to p2
    REQUIRE(partySys.invitePlayer(leader2, member2));
    REQUIRE(partySys.acceptInvite(member2, p2, 1000));
    REQUIRE(partySys.getPartySize(p2) == 2);

    // Cross-party checks: member1 belongs only to p1; member2 only to p2
    REQUIRE_FALSE((partySys.isInParty(member1) && partySys.getPartyId(member1) != p1));
    REQUIRE_FALSE((partySys.isInParty(member2) && partySys.getPartyId(member2) != p2));
    REQUIRE(partySys.getPartyLeader(p1) == leader1);
    REQUIRE(partySys.getPartyLeader(p2) == leader2);
}

TEST_CASE("PartySystem - edge cases: invalid operations return false", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID player = createPlayer(registry);

    // Try to leave when not in party
    REQUIRE_FALSE(partySys.leaveParty(player));

    // Try to disband non-existent party
    REQUIRE_FALSE(partySys.disbandParty(9999, player));

    // Accept invite to non-existent party
    REQUIRE_FALSE(partySys.acceptInvite(player, 9999, 1000));

    // Remove from party where inviter is not leader
    EntityID leader = createPlayer(registry);
    EntityID member = createPlayer(registry);
    uint32_t pid = partySys.createParty(leader, 1000);
    partySys.invitePlayer(leader, member);
    partySys.acceptInvite(member, pid, 1000);

    // member tries to kick leader — should fail
    REQUIRE_FALSE(partySys.removeFromParty(leader, member));
}

TEST_CASE("PartySystem - nextPartyId increments correctly", "[partysystem]") {
    PartySystem partySys;
    Registry registry;

    EntityID a = createPlayer(registry);
    EntityID b = createPlayer(registry);

    uint32_t id1 = partySys.createParty(a, 1000);
    uint32_t id2 = partySys.createParty(b, 1000);

    REQUIRE(id2 == id1 + 1);

    // After many creations, keep increasing
    EntityID c = createPlayer(registry);
    uint32_t id3 = partySys.createParty(c, 1000);
    REQUIRE(id3 == id2 + 1);
}
