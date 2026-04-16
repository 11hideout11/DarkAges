// Redis Streams operations implementation
// XADD/XREAD for non-blocking event streaming

#include "db/StreamManager.hpp"
#include "db/RedisManager.hpp"
#include "db/RedisInternal.hpp"
#include "db/ConnectionPool.hpp"
#include <chrono>
#include <mutex>
#include <iostream>

#ifdef REDIS_AVAILABLE
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <hiredis.h>
#endif

namespace DarkAges {

StreamManager::StreamManager(RedisManager& redis, RedisInternal& internal)
    : redis_(redis), internal_(internal) {}

void StreamManager::xadd(std::string_view streamKey,
                         std::string_view id,
                         const std::unordered_map<std::string, std::string>& fields,
                         StreamAddCallback callback) {
    internal_.commandsSent_++;
    
    if (!internal_.connected) {
        internal_.commandsFailed_++;
        if (callback) {
            AsyncResult<std::string> result;
            result.success = false;
            result.error = "Not connected to Redis";
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                          std::chrono::steady_clock::now()});
        }
        return;
    }
    
    if (fields.empty()) {
        internal_.commandsFailed_++;
        if (callback) {
            AsyncResult<std::string> result;
            result.success = false;
            result.error = "At least one field-value pair required";
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                          std::chrono::steady_clock::now()});
        }
        return;
    }
    
    #ifdef REDIS_AVAILABLE
    auto start = std::chrono::high_resolution_clock::now();
    
    auto* ctx = internal_.pool->acquire();
    if (!ctx) {
        internal_.commandsFailed_++;
        if (callback) {
            AsyncResult<std::string> result;
            result.success = false;
            result.error = "Failed to acquire connection from pool";
            std::lock_guard<std::mutex> lock(internal_.callbackMutex);
            internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                          std::chrono::steady_clock::now()});
        }
        return;
    }
    
    // Build XADD command with field-value pairs
    std::string command = "XADD";
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    
    argv.push_back(command.c_str());
    argvlen.push_back(command.length());
    
    argv.push_back(streamKey.data());
    argvlen.push_back(streamKey.size());
    
    argv.push_back(id.data());
    argvlen.push_back(id.size());
    
    // Add field-value pairs
    std::vector<std::string> fieldValues;
    for (const auto& [field, value] : fields) {
        fieldValues.push_back(field);
        fieldValues.push_back(value);
    }
    
    for (const auto& fv : fieldValues) {
        argv.push_back(fv.c_str());
        argvlen.push_back(fv.length());
    }
    
    redisReply* reply = (redisReply*)redisCommandArgv(ctx, static_cast<int>(argv.size()), 
                                                      argv.data(), argvlen.data());
    
    AsyncResult<std::string> result;
    if (reply && reply->type == REDIS_REPLY_STRING) {
        result.success = true;
        result.value = std::string(reply->str, reply->len);
        internal_.commandsCompleted_++;
    } else {
        result.success = false;
        if (reply && reply->type == REDIS_REPLY_ERROR) {
            result.error = std::string(reply->str, reply->len);
        } else if (ctx->err) {
            result.error = std::string(ctx->errstr);
        } else {
            result.error = "XADD command failed with unknown error";
        }
        internal_.commandsFailed_++;
        
        std::cerr << "[REDIS] XADD failed: " << result.error << std::endl;
        if (reply) {
            std::cerr << "[REDIS] Reply type: " << reply->type << std::endl;
        }
    }
    
    if (reply) {
        freeReplyObject(reply);
    }
    
    internal_.pool->release(ctx);
    
    // Track latency
    auto end = std::chrono::high_resolution_clock::now();
    float latencyMs = std::chrono::duration<float, std::milli>(end - start).count();
    {
        std::lock_guard<std::mutex> lock(internal_.latencyMutex);
        internal_.latencySamples.push(latencyMs);
        if (internal_.latencySamples.size() > RedisInternal::MAX_LATENCY_SAMPLES) {
            internal_.latencySamples.pop();
        }
    }
    
    if (callback) {
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                      std::chrono::steady_clock::now()});
    }
    #else
    // Stub implementation
    internal_.commandsCompleted_++;
    if (callback) {
        AsyncResult<std::string> result;
        result.success = true;
        result.value = "0-0"; // Fake stream ID
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                      std::chrono::steady_clock::now()});
    }
    #endif
}

