// Zone player set operations implementation
// Manages SADD/SREM/SMEMBERS on Redis zone player sets

#include "db/ZoneManager.hpp"
#include "db/RedisManager.hpp"
#include "db/RedisInternal.hpp"
#include "db/ConnectionPool.hpp"
#include <chrono>
#include <mutex>

#ifdef REDIS_AVAILABLE
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <hiredis.h>
#endif

namespace DarkAges {

ZoneManager::ZoneManager(RedisManager& redis, RedisInternal& internal)
    : redis_(redis), internal_(internal) {}

void ZoneManager::addPlayerToZone(uint32_t zoneId, uint64_t playerId, SetCallback callback) {
    internal_.commandsSent_++;
    
    if (!internal_.connected) {
        internal_.commandsFailed_++;
        if (callback) {
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback]() { callback(false); }, std::chrono::steady_clock::now()});
        }
        return;
    }
    
    #ifdef REDIS_AVAILABLE
    auto* ctx = internal_.pool->acquire();
    if (!ctx) {
        internal_.commandsFailed_++;
        if (callback) {
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback]() { callback(false); }, std::chrono::steady_clock::now()});
        }
        return;
    }
    
    std::string key = RedisKeys::zonePlayers(zoneId);
    redisReply* reply = (redisReply*)redisCommand(ctx, "SADD %s %llu",
                                                  key.c_str(),
                                                  static_cast<unsigned long long>(playerId));
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    
    if (reply) {
        freeReplyObject(reply);
    }
    
    internal_.pool->release(ctx);
    
    if (success) {
        internal_.commandsCompleted_++;
    } else {
        internal_.commandsFailed_++;
    }
    
    if (callback) {
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, success]() { callback(success); }, std::chrono::steady_clock::now()});
    }
    #else
    (void)zoneId;
    (void)playerId;
    internal_.commandsCompleted_++;
    if (callback) {
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback]() { callback(true); }, std::chrono::steady_clock::now()});
    }
    #endif
}

void ZoneManager::removePlayerFromZone(uint32_t zoneId, uint64_t playerId, SetCallback callback) {
    internal_.commandsSent_++;
    
    if (!internal_.connected) {
        internal_.commandsFailed_++;
        if (callback) {
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback]() { callback(false); }, std::chrono::steady_clock::now()});
        }
        return;
    }
    
    #ifdef REDIS_AVAILABLE
    auto* ctx = internal_.pool->acquire();
    if (!ctx) {
        internal_.commandsFailed_++;
        if (callback) {
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback]() { callback(false); }, std::chrono::steady_clock::now()});
        }
        return;
    }
    
    std::string key = RedisKeys::zonePlayers(zoneId);
    redisReply* reply = (redisReply*)redisCommand(ctx, "SREM %s %llu",
                                                  key.c_str(),
                                                  static_cast<unsigned long long>(playerId));
    bool success = (reply != nullptr && reply->type == REDIS_REPLY_INTEGER);
    
    if (reply) {
        freeReplyObject(reply);
    }
    
    internal_.pool->release(ctx);
    
    if (success) {
        internal_.commandsCompleted_++;
    } else {
        internal_.commandsFailed_++;
    }
    
    if (callback) {
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, success]() { callback(success); }, std::chrono::steady_clock::now()});
    }
    #else
    (void)zoneId;
    (void)playerId;
    internal_.commandsCompleted_++;
    if (callback) {
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback]() { callback(true); }, std::chrono::steady_clock::now()});
    }
    #endif
}

void ZoneManager::getZonePlayers(uint32_t zoneId, ZonePlayersCallback callback) {
    internal_.commandsSent_++;
    
    if (!internal_.connected || !callback) {
        internal_.commandsFailed_++;
        if (callback) {
            AsyncResult<std::vector<uint64_t>> result;
            result.success = false;
            result.error = "Not connected";
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback, result]() { callback(result); }, std::chrono::steady_clock::now()});
        }
        return;
    }
    
    #ifdef REDIS_AVAILABLE
    auto* ctx = internal_.pool->acquire();
    if (!ctx) {
        internal_.commandsFailed_++;
        AsyncResult<std::vector<uint64_t>> result;
        result.success = false;
        result.error = "Failed to acquire connection";
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, result]() { callback(result); }, std::chrono::steady_clock::now()});
        return;
    }
    
    std::string key = RedisKeys::zonePlayers(zoneId);
    redisReply* reply = (redisReply*)redisCommand(ctx, "SMEMBERS %s", key.c_str());
    
    AsyncResult<std::vector<uint64_t>> result;
    if (!reply) {
        result.success = false;
        result.error = "Command failed";
        internal_.commandsFailed_++;
    } else if (reply->type == REDIS_REPLY_ARRAY) {
        result.success = true;
        result.value.reserve(reply->elements);
        for (size_t i = 0; i < reply->elements; ++i) {
            if (reply->element[i]->type == REDIS_REPLY_STRING ||
                reply->element[i]->type == REDIS_REPLY_INTEGER) {
                uint64_t playerId = 0;
                if (reply->element[i]->type == REDIS_REPLY_INTEGER) {
                    playerId = static_cast<uint64_t>(reply->element[i]->integer);
                } else {
                    playerId = std::stoull(std::string(reply->element[i]->str, reply->element[i]->len));
                }
                result.value.push_back(playerId);
            }
        }
        internal_.commandsCompleted_++;
    } else {
        result.success = false;
        result.error = "Unexpected reply type";
        internal_.commandsFailed_++;
    }
    
    if (reply) {
        freeReplyObject(reply);
    }
    
    internal_.pool->release(ctx);
    
    std::lock_guard<std::mutex> lock(internal_.callbackMutex);
    internal_.callbackQueue.push({[callback, result]() { callback(result); }, std::chrono::steady_clock::now()});
    #else
    (void)zoneId;
    internal_.commandsCompleted_++;
    AsyncResult<std::vector<uint64_t>> result;
    result.success = true;
    std::lock_guard<std::mutex> lock(internal_.callbackMutex);
    internal_.callbackQueue.push({[callback, result]() { callback(result); }, std::chrono::steady_clock::now()});
    #endif
}

} // namespace DarkAges
