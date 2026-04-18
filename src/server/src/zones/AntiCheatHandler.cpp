// Anti-cheat handler implementation
// Extracted from ZoneServer.cpp to improve code organization

#include "zones/AntiCheatHandler.hpp"
#include "zones/ZoneServer.hpp"
#include "netcode/NetworkManager.hpp"
#include "db/RedisManager.hpp"
#include "db/ScyllaManager.hpp"
#include "monitoring/MetricsExporter.hpp"
#include <iostream>
#include <string>

namespace DarkAges {

AntiCheatHandler::AntiCheatHandler(ZoneServer& server)
    : server_(server) {
}

void AntiCheatHandler::setConnectionMappings(
    std::unordered_map<ConnectionID, EntityID>* connToEntity,
    std::unordered_map<EntityID, ConnectionID>* entityToConn) {
    connectionToEntity_ = connToEntity;
    entityToConnection_ = entityToConn;
}

void AntiCheatHandler::initialize() {
    if (!antiCheat_) {
        std::cerr << "[ANTICHEAT] AntiCheatSystem pointer not set!" << std::endl;
        return;
    }

    const auto& config = server_.getConfig();

    if (!antiCheat_->initialize()) {
        std::cerr << "[ZONE " << config.zoneId << "] Failed to initialize anti-cheat system!" << std::endl;
        return;
    }

    // Set spatial hash for no-clip collision detection
    antiCheat_->setSpatialHash(&server_.getSpatialHash());

    // Set up cheat detection callback
    antiCheat_->setOnCheatDetected([this](uint64_t playerId,
                                          const Security::CheatDetectionResult& result) {
        onCheatDetected(playerId, result);
    });

    // Set up ban callback
    antiCheat_->setOnPlayerBanned([this](uint64_t playerId, const char* reason,
                                         uint32_t durationMinutes) {
        onPlayerBanned(playerId, reason, durationMinutes);
    });

    // Set up kick callback
    antiCheat_->setOnPlayerKicked([this](uint64_t playerId, const char* reason) {
        onPlayerKicked(playerId, reason);
    });

    std::cout << "[ZONE " << config.zoneId << "] Anti-cheat system initialized" << std::endl;
}

void AntiCheatHandler::onCheatDetected(uint64_t playerId, const Security::CheatDetectionResult& result) {
    const auto& config = server_.getConfig();
    auto& registry = server_.getRegistry();
    uint32_t currentTick = server_.getCurrentTick();

    // [DEVOPS_AGENT] Track anti-cheat violations in Prometheus
    auto& metrics = Monitoring::MetricsExporter::Instance();
    std::string zoneIdStr = std::to_string(config.zoneId);
    const char* cheatTypeStr = Security::cheatTypeToString(result.type);
    const char* severityStr = result.severity == Security::ViolationSeverity::CRITICAL ? "critical" :
                              result.severity == Security::ViolationSeverity::SUSPICIOUS ? "suspicious" : "minor";
    std::unordered_map<std::string, std::string> violationLabels = {
        {"zone_id", zoneIdStr},
        {"cheat_type", cheatTypeStr},
        {"severity", severityStr}
    };
    metrics.AntiCheatViolationsTotal().Increment(1.0, violationLabels);

    // Log to ScyllaDB for analytics and review
    if (scylla_ && scylla_->isConnected()) {
        AntiCheatEvent event;
        event.eventId = static_cast<uint64_t>(playerId) * 100000 + currentTick;
        event.timestamp = currentTick * 16 / 1000;  // Convert ticks to seconds
        event.zoneId = config.zoneId;
        event.playerId = playerId;
        event.cheatType = cheatTypeStr;
        event.severity = severityStr;
        event.description = result.description;
        event.confidence = result.confidence;
        event.serverTick = currentTick;
        event.position = {0, 0, 0};

        if (connectionToEntity_) {
            for (const auto& [connId, entityId] : *connectionToEntity_) {
                if (const PlayerInfo* info = registry.try_get<PlayerInfo>(entityId)) {
                    if (info->playerId == playerId) {
                        if (const Position* pos = registry.try_get<Position>(entityId)) {
                            event.position = *pos;
                        }
                        break;
                    }
                }
            }
        }

        scylla_->logAntiCheatEvent(event, nullptr);
    } else if (scylla_) {
        std::cerr << "[ANTICHEAT] ScyllaDB not connected, cannot log violation for player " << playerId << std::endl;
    }

    // Could also notify monitoring systems, Discord webhooks, etc.
    if (result.severity == Security::ViolationSeverity::SUSPICIOUS) {
        // Flag for admin review but don't take action yet
        std::cout << "[ANTICHEAT] Player " << playerId << " flagged for review: "
                  << result.description << std::endl;
    }
}

void AntiCheatHandler::onPlayerBanned(uint64_t playerId, const char* reason, uint32_t durationMinutes) {
    auto& registry = server_.getRegistry();

    // Persist ban to Redis if connected
    if (redis_ && redis_->isConnected()) {
        std::string key = "ban:" + std::to_string(playerId);
        std::string value = std::string(reason) + "|" + std::to_string(durationMinutes);
        redis_->set(key, value, durationMinutes * 60, nullptr);
    }

    // Kick any connected sessions for this player
    if (connectionToEntity_) {
        for (const auto& [connId, entityId] : *connectionToEntity_) {
            if (const PlayerInfo* info = registry.try_get<PlayerInfo>(entityId)) {
                if (info->playerId == playerId) {
                    if (network_) {
                        network_->disconnect(connId, reason);
                    }
                    break;
                }
            }
        }
    }

    // Log ban event
    std::cout << "[ANTICHEAT] Player " << playerId << " banned for "
              << durationMinutes << " minutes: " << reason << std::endl;
}

void AntiCheatHandler::onPlayerKicked(uint64_t playerId, const char* reason) {
    auto& registry = server_.getRegistry();

    // Find and kick the player
    if (connectionToEntity_) {
        for (const auto& [connId, entityId] : *connectionToEntity_) {
            if (const PlayerInfo* info = registry.try_get<PlayerInfo>(entityId)) {
                if (info->playerId == playerId) {
                    if (network_) {
                        network_->disconnect(connId, reason);
                    }
                    break;
                }
            }
        }
    }

    // Log kick event
    std::cout << "[ANTICHEAT] Player " << playerId << " kicked: " << reason << std::endl;
}

} // namespace DarkAges
