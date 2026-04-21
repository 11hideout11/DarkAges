#include "combat/PartySystem.hpp"
#include <algorithm>
#include <iostream>

namespace DarkAges {

// ============================================================================
// Party Management
// ============================================================================

uint32_t PartySystem::createParty(EntityID leader, uint32_t currentTimeMs) {
    // Check if leader is already in a party
    uint32_t existingParty = getPartyId(leader);
    if (existingParty != 0) {
        return 0;
    }

    uint32_t partyId = nextPartyId_++;
    PartyData& party = parties_[partyId];
    party.partyId = partyId;
    party.leader = leader;
    party.members.push_back(leader);
    party.createdAt = currentTimeMs;

    std::cout << "[PARTY] Party " << partyId << " created by entity "
              << static_cast<uint32_t>(leader) << std::endl;

    return partyId;
}

bool PartySystem::disbandParty(uint32_t partyId, EntityID requester) {
    auto it = parties_.find(partyId);
    if (it == parties_.end()) return false;

    // Only the leader can disband
    if (it->second.leader != requester) return false;

    std::cout << "[PARTY] Party " << partyId << " disbanded" << std::endl;

    parties_.erase(it);
    return true;
}

// ============================================================================
// Membership
// ============================================================================

bool PartySystem::invitePlayer(EntityID inviter, EntityID target) {
    uint32_t partyId = getPartyId(inviter);
    if (partyId == 0) return false;  // Inviter not in a party

    // Must be leader to invite
    if (!isLeader(inviter)) return false;

    // Check party size
    if (isPartyFull(partyId)) return false;

    // Check if target is already in a party
    if (isInParty(target)) return false;

    // Add pending invite
    PendingInvite invite;
    invite.partyId = partyId;
    invite.expireTimeMs = 0;  // Will be set by caller
    pendingInvites_[static_cast<uint32_t>(target)].push_back(invite);

    return true;
}

bool PartySystem::acceptInvite(EntityID player, uint32_t partyId, uint32_t currentTimeMs) {
    // Check if player has a pending invite
    if (!hasPendingInvite(player, partyId)) return false;

    // Check if player is already in a party
    if (isInParty(player)) return false;

    auto it = parties_.find(partyId);
    if (it == parties_.end()) return false;

    // Check party size
    if (isPartyFull(partyId)) return false;

    // Add to party
    it->second.members.push_back(player);

    // Clear the invite
    declineInvite(player, partyId);

    std::cout << "[PARTY] Entity " << static_cast<uint32_t>(player)
              << " joined party " << partyId << std::endl;

    return true;
}

void PartySystem::declineInvite(EntityID player, uint32_t partyId) {
    auto it = pendingInvites_.find(static_cast<uint32_t>(player));
    if (it == pendingInvites_.end()) return;

    auto& invites = it->second;
    invites.erase(
        std::remove_if(invites.begin(), invites.end(),
                       [partyId](const PendingInvite& i) { return i.partyId == partyId; }),
        invites.end()
    );

    if (invites.empty()) {
        pendingInvites_.erase(it);
    }
}

bool PartySystem::removeFromParty(EntityID player, EntityID requester) {
    uint32_t partyId = getPartyId(player);
    if (partyId == 0) return false;

    auto it = parties_.find(partyId);
    if (it == parties_.end()) return false;

    // Can remove self (leave) or leader can kick others
    if (player != requester && it->second.leader != requester) {
        return false;
    }

    // If leader is leaving, disband or transfer leadership
    if (player == it->second.leader) {
        if (it->second.members.size() <= 1) {
            // Last member — disband
            parties_.erase(it);
            std::cout << "[PARTY] Party " << partyId << " disbanded (leader left)" << std::endl;
            return true;
        }
        // Transfer leadership to next member
        EntityID newLeader = entt::null;
        for (EntityID member : it->second.members) {
            if (member != player) {
                newLeader = member;
                break;
            }
        }
        it->second.leader = newLeader;
    }

    removeMemberFromParty(partyId, player);

    std::cout << "[PARTY] Entity " << static_cast<uint32_t>(player)
              << " left party " << partyId << std::endl;

    // If party is now empty, clean up
    if (it->second.members.empty()) {
        parties_.erase(it);
    }

    return true;
}

bool PartySystem::leaveParty(EntityID player) {
    return removeFromParty(player, player);
}

// ============================================================================
// Queries
// ============================================================================

uint32_t PartySystem::getPartyId(EntityID player) const {
    for (const auto& [id, party] : parties_) {
        for (EntityID member : party.members) {
            if (member == player) return id;
        }
    }
    return 0;
}

bool PartySystem::isInParty(EntityID player) const {
    return getPartyId(player) != 0;
}

bool PartySystem::isLeader(EntityID player) const {
    uint32_t partyId = getPartyId(player);
    if (partyId == 0) return false;

    auto it = parties_.find(partyId);
    if (it == parties_.end()) return false;

    return it->second.leader == player;
}

std::vector<EntityID> PartySystem::getPartyMembers(uint32_t partyId) const {
    auto it = parties_.find(partyId);
    if (it == parties_.end()) return {};
    return it->second.members;
}

EntityID PartySystem::getPartyLeader(uint32_t partyId) const {
    auto it = parties_.find(partyId);
    if (it == parties_.end()) return entt::null;
    return it->second.leader;
}

uint32_t PartySystem::getPartySize(uint32_t partyId) const {
    auto it = parties_.find(partyId);
    if (it == parties_.end()) return 0;
    return static_cast<uint32_t>(it->second.members.size());
}

bool PartySystem::isPartyFull(uint32_t partyId) const {
    return getPartySize(partyId) >= MAX_PARTY_SIZE;
}

bool PartySystem::hasPendingInvite(EntityID player, uint32_t partyId) const {
    auto it = pendingInvites_.find(static_cast<uint32_t>(player));
    if (it == pendingInvites_.end()) return false;

    for (const auto& invite : it->second) {
        if (invite.partyId == partyId) return true;
    }
    return false;
}

// ============================================================================
// XP Sharing
// ============================================================================

std::vector<EntityID> PartySystem::getXPParticipants(Registry& registry,
                                                      EntityID killer,
                                                      uint32_t partyId) const {
    std::vector<EntityID> participants;
    auto members = getPartyMembers(partyId);
    if (members.empty()) return participants;

    // Get killer position
    const Position* killerPos = registry.try_get<Position>(killer);
    if (!killerPos) {
        participants.push_back(killer);
        return participants;
    }

    float rangeSq = PARTY_XP_SHARE_RANGE * PARTY_XP_SHARE_RANGE;

    for (EntityID member : members) {
        // Only include members with shareXP enabled
        const PartyComponent* pc = registry.try_get<PartyComponent>(member);
        if (pc && !pc->shareXP) continue;

        // Check distance
        const Position* memberPos = registry.try_get<Position>(member);
        if (!memberPos) continue;

        float dx = static_cast<float>(killerPos->x - memberPos->x) * Constants::FIXED_TO_FLOAT;
        float dz = static_cast<float>(killerPos->z - memberPos->z) * Constants::FIXED_TO_FLOAT;
        float distSq = dx * dx + dz * dz;

        if (distSq <= rangeSq) {
            participants.push_back(member);
        }
    }

    // Always include the killer even if they're out of range
    bool killerIncluded = false;
    for (EntityID p : participants) {
        if (p == killer) { killerIncluded = true; break; }
    }
    if (!killerIncluded) {
        participants.push_back(killer);
    }

    return participants;
}

// ============================================================================
// Component Management
// ============================================================================

void PartySystem::ensureComponent(Registry& registry, EntityID entity) {
    if (!registry.all_of<PartyComponent>(entity)) {
        registry.emplace<PartyComponent>(entity);
    }
}

void PartySystem::onPlayerDisconnect(Registry& registry, EntityID player) {
    removeFromParty(player, player);
}

// ============================================================================
// Private Helpers
// ============================================================================

void PartySystem::removeMemberFromParty(uint32_t partyId, EntityID player) {
    auto it = parties_.find(partyId);
    if (it == parties_.end()) return;

    auto& members = it->second.members;
    members.erase(
        std::remove(members.begin(), members.end(), player),
        members.end()
    );
}

} // namespace DarkAges
