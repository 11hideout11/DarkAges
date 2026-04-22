#pragma once

#include "ecs/CoreTypes.hpp"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

// Guild System — manages player guilds of up to MAX_GUILD_SIZE members.
// Handles guild creation, membership, ranks, and guild chat.
// Provides member lookup for ChatSystem guild message routing.

namespace DarkAges {

class GuildSystem {
public:
    GuildSystem() = default;

    // --- Guild Management ---

    // Create a new guild
    // Returns the guild ID, or 0 if failed (name taken, player already in guild, etc.)
    uint32_t createGuild(EntityID founder, const char* guildName, uint32_t currentTimeMs);

    // Disband a guild (leader only)
    bool disbandGuild(uint32_t guildId, EntityID requester);

    // Get guild name
    const char* getGuildName(uint32_t guildId) const;

    // --- Membership ---

    // Invite a player to a guild
    bool invitePlayer(EntityID inviter, EntityID target);

    // Accept a pending invitation
    bool acceptInvite(EntityID player, uint32_t guildId, uint32_t currentTimeMs);

    // Decline or ignore an invitation
    void declineInvite(EntityID player, uint32_t guildId);

    // Remove a player from their guild (officers+ can kick, anyone can leave)
    bool removeFromGuild(EntityID player, EntityID requester);

    // Leave current guild
    bool leaveGuild(EntityID player);

    // --- Rank Management ---

    // Set a player's rank (officers+ can promote members, leader can promote to officer)
    bool setRank(EntityID player, GuildRank newRank, EntityID requester);

    // --- Queries ---

    // Get the guild ID for a player (0 if not in a guild)
    uint32_t getGuildId(EntityID player) const;

    // Check if player is in a guild
    bool isInGuild(EntityID player) const;

    // Get player's guild rank
    GuildRank getRank(EntityID player) const;

    // Get all online members of a guild
    std::vector<EntityID> getGuildMembers(uint32_t guildId) const;

    // Get guild size
    uint32_t getGuildSize(uint32_t guildId) const;

    // Check if a guild is full
    bool isGuildFull(uint32_t guildId) const;

    // Check if player has a pending invite to a guild
    bool hasPendingInvite(EntityID player, uint32_t guildId) const;

    // --- Component Management ---

    // Add GuildComponent to entity if not present
    void ensureComponent(Registry& registry, EntityID entity);

    // Clean up when a player disconnects
    void onPlayerDisconnect(Registry& registry, EntityID player);
private:
    // Per-member rank overrides (only stores non-default ranks like Officer)
    // Member default rank is Member; leader is determined from GuildData.leader
    mutable std::unordered_map<EntityID, GuildRank> memberRanks_;

    // Guild data
    struct GuildData {
        uint32_t guildId{0};
        char name[GUILD_NAME_MAX]{0};
        EntityID leader{entt::null};
        std::vector<EntityID> members;
        uint32_t createdAt{0};
    };

    // Pending invitation
    struct PendingInvite {
        uint32_t guildId{0};
        uint32_t expireTimeMs{0};
    };

    uint32_t nextGuildId_{1};
    std::unordered_map<uint32_t, GuildData> guilds_;
    std::unordered_map<uint32_t, std::vector<PendingInvite>> pendingInvites_;

    static constexpr uint32_t INVITE_EXPIRE_MS = 60000;  // 60 seconds

    // Remove player from a guild's member list
    void removeMemberFromGuild(uint32_t guildId, EntityID player);
};

} // namespace DarkAges
