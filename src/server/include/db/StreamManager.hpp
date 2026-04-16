#pragma once

// Redis Streams operations
// Extracted from RedisManager - XADD/XREAD for non-blocking event streaming

#include "db/PlayerSessionManager.hpp"  // For AsyncResult
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace DarkAges {

class RedisManager;
struct RedisInternal;

// Stream entry (field-value pairs)
struct StreamEntry {
    std::string id;  // Auto-generated or explicit ID
    std::unordered_map<std::string, std::string> fields;
};

// Manages Redis Streams operations (XADD/XREAD)
class StreamManager {
public:
    using StreamAddCallback = std::function<void(const AsyncResult<std::string>& result)>;
    using StreamReadCallback = std::function<void(const AsyncResult<std::vector<StreamEntry>>& result)>;

    explicit StreamManager(RedisManager& redis, RedisInternal& internal);

    // Add entry to stream (XADD)
    void xadd(std::string_view streamKey,
              std::string_view id,
              const std::unordered_map<std::string, std::string>& fields,
              StreamAddCallback callback = nullptr);

    // Read entries from stream (XREAD)
    void xread(std::string_view streamKey,
               std::string_view lastId,
               StreamReadCallback callback,
               uint32_t count = 100,
               uint32_t blockMs = 0);

private:
    RedisManager& redis_;
    RedisInternal& internal_;
};

} // namespace DarkAges
