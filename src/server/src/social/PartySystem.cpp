/**
 * @file PartySystem.cpp
 * @brief Party system implementation
 */

#include "social/PartySystem.hpp"
#include <iostream>

namespace DarkAges::social {

// ============================================================================
// PartySystem Implementation
// ============================================================================

PartySystem::PartySystem() {
    std::cout << "[PartySystem] Initialized" << std::endl;
}

PartySystem::~PartySystem() {
    std::cout << "[PartySystem] Shutdown" << std::endl;
}

// ============================================================================
// Party Management
// ============================================================================

std::optional<uint64_t> PartySystem::CreateParty(uint64_t leader_id, const std::string& leader_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check not already in party
    if (player_party_.count(leader_id)) {
        return std::nullopt;
    }
    
    uint64_t party_id = next_party_id_.fetch_add(1);
    
    Party party;
    party.party_id = party_id;
    party.leader_id = leader_id;
    party.created_at = std::chrono::system_clock::now();
    
    PartyMember leader;
    leader.player_id = leader_id;
    leader.name = leader_name;
    leader.level = 1;
    leader.health_percent = 100.0f;
    leader.max_health = 100;
    leader.is_leader = true;
    
    party.members.push_back(leader);
    
    parties_[party_id] = party;
    player_party_[leader_id] = party_id;
    
    std::cout << "[PartySystem] Party created: " << party_id << std::endl;
    
    return party_id;
}

bool PartySystem::DisbandParty(uint64_t party_id, uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = parties_.find(party_id);
    if (it == parties_.end()) {
        return false;
    }
    
    Party& party = it->second;
    
    // Only leader can disband
    if (party.leader_id != player_id) {
        return false;
    }
    
    // Remove all members
    for (const auto& member : party.members) {
        player_party_.erase(member.player_id);
    }
    
    parties_.erase(party_id);
    
    std::cout << "[PartySystem] Party disbanded: " << party_id << std::endl;
    
    return true;
}

std::optional<Party> PartySystem::GetParty(uint64_t party_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = parties_.find(party_id);
    if (it == parties_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::optional<uint64_t> PartySystem::GetPlayerParty(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = player_party_.find(player_id);
    if (it == player_party_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

// ============================================================================
// Membership
// ============================================================================

bool PartySystem::InvitePlayer(uint64_t party_id, uint64_t inviter_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return false;
    }
    Party& party = pit->second;
    
    // Check inviter can invite
    if (!CanInvite(party, inviter_id)) {
        return false;
    }
    
    // Check party not full
    if (party.members.size() >= MAX_PARTY_SIZE) {
        return false;
    }
    
    // Check target not in party
    if (player_party_.count(target_id)) {
        return false;
    }
    
    // Add pending invitation
    pending_invitations_.push_back(party_id);
    
    std::cout << "[PartySystem] Invited " << target_id 
              << " to party " << party_id << std::endl;
    
    return true;
}

bool PartySystem::AcceptInvitation(uint64_t player_id, uint64_t party_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Remove from pending
    auto inv_it = std::find(pending_invitations_.begin(), pending_invitations_.end(), party_id);
    if (inv_it == pending_invitations_.end()) {
        return false;
    }
    pending_invitations_.erase(inv_it);
    
    // Add to party
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return false;
    }
    
    Party& party = pit->second;
    if (party.members.size() >= MAX_PARTY_SIZE) {
        return false;
    }
    
    PartyMember member;
    member.player_id = player_id;
    member.name = "Unknown";
    member.level = 1;
    member.health_percent = 100.0f;
    member.max_health = 100;
    member.is_leader = false;
    
    party.members.push_back(member);
    player_party_[player_id] = party_id;
    
    std::cout << "[PartySystem] Player " << player_id 
              << " joined party " << party_id << std::endl;
    
    return true;
}

void PartySystem::DeclineInvitation(uint64_t player_id, uint64_t party_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find(pending_invitations_.begin(), pending_invitations_.end(), party_id);
    if (it != pending_invitations_.end()) {
        pending_invitations_.erase(it);
    }
    
    std::cout << "[PartySystem] Player " << player_id 
              << " declined party " << party_id << std::endl;
}

bool PartySystem::KickMember(uint64_t party_id, uint64_t kicker_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return false;
    }
    Party& party = pit->second;
    
    // Check kicker can kick (leader)
    if (!CanKick(party, kicker_id)) {
        return false;
    }
    
    // Can't kick leader
    if (target_id == party.leader_id) {
        return false;
    }
    
    // Remove member
    for (auto it = party.members.begin(); it != party.members.end(); ++it) {
        if (it->player_id == target_id) {
            party.members.erase(it);
            player_party_.erase(target_id);
            
            std::cout << "[PartySystem] Kicked " << target_id 
                      << " from party " << party_id << std::endl;
            return true;
        }
    }
    
    return false;
}

bool PartySystem::LeaveParty(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = player_party_.find(player_id);
    if (pit == player_party_.end()) {
        return false;
    }
    
    uint64_t party_id = pit->second;
    auto part_it = parties_.find(party_id);
    if (part_it == parties_.end()) {
        player_party_.erase(player_id);
        return false;
    }
    
    Party& party = part_it->second;
    
    // Leader can't leave - must disband
    if (party.leader_id == player_id) {
        return false;
    }
    
    // Remove member
    for (auto it = party.members.begin(); it != party.members.end(); ++it) {
        if (it->player_id == player_id) {
            party.members.erase(it);
            player_party_.erase(player_id);
            
            std::cout << "[PartySystem] Player " << player_id 
                      << " left party " << party_id << std::endl;
            return true;
        }
    }
    
    return false;
}

bool PartySystem::TransferLeadership(uint64_t party_id, uint64_t current_leader, uint64_t new_leader_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return false;
    }
    
    Party& party = pit->second;
    
    // Only current leader can transfer
    if (party.leader_id != current_leader) {
        return false;
    }
    
    // Update leader
    party.leader_id = new_leader_id;
    
    // Update member flags
    for (auto& member : party.members) {
        member.is_leader = (member.player_id == new_leader_id);
    }
    
    std::cout << "[PartySystem] Leadership transferred in party " << party_id << std::endl;
    
    return true;
}

// ============================================================================
// Options
// ============================================================================

bool PartySystem::SetLootMode(uint64_t party_id, uint64_t setter_id, PartyLootMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return false;
    }
    
