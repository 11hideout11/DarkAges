/**
 * @file GuildSystem.hpp
 * @brief Guild system for persistent social groups
 * 
 * Handles guild creation, invites, ranks, chat, and storage.
 */

#pragma once

#include "ecs/CoreTypes.hpp"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <optional>
#include <chrono>
#include <atomic>

namespace DarkAges::social {

// ============================================================================
// Constants
// ============================================================================

constexpr uint32_t MAX_GUILD_NAME_LENGTH = 24;
constexpr uint32_t MAX_GUILD_TAG_LENGTH = 5;
constexpr uint32_t MAX_GUILD_MEMBERS = 50;
constexpr uint32_t MAX_GUILD_RANKS = 5;

// ============================================================================
// Data Structures
// ============================================================================

enum class GuildRank : uint8_t {
    Leader = 0,
    Officer = 1,
    Member = 2,
    Recruit = 3
};

struct GuildMember {
    uint64_t player_id;
    std::string name;
    GuildRank rank;
    uint8_t level;
    uint32_t contribution;
    std::chrono::system_clock::time_point joined_at;
    bool online;
};

struct Guild {
    uint64_t guild_id{0};
    std::string name;
    std::string tagline;
    uint64_t leader_id;
    std::string leader_name;
    uint32_t level{1};
    uint32_t xp{0};
    uint32_t member_count{0};
    uint64_t bank_gold{0};
    
    std::unordered_map<uint64_t, GuildMember> members;
    std::string rank_names[MAX_GUILD_RANKS];
    
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_activity;
};

struct GuildInvitation {
    uint64_t guild_id;
    uint64_t player_id;
    std::chrono::system_clock::time_point invited_at;
    bool accepted{false};
    bool declined{false};
};

// ============================================================================
// GuildSystem
// ============================================================================

class GuildSystem {
public:
    GuildSystem();
    ~GuildSystem();

    // ============================================================================
    // Guild Management
    // ============================================================================

    std::optional<uint64_t> CreateGuild(
        const std::string& name,
        uint64_t leader_id,
        const std::string& leader_name
    );

    bool DisbandGuild(uint64_t guild_id, uint64_t player_id);
    std::optional<Guild> GetGuild(uint64_t guild_id);
    std::optional<uint64_t> GetPlayerGuild(uint64_t player_id);

    // ============================================================================
    // Membership
    // ============================================================================

    bool InvitePlayer(uint64_t guild_id, uint64_t inviter_id, uint64_t target_id);
    bool AcceptInvitation(uint64_t player_id, uint64_t guild_id);
    void DeclineInvitation(uint64_t player_id, uint64_t guild_id);
    bool KickMember(uint64_t guild_id, uint64_t kicker_id, uint64_t target_id);
    bool LeaveGuild(uint64_t player_id);
    bool SetMemberRank(
        uint64_t guild_id,
        uint64_t promoter_id,
        uint64_t target_id,
        GuildRank new_rank
    );

    // ============================================================================
    // Ranks
    // ============================================================================

    bool SetRankName(
        uint64_t guild_id,
        uint64_t setter_id,
        uint8_t rank_index,
        const std::string& name
    );

    // ============================================================================
    // Chat
    // ============================================================================

    bool SendChat(
        uint64_t guild_id,
        uint64_t sender_id,
        const std::string& sender_name,
        const std::string& message
    );

    // ============================================================================
    // Bank
    // ============================================================================

    bool DepositGold(uint64_t guild_id, uint64_t player_id, uint64_t amount);
    bool WithdrawGold(uint64_t guild_id, uint64_t player_id, uint64_t amount);

    // ============================================================================
    // Queries
    // ============================================================================

    std::vector<GuildMember> GetMemberList(uint64_t guild_id);
    bool AreSameGuild(uint64_t player1_id, uint64_t player2_id);
    uint32_t GetOnlineCount(uint64_t guild_id);
    void SetOnlineStatus(uint64_t player_id, bool online);
    std::vector<GuildInvitation> GetInvitations(uint64_t player_id);

    // ============================================================================
    // Persistence
    // ============================================================================

    void SaveGuilds(const std::string& directory);
    void LoadGuilds(const std::string& directory);

private:
    std::mutex mutex_;
    std::unordered_map<uint64_t, Guild> guilds_;
    std::unordered_map<uint64_t, uint64_t> player_guild_;
    std::vector<GuildInvitation> invitations_;
    std::atomic<uint64_t> next_guild_id_{1};

    bool CanManageMember(const Guild& guild, uint64_t player_id) const;
    bool CanInvite(const Guild& guild, uint64_t player_id) const;
    bool ValidateName(const std::string& name) const;
    void AddMember(Guild& guild, uint64_t player_id, const std::string& name, uint8_t level);
    void RemoveMember(Guild& guild, uint64_t player_id);
};

} // namespace DarkAges::social