void StreamManager::xread(std::string_view streamKey,
                          std::string_view lastId,
                          StreamReadCallback callback,
                          uint32_t count,
                          uint32_t blockMs) {
    if (!callback) return;
    
    internal_.commandsSent_++;
    
    if (!internal_.connected) {
        internal_.commandsFailed_++;
        AsyncResult<std::vector<StreamEntry>> result;
        result.success = false;
        result.error = "Not connected to Redis";
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                      std::chrono::steady_clock::now()});
        return;
    }
    
    #ifdef REDIS_AVAILABLE
    auto start = std::chrono::high_resolution_clock::now();
    
    auto* ctx = internal_.pool->acquire();
    if (!ctx) {
        internal_.commandsFailed_++;
        AsyncResult<std::vector<StreamEntry>> result;
        result.success = false;
        result.error = "Failed to acquire connection from pool";
        std::lock_guard<std::mutex> lock(internal_.callbackMutex);
        internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                      std::chrono::steady_clock::now()});
        return;
    }
    
    // Build XREAD command
    redisReply* reply;
    if (blockMs > 0 && count > 0) {
        reply = (redisReply*)redisCommand(ctx, "XREAD COUNT %u BLOCK %u STREAMS %b %b",
                                          count, blockMs,
                                          streamKey.data(), streamKey.size(),
                                          lastId.data(), lastId.size());
    } else if (count > 0) {
        reply = (redisReply*)redisCommand(ctx, "XREAD COUNT %u STREAMS %b %b",
                                          count,
                                          streamKey.data(), streamKey.size(),
                                          lastId.data(), lastId.size());
    } else if (blockMs > 0) {
        reply = (redisReply*)redisCommand(ctx, "XREAD BLOCK %u STREAMS %b %b",
                                          blockMs,
                                          streamKey.data(), streamKey.size(),
                                          lastId.data(), lastId.size());
    } else {
        reply = (redisReply*)redisCommand(ctx, "XREAD STREAMS %b %b",
                                          streamKey.data(), streamKey.size(),
                                          lastId.data(), lastId.size());
    }
    
    AsyncResult<std::vector<StreamEntry>> result;
    
    if (reply && reply->type == REDIS_REPLY_ARRAY && reply->elements > 0) {
        result.success = true;
        
        if (reply->element[0]->type == REDIS_REPLY_ARRAY && reply->element[0]->elements >= 2) {
            redisReply* entries = reply->element[0]->element[1];
            
            if (entries->type == REDIS_REPLY_ARRAY) {
                for (size_t i = 0; i < entries->elements; ++i) {
                    redisReply* entry = entries->element[i];
                    
                    if (entry->type == REDIS_REPLY_ARRAY && entry->elements >= 2) {
                        StreamEntry streamEntry;
                        
                        if (entry->element[0]->type == REDIS_REPLY_STRING) {
                            streamEntry.id = std::string(entry->element[0]->str, entry->element[0]->len);
                        }
                        
                        redisReply* fieldValues = entry->element[1];
                        if (fieldValues->type == REDIS_REPLY_ARRAY) {
                            for (size_t j = 0; j + 1 < fieldValues->elements; j += 2) {
                                std::string field(fieldValues->element[j]->str, fieldValues->element[j]->len);
                                std::string value(fieldValues->element[j + 1]->str, fieldValues->element[j + 1]->len);
                                streamEntry.fields[field] = value;
                            }
                        }
                        
                        result.value.push_back(std::move(streamEntry));
                    }
                }
            }
        }
        
        internal_.commandsCompleted_++;
    } else if (reply && reply->type == REDIS_REPLY_NIL) {
        result.success = true;
        result.value.clear();
        internal_.commandsCompleted_++;
    } else {
        result.success = false;
        result.error = reply && reply->str ? std::string(reply->str) : "XREAD command failed";
        internal_.commandsFailed_++;
    }
    
    if (reply) {
        freeReplyObject(reply);
    }
    
    internal_.pool->release(ctx);
    
    // Track latency
    auto end = std::chrono::high_resolution_clock::now();
    float latencyMs = std::chrono::duration<float, std::milli>(end - start).count();
    {
        std::lock_guard<std::mutex> lock(internal_.latencyMutex);
        internal_.latencySamples.push(latencyMs);
        if (internal_.latencySamples.size() > RedisInternal::MAX_LATENCY_SAMPLES) {
            internal_.latencySamples.pop();
        }
    }
    
    std::lock_guard<std::mutex> lock(internal_.callbackMutex);
    internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                  std::chrono::steady_clock::now()});
    #else
    // Stub implementation
    internal_.commandsCompleted_++;
    AsyncResult<std::vector<StreamEntry>> result;
    result.success = true;
    result.value.clear();
    std::lock_guard<std::mutex> lock(internal_.callbackMutex);
    internal_.callbackQueue.push({[callback, result]() { callback(result); }, 
                                  std::chrono::steady_clock::now()});
    #endif
}

} // namespace DarkAges
