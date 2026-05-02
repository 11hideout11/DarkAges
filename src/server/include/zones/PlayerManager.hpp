/**
 * @file PlayerManager.hpp
 * @brief Player lifecycle management for zone servers
 *
 * Handles player registration, despawning, connection mapping, and state persistence.
 * Manages the mapping between network connections, player IDs, and ECS entities.
 *
 * @namespace DarkAges
 */

#pragma once

#include "ecs/CoreTypes.hpp"
#include "db/RedisManager.hpp"
#include "db/ScyllaManager.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <functional>

namespace DarkAges {

class ZoneServer;

/**
 * @brief Player lifecycle manager
 *
 * Manages all player-related operations on a zone server including:
 * - Player registration and connection mapping
 * - Entity creation and cleanup
 * - State persistence (Redis caching, Scylla long-term storage)
 * - Player iteration for broadcasts and updates
 *
 * ## Usage
 * @code
 * PlayerManager playerMgr(zoneServer);
 * playerMgr.setDatabaseConnections(redis, scylla);
 * playerMgr.setZoneId(1);
 *
 * // Player connects
 * EntityID player = playerMgr.registerPlayer(connectionId, playerId, "username", spawnPos);
 *
 * // Query player
 * if (playerMgr.isPlayerConnected(playerId)) {
 *     // Handle connected player
 * }
 * @endcode
 */
class PlayerManager {
public:
    /**
     * @brief Construct with null dependencies
     */
    explicit PlayerManager() : zoneServer_(nullptr), redis_(nullptr), scylla_(nullptr), zoneId_(1) {}

    /**
     * @brief Construct with zone server
     * @param zoneServer Pointer to the zone server instance
     */
    explicit PlayerManager(ZoneServer* zoneServer);

    /**
     * @brief Destructor
     */
    ~PlayerManager();

    /**
     * @brief Set the zone server reference
     * @param zoneServer Pointer to the zone server instance
     */
    void setZoneServer(ZoneServer* zoneServer) { zoneServer_ = zoneServer; }

    /**
     * @brief Initialize database connections
     * @param redis Redis manager for caching
     * @param scylla Scylla manager for long-term storage
     */
    void setDatabaseConnections(RedisManager* redis, ScyllaManager* scylla);

    /**
     * @brief Set the zone ID for this manager
     * @param zoneId Zone identifier
     */
    void setZoneId(uint32_t zoneId);

    /**
     * @brief Register a new player connection
     *
     * Creates an ECS entity for the player and establishes connection mappings.
     * The player entity is initialized with default components from PlayerInfo.
     *
     * @param connectionId Network connection handle
     * @param playerId Persistent player ID from authentication
     * @param username Player's display name
     * @param spawnPos Initial spawn position
     * @return Newly created entity ID, or entt::null on failure
     */
    EntityID registerPlayer(ConnectionID connectionId, uint64_t playerId,
                         const std::string& username, const Position& spawnPos);

    /**
     * @brief Unregister and cleanup a player
     *
     * Removes the player entity and all associated mappings.
     * Player state is saved before cleanup.
     *
     * @param entity Player entity to remove
     */
    void unregisterPlayer(EntityID entity);

    /**
     * @brief Despawn a player (remove from zone, retain connection)
     *
     * Removes player from the zone but maintains the connection mapping
     * for reconnection or zone handoff.
     *
     * @param entity Player entity to despawn
     */
    void despawnPlayer(EntityID entity);

    /**
     * @brief Remove connection mapping without entity cleanup
     *
     * Used when connection is lost but entity persists.
     *
     * @param entity Player entity
     */
    void removeConnectionMapping(EntityID entity);

    /**
     * @brief Get entity by connection ID
     * @param connectionId Network connection handle
     * @return Entity ID, or entt::null if not found
     */
    [[nodiscard]] EntityID getEntityByConnection(ConnectionID connectionId) const;

    /**
     * @brief Get connection by entity ID
     * @param entity Player entity
     * @return Connection ID, or 0 if not found
     */
    [[nodiscard]] ConnectionID getConnectionByEntity(EntityID entity) const;

    /**
     * @brief Get entity by player ID
     * @param playerId Persistent player ID
     * @return Entity ID, or entt::null if not found
     */
    [[nodiscard]] EntityID getEntityByPlayerId(uint64_t playerId) const;

    /**
     * @brief Check if player is connected to this zone
     * @param playerId Persistent player ID
     * @return true if player is in this zone
     */
    bool isPlayerConnected(uint64_t playerId) const;

    /**
     * @brief Check if entity is a player
     * @param entity Entity to check
     * @return true if entity is a player in this zone
     */
    bool isEntityPlayer(EntityID entity) const;

    /**
     * @brief Save all player states to database
     *
     * Called during zone shutdown or periodic checkpoints.
     */
    void saveAllPlayerStates();

    /**
     * @brief Get current player count
     * @return Number of connected players in this zone
     */
    size_t getPlayerCount() const { return connectionToEntity_.size(); }

    /**
     * @brief Iterate all players
     *
     * Safe for broadcasts and bulk operations.
     *
     * @param callback Function called with (entity, connectionId, playerInfo)
     */
    void forEachPlayer(const std::function<void(EntityID, ConnectionID, const PlayerInfo*)>& callback);

    /**
     * @brief Save a single player's state
     * @param entity Player entity
     */
    void savePlayerState(EntityID entity);

    /**
     * @brief Get current server time in milliseconds
     * @return Current tick time
     */
    uint32_t getCurrentTimeMs() const;

    /**
     * @brief Get all player connection/entity pairs
     * @return Vector of (connectionId, entity) pairs
     */
    std::vector<std::pair<ConnectionID, EntityID>> getAllPlayers() const;

private:
    ZoneServer* zoneServer_;
    RedisManager* redis_;
    ScyllaManager* scylla_;
    uint32_t zoneId_;

    std::unordered_map<ConnectionID, EntityID> connectionToEntity_;
    std::unordered_map<EntityID, ConnectionID> entityToConnection_;
    std::unordered_map<uint64_t, EntityID> playerIdToEntity_;

    void updatePlayerIdMapping(EntityID entity, uint64_t playerId);
    void removePlayerIdMapping(EntityID entity);
};

} // namespace DarkAges