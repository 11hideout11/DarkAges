/**
 * @file GuildSystem.cpp
 * @brief Guild system implementation
 */

#include "social/GuildSystem.hpp"
#include <iostream>
#include <fstream>

namespace DarkAges::social {

// ============================================================================
// GuildSystem Implementation
// ============================================================================

GuildSystem::GuildSystem() {
    // Initialize default rank names
    std::cout << "[GuildSystem] Initialized" << std::endl;
}

GuildSystem::~GuildSystem() {
    std::cout << "[GuildSystem] Shutdown" << std::endl;
}

// ============================================================================
// Guild Management
// ============================================================================

std::optional<uint64_t> GuildSystem::CreateGuild(
    const std::string& name,
    uint64_t leader_id,
    const std::string& leader_name
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate name
    if (!ValidateName(name)) {
        std::cout << "[GuildSystem] Invalid guild name: " << name << std::endl;
        return std::nullopt;
    }
    
    // Check name is unique (BUG-1 fix)
    for (const auto& [id, g] : guilds_) {
        if (g.name == name) {
            std::cout << "[GuildSystem] Guild name already taken: " << name << std::endl;
            return std::nullopt;
        }
    }
    
    // Check player not already in guild
    if (player_guild_.count(leader_id)) {
        std::cout << "[GuildSystem] Player already in guild: " << leader_id << std::endl;
        return std::nullopt;
    }
    
    // Create guild
    uint64_t guild_id = next_guild_id_.fetch_add(1);
    
    Guild guild;
    guild.guild_id = guild_id;
    guild.name = name;
    guild.leader_id = leader_id;
    guild.leader_name = leader_name;
    guild.created_at = std::chrono::system_clock::now();
    guild.last_activity = std::chrono::system_clock::now();
    
    // Set default rank names
    guild.rank_names[0] = "Leader";
    guild.rank_names[1] = "Officer";
    guild.rank_names[2] = "Member";
    guild.rank_names[3] = "Recruit";
    guild.rank_names[4] = "Recruit";
    
    // Add founder as leader
    AddMember(guild, leader_id, leader_name, 1);
    guild.members[leader_id].rank = GuildRank::Leader;
    guild.member_count = 1;
    
    // Store guild and index
    guilds_[guild_id] = guild;
    player_guild_[leader_id] = guild_id;
    
    std::cout << "[GuildSystem] Guild created: " << name 
              << " (id: " << guild_id << ")" << std::endl;
    
    return guild_id;
}

bool GuildSystem::DisbandGuild(uint64_t guild_id, uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = guilds_.find(guild_id);
    if (it == guilds_.end()) {
        return false;
    }
    
    Guild& guild = it->second;
    
    // Only leader can disband
    if (guild.leader_id != player_id) {
        std::cout << "[GuildSystem] Only leader can disband: " << player_id << std::endl;
        return false;
    }
    
    // Remove all members from index
    for (const auto& [pid, member] : guild.members) {
        player_guild_.erase(pid);
    }
    
    // Remove guild
    guilds_.erase(guild_id);
    
    std::cout << "[GuildSystem] Guild disbanded: " << guild_id << std::endl;
    
    return true;
}

std::optional<Guild> GuildSystem::GetGuild(uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = guilds_.find(guild_id);
    if (it == guilds_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

std::optional<uint64_t> GuildSystem::GetPlayerGuild(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = player_guild_.find(player_id);
    if (it == player_guild_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

// ============================================================================
// Membership
// ============================================================================

bool GuildSystem::InvitePlayer(uint64_t guild_id, uint64_t inviter_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check guild exists
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    Guild& guild = git->second;
    
    // Check inviter can invite
    auto mit = guild.members.find(inviter_id);
    if (mit == guild.members.end() || !CanInvite(guild, inviter_id)) {
        return false;
    }
    
    // Check target not already in guild
    if (player_guild_.count(target_id)) {
        return false;
    }
    
    // Check guild not full
    if (guild.member_count >= MAX_GUILD_MEMBERS) {
        return false;
    }
    
    // Create invitation
    GuildInvitation inv;
    inv.guild_id = guild_id;
    inv.player_id = target_id;
    inv.invited_at = std::chrono::system_clock::now();
    invitations_.push_back(inv);
    
    std::cout << "[GuildSystem] Invited player " << target_id 
              << " to guild " << guild_id << std::endl;
    
    return true;
}

bool GuildSystem::AcceptInvitation(uint64_t player_id, uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // BUG-2 fix: Check player not already in guild
    if (player_guild_.count(player_id)) {
        std::cout << "[GuildSystem] Player already in guild: " << player_id << std::endl;
        return false;
    }
    
    // Find invitation
    for (auto& inv : invitations_) {
        if (inv.guild_id == guild_id && inv.player_id == player_id) {
            if (inv.declined) {
                return false;
            }
            inv.accepted = true;
            
            // Add to guild
            auto git = guilds_.find(guild_id);
            if (git != guilds_.end()) {
                AddMember(git->second, player_id, "Unknown", 1);
                player_guild_[player_id] = guild_id;
                git->second.member_count++;
                git->second.last_activity = std::chrono::system_clock::now();
                
                std::cout << "[GuildSystem] Player " << player_id 
                          << " joined guild " << guild_id << std::endl;
            }
            
            return true;
        }
    }
    
    return false;
}

void GuildSystem::DeclineInvitation(uint64_t player_id, uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& inv : invitations_) {
        if (inv.guild_id == guild_id && inv.player_id == player_id) {
            inv.declined = true;
            break;
        }
    }
}

bool GuildSystem::KickMember(uint64_t guild_id, uint64_t kicker_id, uint64_t target_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    Guild& guild = git->second;
    
    // Check kicker can manage
    auto mit = guild.members.find(kicker_id);
    if (mit == guild.members.end() || !CanManageMember(guild, kicker_id)) {
        return false;
    }
    
    // Can't kick leader
    auto tit = guild.members.find(target_id);
    if (tit == guild.members.end() || tit->second.rank == GuildRank::Leader) {
        return false;
    }
    
    RemoveMember(guild, target_id);
    guild.member_count--;
    player_guild_.erase(target_id);
    
    std::cout << "[GuildSystem] Kicked " << target_id 
              << " from guild " << guild_id << std::endl;
    
    return true;
}

bool GuildSystem::LeaveGuild(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = player_guild_.find(player_id);
    if (pit == player_guild_.end()) {
        return false;
    }
    
    uint64_t guild_id = pit->second;
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        player_guild_.erase(player_id);
        return false;
    }
    
    Guild& guild = git->second;
    
    // Leader can't leave unless disbanding
    if (guild.leader_id == player_id) {
        std::cout << "[GuildSystem] Leader must disband or transfer" << std::endl;
        return false;
    }
    
    RemoveMember(guild, player_id);
    guild.member_count--;
    player_guild_.erase(player_id);
    
    std::cout << "[GuildSystem] Player " << player_id 
              << " left guild " << guild_id << std::endl;
    
    return true;
}

bool GuildSystem::SetMemberRank(
    uint64_t guild_id,
    uint64_t promoter_id,
    uint64_t target_id,
    GuildRank new_rank
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    Guild& guild = git->second;
    
    // Check promoter can manage
    auto pit = guild.members.find(promoter_id);
    if (pit == guild.members.end() || !CanManageMember(guild, promoter_id)) {
        return false;
    }
    
    // Set rank
    auto tit = guild.members.find(target_id);
    if (tit == guild.members.end()) {
        return false;
    }
    
    tit->second.rank = new_rank;
    guild.last_activity = std::chrono::system_clock::now();
    
    return true;
}

// ============================================================================
// Ranks
// ============================================================================

bool GuildSystem::SetRankName(
    uint64_t guild_id,
    uint64_t setter_id,
    uint8_t rank_index,
    const std::string& name
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (rank_index >= MAX_GUILD_RANKS) {
        return false;
    }
    
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    Guild& guild = git->second;
    
    // Check setter can manage
    auto sit = guild.members.find(setter_id);
    if (sit == guild.members.end() || !CanManageMember(guild, setter_id)) {
        return false;
    }
    
    guild.rank_names[rank_index] = name;
    return true;
}

// ============================================================================
// Chat
// ============================================================================

bool GuildSystem::SendChat(
    uint64_t guild_id,
    uint64_t sender_id,
    const std::string& sender_name,
    const std::string& message
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    
    Guild& guild = git->second;
    
    // Check sender in guild
    if (!guild.members.count(sender_id)) {
        return false;
    }
    
    // Update last activity
    guild.last_activity = std::chrono::system_clock::now();
    
    // In production, broadcast to all members
    // For now, just log
    std::cout << "[Guild][" << guild.name << "] " << sender_name 
              << ": " << message << std::endl;
    
    return true;
}

// ============================================================================
// Bank
// ============================================================================

bool GuildSystem::DepositGold(uint64_t guild_id, uint64_t player_id, uint64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    
    // Check player in guild
    if (!git->second.members.count(player_id)) {
        return false;
    }
    
    git->second.bank_gold += amount;
    return true;
}

bool GuildSystem::WithdrawGold(uint64_t guild_id, uint64_t player_id, uint64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto git = guilds_.find(guild_id);
    if (git == guilds_.end()) {
        return false;
    }
    Guild& guild = git->second;
    
    // Check player can withdraw (officer+)
    auto mit = guild.members.find(player_id);
    if (mit == guild.members.end() || mit->second.rank > GuildRank::Officer) {
        return false;
    }
    
    if (guild.bank_gold < amount) {
        return false;
    }
    
    guild.bank_gold -= amount;
    return true;
}

// ============================================================================
// Queries
// ============================================================================

std::vector<GuildMember> GuildSystem::GetMemberList(uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GuildMember> result;
    
    auto git = guilds_.find(guild_id);
    if (git != guilds_.end()) {
        for (const auto& [pid, member] : git->second.members) {
            result.push_back(member);
        }
    }
    
    return result;
}

bool GuildSystem::AreSameGuild(uint64_t player1_id, uint64_t player2_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it1 = player_guild_.find(player1_id);
    auto it2 = player_guild_.find(player2_id);
    
    if (it1 == player_guild_.end() || it2 == player_guild_.end()) {
        return false;
    }
    
    return it1->second == it2->second;
}

uint32_t GuildSystem::GetOnlineCount(uint64_t guild_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t count = 0;
    
    auto git = guilds_.find(guild_id);
    if (git != guilds_.end()) {
        for (const auto& [pid, member] : git->second.members) {
            if (member.online) count++;
        }
    }
    
    return count;
}

void GuildSystem::SetOnlineStatus(uint64_t player_id, bool online) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto pit = player_guild_.find(player_id);
    if (pit == player_guild_.end()) {
        return;
    }
    
    auto git = guilds_.find(pit->second);
    if (git != guilds_.end()) {
        auto mit = git->second.members.find(player_id);
        if (mit != git->second.members.end()) {
            mit->second.online = online;
        }
    }
}

std::vector<GuildInvitation> GuildSystem::GetInvitations(uint64_t player_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GuildInvitation> result;
    
    for (const auto& inv : invitations_) {
        if (inv.player_id == player_id && !inv.accepted && !inv.declined) {
            result.push_back(inv);
        }
    }
    
    return result;
}

// ============================================================================
// Persistence
// ============================================================================

void GuildSystem::SaveGuilds(const std::string& directory) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // In production, serialize to JSON
    std::cout << "[GuildSystem] Saved " << guilds_.size() << " guilds" << std::endl;
}

void GuildSystem::LoadGuilds(const std::string& directory) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // In production, load from JSON
    std::cout << "[GuildSystem] Loaded " << guilds_.size() << " guilds" << std::endl;
}

