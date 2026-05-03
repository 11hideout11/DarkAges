/**
 * @file PartySystem.hpp
 * @brief Party system for temporary grouping
 * 
 * Handles party creation, invites, shared health bars, and XP sharing.
 */

#pragma once

#include "ecs/CoreTypes.hpp"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <optional>
#include <chrono>

namespace DarkAges::social {

// ============================================================================
// Constants
// ============================================================================

constexpr uint32_t MAX_PARTY_SIZE = 6;  // Including party leader

// ============================================================================
// Data Structures
// ============================================================================

enum class PartyLootMode : uint8_t {
    FreeForAll = 0,      // Everyone can loot
    RoundRobin = 1,      // Rotate loot rights
    LeaderGets = 2,       // Leader gets priority
    ByRoll = 3           // Need greed
};

enum class PartyXPShare : uint8_t {
    Off = 0,            // No XP sharing
    Shared = 1,         // Split XP
    LeaderBonus = 2       // Extra for leader
};

struct PartyMember {
    uint64_t player_id;
    std::string name;
    uint8_t level;
    float health_percent;
    uint32_t max_health;
    bool is_leader;
};

struct Party {
    uint64_t party_id{0};
    uint64_t leader_id;
    std::vector<PartyMember> members;
    PartyLootMode loot_mode{PartyLootMode::FreeForAll};
    PartyXPShare xp_share{PartyXPShare::Off};
    
    std::chrono::system_clock::time_point created_at;
};

// ============================================================================
// PartySystem
// ============================================================================

class PartySystem {
public:
    PartySystem();
    ~PartySystem();

    // ============================================================================
    // Party Management
    // ============================================================================

    /**
     * @brief Create a party
     * @param leader_id Party creator
     * @param leader_name Creator name
     * @return Party ID or nullopt
     */
    std::optional<uint64_t> CreateParty(uint64_t leader_id, const std::string& leader_name);

    /**
     * @brief Disband party
     * @param party_id Party to disband
     * @param player_id Player requesting
     * @return true if disbanded
     */
    bool DisbandParty(uint64_t party_id, uint64_t player_id);

    /**
     * @brief Get party
     * @param party_id Party to query
     * @return Party data or nullopt
     */
    std::optional<Party> GetParty(uint64_t party_id);

    /**
     * @brief Get player's party
     * @param player_id Player
     * @return Party ID or nullopt
     */
    std::optional<uint64_t> GetPlayerParty(uint64_t player_id);

    // ============================================================================
    // Membership
    // ============================================================================

    /**
     * @brief Invite player to party
     * @param party_id Target party
     * @param inviter_id Player sending invite
     * @param target_id Player to invite
     * @return true if invited
     */
    bool InvitePlayer(uint64_t party_id, uint64_t inviter_id, uint64_t target_id);

    /**
     * @brief Accept invitation
     * @param player_id Player accepting
     * @param party_id Party to join
     * @return true if joined
     */
    bool AcceptInvitation(uint64_t player_id, uint64_t party_id);

    /**
     * @brief Decline invitation
     * @param player_id Player declining
     * @param party_id Party
     */
    void DeclineInvitation(uint64_t player_id, uint64_t party_id);

    /**
     * @brief Kick member (leader only)
     * @param party_id Party
     * @param kicker_id Leader kicking
     * @param target_id Member to kick
     * @return true if kicked
     */
    bool KickMember(uint64_t party_id, uint64_t kicker_id, uint64_t target_id);

    /**
     * @brief Leave party
     * @param player_id Player leaving
     * @return true if left
     */
    bool LeaveParty(uint64_t player_id);

    /**
     * @brief Transfer leadership
     * @param party_id Party
     * @param current_leader Current leader
     * @param new_leader_id New leader
     * @return true if transferred
     */
    bool TransferLeadership(uint64_t party_id, uint64_t current_leader, uint64_t new_leader_id);

    // ============================================================================
    // Options
    // ============================================================================

    /**
     * @brief Set loot mode
     * @param party_id Party
     * @param setter_id Player setting (leader)
     * @param mode Loot mode
     * @return true if set
     */
    bool SetLootMode(uint64_t party_id, uint64_t setter_id, PartyLootMode mode);

    /**
     * @brief Set XP share mode
     * @param party_id Party
     * @param setter_id Player setting (leader)
     * @param mode XP share mode
     * @return true if set
     */
    bool SetXPShare(uint64_t party_id, uint64_t setter_id, PartyXPShare mode);

    // ============================================================================
    // Health Sharing
    // ============================================================================

    /**
     * @brief Update member health
     * @param party_id Party
     * @param player_id Player
     * @param health_percent Current health % (0-100)
     * @param max_health Max health
     */
    void UpdateMemberHealth(uint64_t party_id, uint64_t player_id, float health_percent, uint32_t max_health);

    /**
     * @brief Get party health data
     * @param party_id Party
     * @return Map of player_id to health info
     */
    std::vector<PartyMember> GetMembers(uint64_t party_id);

    // ============================================================================
    // Queries
    // ============================================================================

    /**
     * @brief Check if two players are in same party
     * @param player1_id First player
     * @param player2_id Second player
     * @return true if same party
     */
    bool AreSameParty(uint64_t player1_id, uint64_t player2_id);

    /**
     * @brief Get party size
     * @param party_id Party
     * @return Member count
     */
    uint32_t GetPartySize(uint64_t party_id);

private:
    std::mutex mutex_;
    std::unordered_map<uint64_t, Party> parties_;
    std::unordered_map<uint64_t, uint64_t> player_party_;
    std::vector<uint64_t> pending_invitations_;
    std::atomic<uint64_t> next_party_id_{1};

    bool CanInvite(const Party& party, uint64_t player_id) const;
    bool CanKick(const Party& party, uint64_t player_id) const;
};

} // namespace DarkAges::social