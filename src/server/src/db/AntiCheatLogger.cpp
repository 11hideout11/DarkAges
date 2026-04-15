// AntiCheatLogger - Anti-cheat event logging subsystem
// Extracted from ScyllaManager for cohesion

#include "db/AntiCheatLogger.hpp"
#include "db/ScyllaManager.hpp"  // For AntiCheatEvent struct
#include "Constants.hpp"
#include <cassandra.h>
#include <chrono>
#include <iomanip>
#include <sstream>

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

    // Async callback data structure for write operations
    struct WriteCallbackData {
        AntiCheatLogger::WriteCallback callback;
        AntiCheatLogger* logger;

        WriteCallbackData(AntiCheatLogger::WriteCallback cb, AntiCheatLogger* mgr)
            : callback(std::move(cb)), logger(mgr) {}
    };
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

AntiCheatLogger::AntiCheatLogger() = default;
AntiCheatLogger::~AntiCheatLogger() = default;

// ============================================================================
// Anti-Cheat Event Logging
// ============================================================================

void AntiCheatLogger::logAntiCheatEvent(CassSession* session,
                                         const AntiCheatEvent& event,
                                         WriteCallback callback) {
    if (!session) {
        writesFailed_++;
        if (callback) {
            callback(false);
        }
        return;
    }

    writesQueued_++;

    std::string dayBucket = getDayBucket(event.timestamp);

    std::string query =
        "INSERT INTO darkages.anticheat_events "
        "(day_bucket, timestamp, event_id, player_id, zone_id, cheat_type, "
        " severity, description, confidence, position_x, position_y, position_z, server_tick) "
        "VALUES ('" + dayBucket + "', " +
        std::to_string(static_cast<cass_int64_t>(event.timestamp) * 1000) + ", uuid(), " +
        std::to_string(event.playerId) + ", " +
        std::to_string(event.zoneId) + ", '" +
        event.cheatType + "', '" +
        event.severity + "', '" +
        event.description + "', " +
        std::to_string(event.confidence) + ", " +
        std::to_string(event.position.x * Constants::FIXED_TO_FLOAT) + ", " +
        std::to_string(event.position.y * Constants::FIXED_TO_FLOAT) + ", " +
        std::to_string(event.position.z * Constants::FIXED_TO_FLOAT) + ", " +
        std::to_string(event.serverTick) + ")";

    CassStatement* statement = cass_statement_new(query.c_str(), 0);
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

void AntiCheatLogger::logAntiCheatEventsBatch(CassSession* session,
                                               const std::vector<AntiCheatEvent>& events,
                                               WriteCallback callback) {
    if (!session || events.empty()) {
        if (callback) {
            callback(false);
        }
        return;
    }

    writesQueued_ += events.size();

    std::string query = "BEGIN BATCH ";
    for (const auto& event : events) {
        std::string dayBucket = getDayBucket(event.timestamp);
        query +=
            "INSERT INTO darkages.anticheat_events "
            "(day_bucket, timestamp, event_id, player_id, zone_id, cheat_type, "
            " severity, description, confidence, position_x, position_y, position_z, server_tick) "
            "VALUES ('" + dayBucket + "', " +
            std::to_string(static_cast<cass_int64_t>(event.timestamp) * 1000) + ", uuid(), " +
            std::to_string(event.playerId) + ", " +
            std::to_string(event.zoneId) + ", '" +
            event.cheatType + "', '" +
            event.severity + "', '" +
            event.description + "', " +
            std::to_string(event.confidence) + ", " +
            std::to_string(event.position.x * Constants::FIXED_TO_FLOAT) + ", " +
            std::to_string(event.position.y * Constants::FIXED_TO_FLOAT) + ", " +
            std::to_string(event.position.z * Constants::FIXED_TO_FLOAT) + ", " +
            std::to_string(event.serverTick) + ") ";
    }
    query += "APPLY BATCH";

    CassStatement* statement = cass_statement_new(query.c_str(), 0);
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

} // namespace DarkAges