    Party& party = pit->second;
    
    // Only leader can set
    if (party.leader_id != setter_id) {
        return false;
    }
    
    party.loot_mode = mode;
    return true;
}

bool PartySystem::SetXPShare(uint64_t party_id, uint64_t setter_id, PartyXPShare mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return false;
    }
    
    Party& party = pit->second;
    
    if (party.leader_id != setter_id) {
        return false;
    }
    
    party.xp_share = mode;
    return true;
}

// ============================================================================
// Health Sharing
// ============================================================================

void PartySystem::UpdateMemberHealth(uint64_t party_id, uint64_t player_id, float health_percent, uint32_t max_health) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return;
    }
    
    Party& party = pit->second;
    
    for (auto& member : party.members) {
        if (member.player_id == player_id) {
            member.health_percent = health_percent;
            member.max_health = max_health;
            return;
        }
    }
}

std::vector<PartyMember> PartySystem::GetMembers(uint64_t party_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = parties_.find(party_id);
    if (pit == parties_.end()) {
        return {};
    }
    
    return pit->second.members;
}

// ============================================================================
// Queries
// ============================================================================

bool PartySystem::AreSameParty(uint64_t player1_id, uint64_t player2_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it1 = player_party_.find(player1_id);
    auto it2 = player_party_.find(player2_id);
    
    if (it1 == player_party_.end() || it2 == player_party_.end()) {
        return false;
    }
    
    return it1->second == it2->second;
}

uint32_t PartySystem::GetPartySize(uint64_t party_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = parties_.find(party_id);
    if (it == parties_.end()) {
        return 0;
    }
    
    return static_cast<uint32_t>(it->second.members.size());
}

// ============================================================================
// Helpers
// ============================================================================

bool PartySystem::CanInvite(const Party& party, uint64_t player_id) const {
    // Leader and officers can invite
    for (const auto& member : party.members) {
        if (member.player_id == player_id && member.is_leader) {
            return true;
        }
    }
    return false;
}

bool PartySystem::CanKick(const Party& party, uint64_t player_id) const {
    return party.leader_id == player_id;
}

} // namespace DarkAges::social