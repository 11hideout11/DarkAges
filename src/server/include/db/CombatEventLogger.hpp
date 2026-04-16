#pragma once
#include "ecs/CoreTypes.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <utility>

// Forward declarations for cassandra-cpp-driver types
struct CassSession;
struct CassPrepared;

namespace DarkAges {

// Forward declarations - structs defined in ScyllaManager.hpp
struct CombatEvent;
struct PlayerCombatStats;

// [DATABASE_AGENT] Combat event logging and player stats subsystem
// Extracted from ScyllaManager for cohesion.
class CombatEventLogger {
public:
    using WriteCallback = std::function<void(bool success)>;

public:
    CombatEventLogger();
    ~CombatEventLogger();

    // Prepare combat/player-stats related statements. Returns true on success.
    bool prepareStatements(CassSession* session);

    // Release prepared statements
    void cleanupPreparedStatements();

    // === Combat Event Logging ===

    // Log a single combat event (async)
    void logCombatEvent(CassSession* session, const CombatEvent& event,
                        WriteCallback callback = nullptr);

    // Batch log multiple combat events (async, unlogged batch)
    void logCombatEventsBatch(CassSession* session,
                              const std::vector<CombatEvent>& events,
                              WriteCallback callback = nullptr);

    // === Player Stats ===

    // Update player combat stats (async, counter increment)
    void updatePlayerStats(CassSession* session, const PlayerCombatStats& stats,
                           WriteCallback callback = nullptr);

    // Get player stats (async query)
    void getPlayerStats(CassSession* session, uint64_t playerId, uint32_t sessionDate,
                        std::function<void(bool success, const PlayerCombatStats& stats)> callback);

    // === Session Management ===

    // Save player session state (async, fire-and-forget)
    void savePlayerState(CassSession* session, uint64_t playerId, uint32_t zoneId,
                         uint64_t timestamp, WriteCallback callback = nullptr);

    // === Analytics Queries ===

    // Get top killers for a zone/time period
    void getTopKillers(CassSession* session, uint32_t zoneId, uint32_t startTime,
                       uint32_t endTime, int limit,
                       std::function<void(bool success, const std::vector<std::pair<uint64_t, uint32_t>>&)> callback);

    // Get kill feed for a zone
    void getKillFeed(CassSession* session, uint32_t zoneId, int limit,
                     std::function<void(bool success, const std::vector<CombatEvent>&)> callback);

    // Check if prepared statements are ready
    [[nodiscard]] bool isReady() const;

    // Metrics
    [[nodiscard]] uint64_t getWritesQueued() const { return writesQueued_; }
    [[nodiscard]] uint64_t getWritesCompleted() const { return writesCompleted_; }
    [[nodiscard]] uint64_t getWritesFailed() const { return writesFailed_; }

private:
    // Prepared statements
    const CassPrepared* insertCombatEvent_{nullptr};
    const CassPrepared* updatePlayerStats_{nullptr};
    const CassPrepared* insertPlayerSession_{nullptr};
    const CassPrepared* queryPlayerStats_{nullptr};
    const CassPrepared* queryKillFeed_{nullptr};

    // Metrics
    uint64_t writesQueued_{0};
    uint64_t writesCompleted_{0};
    uint64_t writesFailed_{0};
};

} // namespace DarkAges
