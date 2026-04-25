#include "ServerStateExporter.hpp"
#include <filesystem>
#include <iomanip>

namespace DarkAges::Instrumentation {

    using namespace std::chrono;

    ServerStateExporter::ServerStateExporter(const std::string& outputDir)
        : outputDir_(outputDir), ticksPerExport_(6), tickCounter_(0), enabled_(false)
    {
        std::filesystem::create_directories(outputDir_);
    }

    ServerStateExporter::~ServerStateExporter() = default;

    void ServerStateExporter::maybeExport(Registry& registry, uint32_t currentTick, uint32_t serverTickMs)
    {
        if (!enabled_) return;

        tickCounter_++;
        if (tickCounter_ < ticksPerExport_) return;
        tickCounter_ = 0;

        WorldSnapshot snap;
        snap.tick = currentTick;
        snap.timestampMs = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        snap.serverTick = serverTickMs; // actually server tick number? We'll use currentTick
        snap.playerCount = 0;
        snap.npcCount = 0;

        // Enumerate entities with Position and Velocity components
        auto view = registry.view<Position, Velocity>();
        for (auto entity : view)
        {
            const auto& pos = view.get<Position>(entity);
            const auto& vel = view.get<Velocity>(entity);

            // Optional Rotation
            float yaw = 0.0f, pitch = 0.0f;
            if (auto rot = registry.try_get<Rotation>(entity)) {
                yaw = rot->yaw;
                pitch = rot->pitch;
            }

            // Optional CombatState for health
            uint8_t healthPct = 0;
            if (auto combat = registry.try_get<CombatState>(entity)) {
                if (combat->maxHealth > 0) {
                    healthPct = static_cast<uint8_t>((combat->health * 100) / combat->maxHealth);
                }
            }

            // Entity type detection via tags
            uint8_t type = 0;
            if (registry.all_of<PlayerTag>(entity)) {
                type = 1;
                snap.playerCount++;
            } else if (registry.all_of<NPCTag>(entity)) {
                type = 2;
                snap.npcCount++;
            }

            // State encoding: placeholder (0). Future: derive from NPCAIState::behavior or InputState bits.
            uint8_t state = 0;

            EntityRecord rec;
            rec.entityId = static_cast<uint32_t>(entity);
            rec.type = type;
            rec.position[0] = static_cast<float>(pos.x * DarkAges::Constants::FIXED_TO_FLOAT);
            rec.position[1] = static_cast<float>(pos.y * DarkAges::Constants::FIXED_TO_FLOAT);
            rec.position[2] = static_cast<float>(pos.z * DarkAges::Constants::FIXED_TO_FLOAT);
            rec.velocity[0] = static_cast<float>(vel.dx * DarkAges::Constants::FIXED_TO_FLOAT);
            rec.velocity[1] = static_cast<float>(vel.dy * DarkAges::Constants::FIXED_TO_FLOAT);
            rec.velocity[2] = static_cast<float>(vel.dz * DarkAges::Constants::FIXED_TO_FLOAT);
            rec.yaw = yaw;
            rec.pitch = pitch;
            rec.healthPercent = healthPct;
            rec.state = state;

            snap.entities.push_back(rec);
        }

        snap.entityCount = static_cast<uint32_t>(snap.entities.size());

        writeSnapshot(snap);
    }

    void ServerStateExporter::writeSnapshot(const WorldSnapshot& snap)
    {
        using json = nlohmann::json;
        json j;
        j["tick"] = snap.tick;
        j["timestamp_ms"] = snap.timestampMs;
        j["server_tick"] = snap.serverTick;
        j["entity_count"] = snap.entityCount;
        j["player_count"] = snap.playerCount;
        j["npc_count"] = snap.npcCount;

        json entities = json::array();
        for (const auto& e : snap.entities)
        {
            json je;
            je["entity_id"] = e.entityId;
            je["type"] = e.type;
            je["position"] = { e.position[0], e.position[1], e.position[2] };
            je["velocity"] = { e.velocity[0], e.velocity[1], e.velocity[2] };
            je["yaw"] = e.yaw;
            je["pitch"] = e.pitch;
            je["health"] = e.healthPercent;
            je["state"] = e.state;
            entities.push_back(je);
        }
        j["entities"] = entities;

        // Write to file
        auto now = steady_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
        std::string filename = outputDir_ + "/server_" + std::to_string(snap.tick) + ".json";
        std::ofstream ofs(filename);
        if (ofs.is_open())
        {
            ofs << j.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
            ofs.close();
        }
    }

} // namespace
