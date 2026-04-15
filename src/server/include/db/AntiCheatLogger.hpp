#pragma once
#include "ecs/CoreTypes.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <atomic>

// Forward declarations for cassandra-cpp-driver types
struct CassSession;

namespace DarkAges {

// Forward declaration - AntiCheatEvent defined in ScyllaManager.hpp
struct AntiCheatEvent;

// [DATABASE_AGENT] Anti-cheat event logging subsystem
// Extracted from ScyllaManager for cohesion.
// Uses raw string queries (not prepared statements) matching original implementation.
class AntiCheatLogger {
public:
    using WriteCallback = std::function<void(bool success)>;

public:
    AntiCheatLogger();
    ~AntiCheatLogger();

    // Log a single anti-cheat event (async)
    void logAntiCheatEvent(CassSession* session, const AntiCheatEvent& event,
                           WriteCallback callback = nullptr);

    // Batch log multiple anti-cheat events (async)
    void logAntiCheatEventsBatch(CassSession* session,
                                 const std::vector<AntiCheatEvent>& events,
                                 WriteCallback callback = nullptr);

    // Metrics
    [[nodiscard]] uint64_t getWritesQueued() const { return writesQueued_; }
    [[nodiscard]] uint64_t getWritesCompleted() const { return writesCompleted_; }
    [[nodiscard]] uint64_t getWritesFailed() const { return writesFailed_; }

private:
    // Metrics
    uint64_t writesQueued_{0};
    uint64_t writesCompleted_{0};
    uint64_t writesFailed_{0};
};

} // namespace DarkAges
