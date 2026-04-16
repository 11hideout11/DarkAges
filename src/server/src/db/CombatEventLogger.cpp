// CombatEventLogger - Combat event logging and player stats subsystem
// Extracted from ScyllaManager for cohesion

#include "db/CombatEventLogger.hpp"
#include "db/ScyllaManager.hpp"  // For CombatEvent, PlayerCombatStats structs
#include "Constants.hpp"
#include <cassandra.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <memory>

namespace DarkAges {

// ============================================================================
// Utility Functions
// ============================================================================

namespace {
    // Convert timestamp to day bucket string (YYYY-MM-DD format)
    std::string getDayBucket(uint32_t timestamp) {
        auto time = static_cast<std::time_t>(timestamp);
        std::tm tm = *std::gmtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        return oss.str();
    }

    // Get current timestamp in seconds
    uint32_t getCurrentTimestamp() {
        return static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }

    // Async callback data structure for write operations
    struct WriteCallbackData {
        CombatEventLogger::WriteCallback callback;
        CombatEventLogger* logger;

        WriteCallbackData(CombatEventLogger::WriteCallback cb, CombatEventLogger* mgr)
            : callback(std::move(cb)), logger(mgr) {}
    };

    // Async callback data for query operations
    template<typename T>
    struct QueryCallbackData {
        T callback;
        CombatEventLogger* logger;

        QueryCallbackData(T cb, CombatEventLogger* mgr)
            : callback(std::move(cb)), logger(mgr) {}
    };
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

CombatEventLogger::CombatEventLogger() = default;

CombatEventLogger::~CombatEventLogger() {
    cleanupPreparedStatements();
}

// ============================================================================
// Statement Preparation
// ============================================================================

bool CombatEventLogger::prepareStatements(CassSession* session) {
    if (!session) {
        return false;
    }

    // Prepare INSERT for combat events
    const char* insertCombat =
        "INSERT INTO darkages.combat_events "
        "(day_bucket, timestamp, event_id, attacker_id, target_id, damage, "
        " hit_type, event_type, zone_id, position_x, position_y, position_z) "
        "VALUES (?, ?, uuid(), ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    CassFuture* future = cass_session_prepare(session, insertCombat);
    cass_future_wait(future);
    CassError rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        cass_future_free(future);
        return false;
    }
    insertCombatEvent_ = cass_future_get_prepared(future);
    cass_future_free(future);

    // Prepare UPDATE for player stats (counters)
    const char* updateStats =
        "UPDATE darkages.player_stats SET "
        "total_kills = total_kills + ?, "
        "total_deaths = total_deaths + ?, "
        "total_assists = total_assists + ?, "
        "total_damage_dealt = total_damage_dealt + ?, "
        "total_damage_taken = total_damage_taken + ?, "
        "total_damage_blocked = total_damage_blocked + ?, "
        "total_healing_done = total_healing_done + ?, "
        "total_playtime_minutes = total_playtime_minutes + ?, "
        "total_matches = total_matches + ?, "
        "total_wins = total_wins + ?, "
        "last_updated = toTimestamp(now()) "
        "WHERE player_id = ?";

    future = cass_session_prepare(session, updateStats);
    cass_future_wait(future);
    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        cass_future_free(future);
        return false;
    }
    updatePlayerStats_ = cass_future_get_prepared(future);
    cass_future_free(future);

    // Prepare INSERT for player sessions
    const char* insertSession =
        "INSERT INTO darkages.player_sessions "
        "(player_id, session_start, session_end, zone_id, kills, deaths, "
        " damage_dealt, damage_taken, playtime_minutes) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

    future = cass_session_prepare(session, insertSession);
    cass_future_wait(future);
    rc = cass_future_error_code(future);
    if (rc != CASS_OK) {
        cass_future_free(future);
        return false;
    }
    insertPlayerSession_ = cass_future_get_prepared(future);
    cass_future_free(future);

    // Prepare query for player stats
    const char* queryStats =
        "SELECT total_kills, total_deaths, total_damage_dealt, total_damage_taken, "
        "total_playtime_minutes FROM darkages.player_stats WHERE player_id = ?";

    future = cass_session_prepare(session, queryStats);
    cass_future_wait(future);
    if (cass_future_error_code(future) == CASS_OK) {
        queryPlayerStats_ = cass_future_get_prepared(future);
    }
    cass_future_free(future);

