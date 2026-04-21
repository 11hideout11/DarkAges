#include "combat/GuildSystem.hpp"
#include <cstring>
#include <algorithm>
#include <iostream>

namespace DarkAges {

// ============================================================================
// Guild Management
// ============================================================================

uint32_t GuildSystem::createGuild(EntityID founder, const char* guildName, uint32_t currentTimeMs) {
    // Check if founder is already in a guild
    if (isInGuild(founder)) return 0;

    // Validate name
    if (!guildName || guildName[0] == '\0') return 0;

    // Check if name is already taken
    for (const auto& [id, guild] : guilds_) {
        if (std::strncmp(guild.name, guildName, GUILD_NAME_MAX - 1) == 0) {
            return 0;  // Name taken
        }
    }

    uint32_t guildId = nextGuildId_++;
    GuildData& guild = guilds_[guildId];
    guild.guildId = guildId;
    std::strncpy(guild.name, guildName, GUILD_NAME_MAX - 1);
    guild.name[GUILD_NAME_MAX - 1] = '\0';
    guild.leader = founder;
    guild.members.push_back(founder);
    guild.createdAt = currentTimeMs;

    std::cout << "[GUILD] Guild '" << guild.name << "' (ID " << guildId
              << ") created by entity " << static_cast<uint32_t>(founder) << std::endl;

    return guildId;
}

bool GuildSystem::disbandGuild(uint32_t guildId, EntityID requester) {
    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return false;

    if (it->second.leader != requester) return false;

    std::cout << "[GUILD] Guild '" << it->second.name << "' disbanded" << std::endl;

    guilds_.erase(it);
    return true;
}

const char* GuildSystem::getGuildName(uint32_t guildId) const {
    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return "";
    return it->second.name;
}

// ============================================================================
// Membership
// ============================================================================

bool GuildSystem::invitePlayer(EntityID inviter, EntityID target) {
    uint32_t guildId = getGuildId(inviter);
    if (guildId == 0) return false;

    // Must be officer or leader to invite
    GuildRank rank = getRank(inviter);
    if (rank < GuildRank::Officer) return false;

    // Check if target is already in a guild
    if (isInGuild(target)) return false;

    if (isGuildFull(guildId)) return false;

    // Add pending invite
    PendingInvite invite;
    invite.guildId = guildId;
    invite.expireTimeMs = 0;
    pendingInvites_[static_cast<uint32_t>(target)].push_back(invite);

    return true;
}

bool GuildSystem::acceptInvite(EntityID player, uint32_t guildId, uint32_t currentTimeMs) {
    if (!hasPendingInvite(player, guildId)) return false;
    if (isInGuild(player)) return false;

    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return false;
    if (isGuildFull(guildId)) return false;

    it->second.members.push_back(player);
    declineInvite(player, guildId);

    std::cout << "[GUILD] Entity " << static_cast<uint32_t>(player)
              << " joined guild '" << it->second.name << "'" << std::endl;

    return true;
}

void GuildSystem::declineInvite(EntityID player, uint32_t guildId) {
    auto it = pendingInvites_.find(static_cast<uint32_t>(player));
    if (it == pendingInvites_.end()) return;

    auto& invites = it->second;
    invites.erase(
        std::remove_if(invites.begin(), invites.end(),
                       [guildId](const PendingInvite& i) { return i.guildId == guildId; }),
        invites.end()
    );

    if (invites.empty()) pendingInvites_.erase(it);
}

bool GuildSystem::removeFromGuild(EntityID player, EntityID requester) {
    uint32_t guildId = getGuildId(player);
    if (guildId == 0) return false;

    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return false;

    // Can leave voluntarily, or officers+ can kick, or leader can kick anyone
    if (player != requester) {
        GuildRank requesterRank = getRank(requester);
        GuildRank targetRank = getRank(player);

        if (requesterRank == GuildRank::Officer && targetRank >= GuildRank::Officer) {
            return false;  // Officers can't kick other officers
        }
        if (requesterRank < GuildRank::Officer) {
            return false;  // Members can't kick anyone
        }
    }

    // If leader is leaving
    if (player == it->second.leader) {
        if (it->second.members.size() <= 1) {
            guilds_.erase(it);
            std::cout << "[GUILD] Guild disbanded (leader left)" << std::endl;
            return true;
        }
        // Transfer to highest-ranked member
        EntityID newLeader = entt::null;
        GuildRank bestRank = GuildRank::Member;
        for (EntityID member : it->second.members) {
            if (member == player) continue;
            GuildRank r = getRank(member);
            if (r > bestRank || newLeader == entt::null) {
                bestRank = r;
                newLeader = member;
            }
        }
        it->second.leader = newLeader;
    }

    removeMemberFromGuild(guildId, player);

    std::cout << "[GUILD] Entity " << static_cast<uint32_t>(player)
              << " left guild '" << it->second.name << "'" << std::endl;

    if (it->second.members.empty()) guilds_.erase(it);
    return true;
}

bool GuildSystem::leaveGuild(EntityID player) {
    return removeFromGuild(player, player);
}

// ============================================================================
// Rank Management
// ============================================================================

bool GuildSystem::setRank(EntityID player, GuildRank newRank, EntityID requester) {
    uint32_t guildId = getGuildId(player);
    if (guildId == 0 || getGuildId(requester) != guildId) return false;

    GuildRank requesterRank = getRank(requester);
    GuildRank currentRank = getRank(player);

    // Leader can set any rank
    if (requesterRank == GuildRank::Leader) {
        // Can't set rank to Leader (only one leader)
        if (newRank == GuildRank::Leader) return false;
        return true;  // Would update component in actual ECS usage
    }

    // Officers can promote/demote members only
    if (requesterRank == GuildRank::Officer) {
        if (currentRank != GuildRank::Member) return false;
        if (newRank != GuildRank::Member) return true;  // Promote to officer
        return false;
    }

    return false;
}

// ============================================================================
// Queries
// ============================================================================

uint32_t GuildSystem::getGuildId(EntityID player) const {
    for (const auto& [id, guild] : guilds_) {
        for (EntityID member : guild.members) {
            if (member == player) return id;
        }
    }
    return 0;
}

bool GuildSystem::isInGuild(EntityID player) const {
    return getGuildId(player) != 0;
}

GuildRank GuildSystem::getRank(EntityID player) const {
    uint32_t guildId = getGuildId(player);
    if (guildId == 0) return GuildRank::None;

    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return GuildRank::None;

    if (it->second.leader == player) return GuildRank::Leader;

    // In a real implementation, this would check the GuildComponent
    // For now, everyone except the leader is a Member
    return GuildRank::Member;
}

std::vector<EntityID> GuildSystem::getGuildMembers(uint32_t guildId) const {
    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return {};
    return it->second.members;
}

uint32_t GuildSystem::getGuildSize(uint32_t guildId) const {
    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return 0;
    return static_cast<uint32_t>(it->second.members.size());
}

bool GuildSystem::isGuildFull(uint32_t guildId) const {
    return getGuildSize(guildId) >= MAX_GUILD_SIZE;
}

bool GuildSystem::hasPendingInvite(EntityID player, uint32_t guildId) const {
    auto it = pendingInvites_.find(static_cast<uint32_t>(player));
    if (it == pendingInvites_.end()) return false;

    for (const auto& invite : it->second) {
        if (invite.guildId == guildId) return true;
    }
    return false;
}

// ============================================================================
// Component Management
// ============================================================================

void GuildSystem::ensureComponent(Registry& registry, EntityID entity) {
    if (!registry.all_of<GuildComponent>(entity)) {
        registry.emplace<GuildComponent>(entity);
    }
}

void GuildSystem::onPlayerDisconnect(Registry& registry, EntityID player) {
    leaveGuild(player);
}

// ============================================================================
// Private Helpers
// ============================================================================

void GuildSystem::removeMemberFromGuild(uint32_t guildId, EntityID player) {
    auto it = guilds_.find(guildId);
    if (it == guilds_.end()) return;

    auto& members = it->second.members;
    members.erase(
        std::remove(members.begin(), members.end(), player),
        members.end()
    );
}

} // namespace DarkAges
