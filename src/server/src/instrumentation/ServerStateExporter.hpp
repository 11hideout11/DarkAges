#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>

#include "ecs/CoreTypes.hpp"

namespace DarkAges::Instrumentation {

    struct EntityRecord {
        uint32_t entityId;
        uint8_t type;  // 0=unknown, 1=player, 2=npc
        float position[3];   // from Position (Fixed -> float)
        float velocity[3];   // from Velocity (Fixed -> float)
        float yaw;           // from Rotation.yaw (radians)
        float pitch;         // from Rotation.pitch (radians)
        uint8_t healthPercent;
        uint8_t state;       // 0=unknown, future: NPCAIState::behavior or InputState bits
    };

    struct WorldSnapshot {
        uint32_t tick;
        uint64_t timestampMs;
        uint32_t serverTick; // server's logical tick
        uint32_t entityCount;
        std::vector<EntityRecord> entities;
        // Add zone metrics: player count, npc count, etc.
        uint32_t playerCount;
        uint32_t npcCount;
    };

    class ServerStateExporter {
    public:
        explicit ServerStateExporter(const std::string& outputDir);
        ~ServerStateExporter();

        // Call once per server tick; will export if interval elapsed
        void maybeExport(Registry& registry, uint32_t currentTick, uint32_t serverTickMs);

        // Set export frequency (ticks per export, e.g., 6 = 10 Hz at 60 Hz)
        void setExportInterval(uint32_t ticksPerExport) { ticksPerExport_ = ticksPerExport; }

        // Enable/disable
        void setEnabled(bool e) { enabled_ = e; }
        [[nodiscard]] bool isEnabled() const { return enabled_; }

    private:
        std::string outputDir_;
        uint32_t ticksPerExport_ = 6; // default: export every 6 ticks (10 Hz)
        uint32_t tickCounter_ = 0;
        bool enabled_ = false;

        void writeSnapshot(const WorldSnapshot& snap);
    };

} // namespace DarkAges::Instrumentation
