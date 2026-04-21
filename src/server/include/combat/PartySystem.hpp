#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

// Party System — manages player groups of up to MAX_PARTY_SIZE members.
// Handles party creation, invitations, membership, and XP sharing.
// Provides member lookup for ChatSystem party message routing.

namespace DarkAges {

class PartySystem {
public:
    PartySystem() = default;

    // --- Party Management ---

    // Create a new party with the given player as leader
    // Returns the party ID, or 0 if player is already in a party
    uint32_t createParty(EntityID leader, uint32_t currentTimeMs);

    // Disband a party (leader only)
    // Returns true if successful
    bool disbandParty(uint32_t partyId, EntityID requester);

    // --- Membership ---

    // Invite a player to a party
    // Returns true if invitation was sent
    bool invitePlayer(EntityID inviter, EntityID target);

    // Accept a pending invitation
    // Returns true if joined successfully
    bool acceptInvite(EntityID player, uint32_t partyId, uint32_t currentTimeMs);

    // Decline or ignore an invitation
    void declineInvite(EntityID player, uint32_t partyId);

    // Remove a player from their party (leader can kick, members can leave)
    // Returns true if successful
    bool removeFromParty(EntityID player, EntityID requester);

    // Leave current party (shorthand for removeFromParty with self)
    bool leaveParty(EntityID player);

    // --- Queries ---

    // Get the party ID for a player (0 if not in a party)
    uint32_t getPartyId(EntityID player) const;

    // Check if player is in a party
    bool isInParty(EntityID player) const;

    // Check if player is the party leader
    bool isLeader(EntityID player) const;

    // Get all members of a party
    std::vector<EntityID> getPartyMembers(uint32_t partyId) const;

    // Get the leader of a party
    EntityID getPartyLeader(uint32_t partyId) const;

    // Get party size
    uint32_t getPartySize(uint32_t partyId) const;

    // Check if a party is full
    bool isPartyFull(uint32_t partyId) const;

    // Check if player has a pending invite to a party
    bool hasPendingInvite(EntityID player, uint32_t partyId) const;

    // --- XP Sharing ---

    // Get nearby party members for XP sharing
    // Returns members within PARTY_XP_SHARE_RANGE of the killer
    std::vector<EntityID> getXPParticipants(Registry& registry,
                                             EntityID killer, uint32_t partyId) const;

    // --- Component Management ---

    // Add PartyComponent to entity if not present
    void ensureComponent(Registry& registry, EntityID entity);

    // Clean up when a player disconnects
    void onPlayerDisconnect(Registry& registry, EntityID player);

private:
    // Party data (not an ECS component — managed centrally)
    struct PartyData {
        uint32_t partyId{0};
        EntityID leader{entt::null};
        std::vector<EntityID> members;
        uint32_t createdAt{0};
    };

    // Pending invitation
    struct PendingInvite {
        uint32_t partyId{0};
        uint32_t expireTimeMs{0};
    };

    uint32_t nextPartyId_{1};
    std::unordered_map<uint32_t, PartyData> parties_;
    std::unordered_map<uint32_t, std::vector<PendingInvite>> pendingInvites_;  // player -> invites

    static constexpr uint32_t INVITE_EXPIRE_MS = 30000;  // 30 seconds

    // Remove player from a party's member list
    void removeMemberFromParty(uint32_t partyId, EntityID player);
};

} // namespace DarkAges
