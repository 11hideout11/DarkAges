#pragma once

// Zone player set operations for Redis
// Extracted from RedisManager - manages SADD/SREM/SMEMBERS on zone player sets

#include "db/PlayerSessionManager.hpp"  // For AsyncResult
#include <cstdint>
#include <functional>
#include <vector>
#include <string>

namespace DarkAges {

class RedisManager;
struct RedisInternal;

// Manages zone player sets in Redis (SADD/SREM/SMEMBERS)
class ZoneManager {
public:
    using SetCallback = std::function<void(bool success)>;
    using ZonePlayersCallback = std::function<void(const AsyncResult<std::vector<uint64_t>>&)>;

    explicit ZoneManager(RedisManager& redis, RedisInternal& internal);

    // Add player to zone set
    void addPlayerToZone(uint32_t zoneId, uint64_t playerId, SetCallback callback = nullptr);
    
    // Remove player from zone set
    void removePlayerFromZone(uint32_t zoneId, uint64_t playerId, SetCallback callback = nullptr);
    
    // Get all players in a zone
    void getZonePlayers(uint32_t zoneId, ZonePlayersCallback callback);

private:
    RedisManager& redis_;
    RedisInternal& internal_;
};

} // namespace DarkAges
