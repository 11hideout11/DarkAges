#pragma once

#include "ecs/CoreTypes.hpp"
#include "security/AntiCheat.hpp"
#include "db/ScyllaManager.hpp"
#include <unordered_map>
#include <cstdint>
#include <string>

namespace DarkAges {

class ZoneServer;
class NetworkManager;
class RedisManager;

// Handles anti-cheat initialization, cheat detection, ban/kick processing, and violation logging.
// Extracted from ZoneServer to reduce monolithic file size.
class AntiCheatHandler {
public:
    explicit AntiCheatHandler(ZoneServer& server);
    ~AntiCheatHandler() = default;

    // Set subsystem references (called during ZoneServer::initialize)
    void setNetwork(NetworkManager* network) { network_ = network; }
    void setRedis(RedisManager* redis) { redis_ = redis; }
    void setScylla(ScyllaManager* scylla) { scylla_ = scylla; }
    void setAntiCheat(Security::AntiCheatSystem* antiCheat) { antiCheat_ = antiCheat; }

    // Set entity-connection mapping references (owned by ZoneServer)
    void setConnectionMappings(
        std::unordered_map<ConnectionID, EntityID>* connToEntity,
        std::unordered_map<EntityID, ConnectionID>* entityToConn);

    // Initialize anti-cheat system: set spatial hash, wire up callbacks
    void initialize();

    // Handle cheat detection event - logs to ScyllaDB, updates Prometheus metrics
    void onCheatDetected(uint64_t playerId, const Security::CheatDetectionResult& result);

    // Handle player ban event - persists to Redis, kicks connected sessions
    void onPlayerBanned(uint64_t playerId, const char* reason, uint32_t durationMinutes);

    // Handle player kick event - disconnects the player
    void onPlayerKicked(uint64_t playerId, const char* reason);

private:
    ZoneServer& server_;
    NetworkManager* network_{nullptr};
    RedisManager* redis_{nullptr};
    ScyllaManager* scylla_{nullptr};
    Security::AntiCheatSystem* antiCheat_{nullptr};

    // Borrowed references to ZoneServer's connection mappings
    std::unordered_map<ConnectionID, EntityID>* connectionToEntity_{nullptr};
    std::unordered_map<EntityID, ConnectionID>* entityToConnection_{nullptr};
};

} // namespace DarkAges