    // Prepare query for kill feed (events where target died)
    const char* queryKillFeedStmt =
        "SELECT timestamp, attacker_id, target_id, event_type, zone_id "
        "FROM darkages.combat_events "
        "WHERE zone_id = ? AND day_bucket = ? AND event_type = 'kill' "
        "ALLOW FILTERING";

    future = cass_session_prepare(session, queryKillFeedStmt);
    cass_future_wait(future);
    if (cass_future_error_code(future) == CASS_OK) {
        queryKillFeed_ = cass_future_get_prepared(future);
    }
    cass_future_free(future);

    return true;
}

void CombatEventLogger::cleanupPreparedStatements() {
    if (insertCombatEvent_) {
        cass_prepared_free(insertCombatEvent_);
        insertCombatEvent_ = nullptr;
    }
    if (updatePlayerStats_) {
        cass_prepared_free(updatePlayerStats_);
        updatePlayerStats_ = nullptr;
    }
    if (insertPlayerSession_) {
        cass_prepared_free(insertPlayerSession_);
        insertPlayerSession_ = nullptr;
    }
    if (queryPlayerStats_) {
        cass_prepared_free(queryPlayerStats_);
        queryPlayerStats_ = nullptr;
    }
    if (queryKillFeed_) {
        cass_prepared_free(queryKillFeed_);
        queryKillFeed_ = nullptr;
    }
}

bool CombatEventLogger::isReady() const {
    return insertCombatEvent_ != nullptr &&
           updatePlayerStats_ != nullptr &&
           insertPlayerSession_ != nullptr;
}

// ============================================================================
// Combat Event Logging
// ============================================================================

void CombatEventLogger::logCombatEvent(CassSession* session,
                                        const CombatEvent& event,
                                        WriteCallback callback) {
    if (!session || !insertCombatEvent_) {
        if (callback) {
            callback(false);
        }
        writesFailed_++;
        return;
    }

    writesQueued_++;

    // Bind prepared statement
    CassStatement* statement = cass_prepared_bind(insertCombatEvent_);

    // Get day bucket from timestamp
    std::string dayBucket = getDayBucket(event.timestamp);

    // Bind parameters
    cass_statement_bind_string(statement, 0, dayBucket.c_str());
    cass_statement_bind_int64(statement, 1, static_cast<cass_int64_t>(event.timestamp) * 1000);
    cass_statement_bind_int64(statement, 2, static_cast<cass_int64_t>(event.attackerId));
    cass_statement_bind_int64(statement, 3, static_cast<cass_int64_t>(event.targetId));
    cass_statement_bind_int32(statement, 4, event.damageAmount);
    cass_statement_bind_string(statement, 5, event.eventType.c_str());
    cass_statement_bind_string(statement, 6, event.eventType.c_str());
    cass_statement_bind_int32(statement, 7, static_cast<cass_int32_t>(event.zoneId));
    cass_statement_bind_double(statement, 8, event.position.x * Constants::FIXED_TO_FLOAT);
    cass_statement_bind_double(statement, 9, event.position.y * Constants::FIXED_TO_FLOAT);
    cass_statement_bind_double(statement, 10, event.position.z * Constants::FIXED_TO_FLOAT);

    // Execute async
    CassFuture* future = cass_session_execute(session, statement);

    // Set up callback
    auto cbData = std::make_unique<WriteCallbackData>(callback, this);
    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        std::unique_ptr<WriteCallbackData> cbData(static_cast<WriteCallbackData*>(data));
        CassError rc = cass_future_error_code(f);
        bool success = (rc == CASS_OK);

        if (cbData->logger) {
            if (success) {
                cbData->logger->writesCompleted_++;
            } else {
                cbData->logger->writesFailed_++;
            }
        }

        if (cbData->callback) {
            cbData->callback(success);
        }
    }, cbData.release());

    cass_future_free(future);
    cass_statement_free(statement);
}