// ============================================================================
// Helpers
// ============================================================================

bool GuildSystem::CanManageMember(const Guild& guild, uint64_t player_id) const {
    auto it = guild.members.find(player_id);
    if (it == guild.members.end()) {
        return false;
    }
    return it->second.rank <= GuildRank::Officer;
}

bool GuildSystem::CanInvite(const Guild& guild, uint64_t player_id) const {
    auto it = guild.members.find(player_id);
    if (it == guild.members.end()) {
        return false;
    }
    return it->second.rank <= GuildRank::Officer;
}

bool GuildSystem::ValidateName(const std::string& name) const {
    if (name.length() < 3 || name.length() > MAX_GUILD_NAME_LENGTH) {
        return false;
    }
    for (char c : name) {
        if (!std::isalnum(c) && c != ' ') {
            return false;
        }
    }
    return true;
}

void GuildSystem::AddMember(Guild& guild, uint64_t player_id, const std::string& name, uint8_t level) {
    GuildMember member;
    member.player_id = player_id;
    member.name = name;
    member.rank = GuildRank::Recruit;
    member.level = level;
    member.contribution = 0;
    member.joined_at = std::chrono::system_clock::now();
    member.online = false;
    
    guild.members[player_id] = member;
}

void GuildSystem::RemoveMember(Guild& guild, uint64_t player_id) {
    guild.members.erase(player_id);
}

} // namespace DarkAges::social