void CombatEventLogger::logCombatEventsBatch(CassSession* session,
                                              const std::vector<CombatEvent>& events,
                                              WriteCallback callback) {
    if (!session || events.empty()) {
        if (callback) {
            callback(session != nullptr);
        }
        return;
    }

    writesQueued_ += events.size();

    // Create unlogged batch for high-performance writes
    CassBatch* batch = cass_batch_new(CASS_BATCH_TYPE_UNLOGGED);

    // Set batch timeout
    cass_batch_set_timeout(batch, 5000); // 5 seconds

    for (const auto& event : events) {
        CassStatement* statement = cass_prepared_bind(insertCombatEvent_);

        std::string dayBucket = getDayBucket(event.timestamp);

        cass_statement_bind_string(statement, 0, dayBucket.c_str());
        cass_statement_bind_int64(statement, 1, static_cast<cass_int64_t>(event.timestamp) * 1000);
        cass_statement_bind_int64(statement, 2, static_cast<cass_int64_t>(event.attackerId));
        cass_statement_bind_int64(statement, 3, static_cast<cass_int64_t>(event.targetId));
        cass_statement_bind_int32(statement, 4, event.damageAmount);
        cass_statement_bind_string(statement, 5, event.eventType.c_str());
        cass_statement_bind_string(statement, 6, event.eventType.c_str());
        cass_statement_bind_int32(statement, 7, static_cast<cass_int32_t>(event.zoneId));
        cass_statement_bind_double(statement, 8, event.position.x * Constants::FIXED_TO_FLOAT);
        cass_statement_bind_double(statement, 9, event.position.y * Constants::FIXED_TO_FLOAT);
        cass_statement_bind_double(statement, 10, event.position.z * Constants::FIXED_TO_FLOAT);

        cass_batch_add_statement(batch, statement);
        cass_statement_free(statement);
    }

    // Execute batch
    CassFuture* future = cass_session_execute_batch(session, batch);

    auto cbData = std::make_unique<WriteCallbackData>(callback, this);
    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        std::unique_ptr<WriteCallbackData> cbData(static_cast<WriteCallbackData*>(data));
        CassError rc = cass_future_error_code(f);
        bool success = (rc == CASS_OK);

        if (cbData->logger) {
            if (success) {
                cbData->logger->writesCompleted_++;
            } else {
                cbData->logger->writesFailed_++;
            }
        }

        if (cbData->callback) {
            cbData->callback(success);
        }
    }, cbData.release());

    cass_future_free(future);
    cass_batch_free(batch);
}

// ============================================================================
// Player Stats
// ============================================================================

void CombatEventLogger::updatePlayerStats(CassSession* session,
                                           const PlayerCombatStats& stats,
                                           WriteCallback callback) {
    if (!session || !updatePlayerStats_) {
        if (callback) {
            callback(false);
        }
        writesFailed_++;
        return;
    }

    writesQueued_++;

    CassStatement* statement = cass_prepared_bind(updatePlayerStats_);

    // Bind counter increments
    cass_statement_bind_int64(statement, 0, static_cast<cass_int64_t>(stats.kills));
    cass_statement_bind_int64(statement, 1, static_cast<cass_int64_t>(stats.deaths));
    cass_statement_bind_int64(statement, 2, 0); // assists (not in current struct)
    cass_statement_bind_int64(statement, 3, static_cast<cass_int64_t>(stats.damageDealt));
    cass_statement_bind_int64(statement, 4, static_cast<cass_int64_t>(stats.damageTaken));
    cass_statement_bind_int64(statement, 5, 0); // damage_blocked
    cass_statement_bind_int64(statement, 6, 0); // healing_done
    cass_statement_bind_int64(statement, 7, 0); // playtime_minutes
    cass_statement_bind_int64(statement, 8, 0); // matches
    cass_statement_bind_int64(statement, 9, 0); // wins
    cass_statement_bind_int64(statement, 10, static_cast<cass_int64_t>(stats.playerId));

    CassFuture* future = cass_session_execute(session, statement);

    auto cbData = std::make_unique<WriteCallbackData>(callback, this);
    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        std::unique_ptr<WriteCallbackData> cbData(static_cast<WriteCallbackData*>(data));
        CassError rc = cass_future_error_code(f);
        bool success = (rc == CASS_OK);

        if (cbData->logger) {
            if (success) {
                cbData->logger->writesCompleted_++;
            } else {
                cbData->logger->writesFailed_++;
            }
        }

        if (cbData->callback) {
            cbData->callback(success);
        }
    }, cbData.release());

    cass_future_free(future);
    cass_statement_free(statement);
}

void CombatEventLogger::getPlayerStats(CassSession* session, uint64_t playerId,
                                        uint32_t sessionDate,
                                        std::function<void(bool success, const PlayerCombatStats& stats)> callback) {

    if (!session || !queryPlayerStats_) {
        if (callback) {
            callback(false, PlayerCombatStats{});
        }
        return;
    }

    CassStatement* statement = cass_prepared_bind(queryPlayerStats_);
    cass_statement_bind_int64(statement, 0, static_cast<cass_int64_t>(playerId));

    CassFuture* future = cass_session_execute(session, statement);

    using CallbackType = std::function<void(bool, const PlayerCombatStats&)>;
    auto* cbData = new QueryCallbackData<CallbackType>(callback, this);

    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        auto* cbData = static_cast<QueryCallbackData<CallbackType>*>(data);
        CassError rc = cass_future_error_code(f);

        PlayerCombatStats stats{};
        bool success = false;

        if (rc == CASS_OK) {
            const CassResult* result = cass_future_get_result(f);
            CassIterator* iterator = cass_iterator_from_result(result);

            if (cass_iterator_next(iterator)) {
                const CassRow* row = cass_iterator_get_row(iterator);

                cass_int64_t kills, deaths, damageDealt, damageTaken, playtime;

                cass_value_get_int64(cass_row_get_column(row, 0), &kills);
                cass_value_get_int64(cass_row_get_column(row, 1), &deaths);
                cass_value_get_int64(cass_row_get_column(row, 2), &damageDealt);
                cass_value_get_int64(cass_row_get_column(row, 3), &damageTaken);
                cass_value_get_int64(cass_row_get_column(row, 4), &playtime);

                stats.playerId = 0; // Will be set from query context
                stats.kills = static_cast<uint32_t>(kills);
                stats.deaths = static_cast<uint32_t>(deaths);
                stats.damageDealt = static_cast<uint64_t>(damageDealt);
                stats.damageTaken = static_cast<uint64_t>(damageTaken);
                success = true;
            }

            cass_iterator_free(iterator);
            cass_result_free(result);
        }

        if (cbData->callback) {
            cbData->callback(success, stats);
        }
        delete cbData;
    }, cbData);

    cass_future_free(future);
    cass_statement_free(statement);
}

// ============================================================================
// Session Management
// ============================================================================

void CombatEventLogger::savePlayerState(CassSession* session, uint64_t playerId,
                                         uint32_t zoneId, uint64_t timestamp,
                                         WriteCallback callback) {
    if (!session || !insertPlayerSession_) {
        if (callback) {
            callback(false);
        }
        writesFailed_++;
        return;
    }

    writesQueued_++;

    // Bind prepared statement
    CassStatement* statement = cass_prepared_bind(insertPlayerSession_);

    // Bind parameters: player_id, session_start, session_end, zone_id, kills, deaths, damage_dealt, damage_taken, playtime_minutes
    cass_statement_bind_int64(statement, 0, static_cast<cass_int64_t>(playerId));
    cass_statement_bind_int64(statement, 1, static_cast<cass_int64_t>(timestamp));
    cass_statement_bind_int64(statement, 2, 0);  // session_end (0 = active)
    cass_statement_bind_int32(statement, 3, static_cast<cass_int32_t>(zoneId));
    cass_statement_bind_int32(statement, 4, 0);  // kills
    cass_statement_bind_int32(statement, 5, 0);  // deaths
    cass_statement_bind_int64(statement, 6, 0);  // damage_dealt
    cass_statement_bind_int64(statement, 7, 0);  // damage_taken
    cass_statement_bind_int32(statement, 8, 0);  // playtime_minutes

    // Execute async
    CassFuture* future = cass_session_execute(session, statement);

    // Set up callback
    auto* cbData = new WriteCallbackData(callback, this);
    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        auto* cbData = static_cast<WriteCallbackData*>(data);
        CassError rc = cass_future_error_code(f);
        bool success = (rc == CASS_OK);

        if (cbData->logger) {
            if (success) {
                cbData->logger->writesCompleted_++;
            } else {
                cbData->logger->writesFailed_++;
            }
        }

        if (cbData->callback) {
            cbData->callback(success);
        }
        delete cbData;
    }, cbData);

    cass_future_free(future);
    cass_statement_free(statement);
}

// ============================================================================
// Analytics Queries
// ============================================================================

void CombatEventLogger::getTopKillers(CassSession* session, uint32_t zoneId,
                                       uint32_t startTime, uint32_t endTime,
                                       int limit,
    std::function<void(bool success, const std::vector<std::pair<uint64_t, uint32_t>>&)> callback) {

    if (!session) {
        if (callback) {
            callback(false, {});
        }
        return;
    }

    // Query all combat events and aggregate (can be optimized with materialized view)
    const char* query =
        "SELECT attacker_id, count(*) as kill_count "
        "FROM darkages.combat_events "
        "WHERE day_bucket = ? AND event_type = 'kill' "
        "ALLOW FILTERING";

    CassStatement* statement = cass_statement_new(query, 1);
    std::string dayBucket = getDayBucket(startTime);
    cass_statement_bind_string(statement, 0, dayBucket.c_str());

    CassFuture* future = cass_session_execute(session, statement);

    using ResultType = std::vector<std::pair<uint64_t, uint32_t>>;
    using CallbackType = std::function<void(bool, const ResultType&)>;
    auto* cbData = new QueryCallbackData<CallbackType>(callback, this);

    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        auto* cbData = static_cast<QueryCallbackData<CallbackType>*>(data);
        CassError rc = cass_future_error_code(f);

        ResultType results;
        bool success = false;

        if (rc == CASS_OK) {
            const CassResult* result = cass_future_get_result(f);
            CassIterator* iterator = cass_iterator_from_result(result);

            while (cass_iterator_next(iterator)) {
                const CassRow* row = cass_iterator_get_row(iterator);
                cass_int64_t attackerId;
                cass_int64_t killCount;

                cass_value_get_int64(cass_row_get_column(row, 0), &attackerId);
                cass_value_get_int64(cass_row_get_column(row, 1), &killCount);

                results.emplace_back(static_cast<uint64_t>(attackerId),
                                    static_cast<uint32_t>(killCount));
            }

            cass_iterator_free(iterator);
            cass_result_free(result);
            success = true;
        }

        if (cbData->callback) {
            cbData->callback(success, results);
        }
        delete cbData;
    }, cbData);

    cass_future_free(future);
    cass_statement_free(statement);
}

void CombatEventLogger::getKillFeed(CassSession* session, uint32_t zoneId,
                                     int limit,
    std::function<void(bool success, const std::vector<CombatEvent>&)> callback) {

    if (!session) {
        if (callback) {
            callback(false, {});
        }
        return;
    }

    // Query for kill events in the zone today
    const char* query =
        "SELECT timestamp, attacker_id, target_id, event_type, zone_id "
        "FROM darkages.combat_events "
        "WHERE day_bucket = ? AND zone_id = ? AND event_type = 'kill' "
        "ALLOW FILTERING";

    CassStatement* statement = cass_statement_new(query, 2);
    std::string dayBucket = getDayBucket(getCurrentTimestamp());
    cass_statement_bind_string(statement, 0, dayBucket.c_str());
    cass_statement_bind_int32(statement, 1, static_cast<cass_int32_t>(zoneId));
    cass_statement_set_page_size(statement, limit);

    CassFuture* future = cass_session_execute(session, statement);

    using CallbackType = std::function<void(bool, const std::vector<CombatEvent>&)>;
    auto* cbData = new QueryCallbackData<CallbackType>(callback, this);

    cass_future_set_callback(future, [](CassFuture* f, void* data) {
        auto* cbData = static_cast<QueryCallbackData<CallbackType>*>(data);
        CassError rc = cass_future_error_code(f);

        std::vector<CombatEvent> events;
        bool success = false;

        if (rc == CASS_OK) {
            const CassResult* result = cass_future_get_result(f);
            CassIterator* iterator = cass_iterator_from_result(result);

            while (cass_iterator_next(iterator)) {
                const CassRow* row = cass_iterator_get_row(iterator);
                CombatEvent event{};

                cass_int64_t timestamp;
                cass_int64_t attackerId;
                cass_int64_t targetId;
                const char* eventType;
                size_t eventTypeLen;
                cass_int32_t zoneId;

                cass_value_get_int64(cass_row_get_column(row, 0), &timestamp);
                cass_value_get_int64(cass_row_get_column(row, 1), &attackerId);
                cass_value_get_int64(cass_row_get_column(row, 2), &targetId);
                cass_value_get_string(cass_row_get_column(row, 3), &eventType, &eventTypeLen);
                cass_value_get_int32(cass_row_get_column(row, 4), &zoneId);

                event.timestamp = static_cast<uint32_t>(timestamp / 1000);
                event.attackerId = static_cast<uint64_t>(attackerId);
                event.targetId = static_cast<uint64_t>(targetId);
                event.eventType = std::string(eventType, eventTypeLen);
                event.zoneId = static_cast<uint32_t>(zoneId);

                events.push_back(event);
            }

            cass_iterator_free(iterator);
            cass_result_free(result);
            success = true;
        }

        if (cbData->callback) {
            cbData->callback(success, events);
        }
        delete cbData;
    }, cbData);

    cass_future_free(future);
    cass_statement_free(statement);
}

} // namespace DarkAges
