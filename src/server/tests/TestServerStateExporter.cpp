// [INSTRUMENTATION] Unit tests for ServerStateExporter
// Tests JSON snapshot export, tick counting, entity enumeration, and field extraction.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/instrumentation/ServerStateExporter.hpp"
#include "ecs/CoreTypes.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <fstream>
#include <filesystem>
#include <cstdlib>

using namespace DarkAges;
using namespace DarkAges::Instrumentation;

namespace {
    std::string tempOutDir() {
        static int counter = 0;
        return "/tmp/server_state_tests_" + std::to_string(++counter);
    }
}

TEST_CASE("ServerStateExporter construction creates output directory", "[instrumentation]") {
    std::string dir = tempOutDir();
    REQUIRE_FALSE(std::filesystem::exists(dir));

    {
        ServerStateExporter exporter(dir);
        REQUIRE(std::filesystem::exists(dir));
    }
    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter default state is disabled", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    REQUIRE(exporter.isEnabled() == false);
    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter setEnabled controls export", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1); // export every tick

    Registry reg;
    reg.create();

    // Tick 1: export immediately (counter starts at 0, increments to 1, >=1 triggers export)
    exporter.maybeExport(reg, 1, 0);
    auto files = std::filesystem::directory_iterator(dir);
    REQUIRE(std::distance(files, std::filesystem::directory_iterator{}) == 1);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter export interval respected", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(3);

    Registry reg;
    reg.create();

    for (int i = 1; i <= 6; ++i) {
        exporter.maybeExport(reg, i, i * 16);
    }
    // Should export at ticks 3 and 6 → 2 files
    auto files = std::filesystem::directory_iterator(dir);
    REQUIRE(std::distance(files, std::filesystem::directory_iterator{}) == 2);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter empty registry produces valid JSON", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    exporter.maybeExport(reg, 42, 1000);

    std::string file = dir + "/server_42.json";
    REQUIRE(std::filesystem::exists(file));

    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;
    REQUIRE(j["tick"] == 42);
    REQUIRE(j["server_tick"] == 1000);
    REQUIRE(j["entity_count"] == 0);
    REQUIRE(j["player_count"] == 0);
    REQUIRE(j["npc_count"] == 0);
    REQUIRE(j["entities"].is_array());
    REQUIRE(j["entities"].size() == 0);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter enumerates entities with both Position and Velocity", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    EntityID e1 = reg.create();
    reg.emplace<Position>(e1, Position::fromVec3(glm::vec3(10.0f, 0.0f, 20.0f), 1000));
    reg.emplace<Velocity>(e1);

    EntityID e2 = reg.create();
    reg.emplace<Position>(e2, Position::fromVec3(glm::vec3(0.0f, 0.0f, 0.0f), 0));
    // No Velocity → skipped

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    REQUIRE(j["entity_count"] == 1);
    REQUIRE(j["entities"][0]["entity_id"] == static_cast<uint32_t>(e1));

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter detects player vs npc vs unknown", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    EntityID p = reg.create();
    reg.emplace<Position>(p, Position{});
    reg.emplace<Velocity>(p);
    reg.emplace<PlayerTag>(p);

    EntityID n = reg.create();
    reg.emplace<Position>(n, Position{});
    reg.emplace<Velocity>(n);
    reg.emplace<NPCTag>(n);

    EntityID u = reg.create();
    reg.emplace<Position>(u, Position{});
    reg.emplace<Velocity>(u);
    // no tag

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    REQUIRE(j["player_count"] == 1);
    REQUIRE(j["npc_count"] == 1);
    REQUIRE(j["entity_count"] == 3);

    auto& entities = j["entities"];
    int type1 = 0, type2 = 0, type0 = 0;
    for (auto& e : entities) {
        if (e["type"] == 1) type1++;
        else if (e["type"] == 2) type2++;
        else type0++;
    }
    REQUIRE(type1 == 1);
    REQUIRE(type2 == 1);
    REQUIRE(type0 == 1);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter converts Fixed position/velocity to float correctly", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    EntityID e = reg.create();
    Position pos = Position::fromVec3(glm::vec3(100.5f, 0.0f, -50.25f), 0);
    reg.emplace<Position>(e, pos);
    Velocity vel;
    vel.dx = 500;   // 0.5 m/s
    vel.dy = 0;
    vel.dz = -250;  // -0.25 m/s
    reg.emplace<Velocity>(e, vel);

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    auto& rec = j["entities"][0];
    REQUIRE(rec["position"][0] == Catch::Approx(100.5f).margin(0.01f));
    REQUIRE(rec["position"][2] == Catch::Approx(-50.25f).margin(0.01f));
    REQUIRE(rec["velocity"][0] == Catch::Approx(0.5f).margin(0.001f));
    REQUIRE(rec["velocity"][2] == Catch::Approx(-0.25f).margin(0.001f));

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter includes Rotation when present", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    EntityID e = reg.create();
    reg.emplace<Position>(e, Position{});
    reg.emplace<Velocity>(e);
    Rotation rot;
    rot.yaw = 1.5708f;
    rot.pitch = 0.5f;
    reg.emplace<Rotation>(e, rot);

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    auto& rec = j["entities"][0];
    REQUIRE(rec["yaw"] == Catch::Approx(1.5708f).margin(0.001f));
    REQUIRE(rec["pitch"] == Catch::Approx(0.5f).margin(0.001f));

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter computes healthPercent from CombatState", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    EntityID e = reg.create();
    reg.emplace<Position>(e, Position{});
    reg.emplace<Velocity>(e);
    CombatState combat;
    combat.health = 7500;
    combat.maxHealth = 10000;
    reg.emplace<CombatState>(e, combat);

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    auto& rec = j["entities"][0];
    REQUIRE(rec["health"] == 75);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter healthPercent defaults to 0 when maxHealth is 0", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    EntityID e = reg.create();
    reg.emplace<Position>(e, Position{});
    reg.emplace<Velocity>(e);
    CombatState combat;
    combat.health = 100;
    combat.maxHealth = 0;
    reg.emplace<CombatState>(e, combat);

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    auto& rec = j["entities"][0];
    REQUIRE(rec["health"] == 0);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter accumulates player/nc counts correctly", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    for (int i = 0; i < 2; ++i) {
        EntityID p = reg.create();
        reg.emplace<Position>(p, Position{});
        reg.emplace<Velocity>(p);
        reg.emplace<PlayerTag>(p);
    }
    for (int i = 0; i < 3; ++i) {
        EntityID n = reg.create();
        reg.emplace<Position>(n, Position{});
        reg.emplace<Velocity>(n);
        reg.emplace<NPCTag>(n);
    }

    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    REQUIRE(j["player_count"] == 2);
    REQUIRE(j["npc_count"] == 3);
    REQUIRE(j["entity_count"] == 5);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter tickCounter resets after export", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(2);

    Registry reg;
    reg.create();

    // Tick 1: counter=1, no export
    exporter.maybeExport(reg, 1, 0);
    REQUIRE(std::filesystem::directory_iterator(dir) == std::filesystem::directory_iterator());

    // Tick 2: counter=2 → export, reset to 0
    exporter.maybeExport(reg, 2, 0);
    REQUIRE(std::distance(std::filesystem::directory_iterator(dir), std::filesystem::directory_iterator{}) == 1);

    // Tick 3: counter=1, no export
    exporter.maybeExport(reg, 3, 0);
    REQUIRE(std::distance(std::filesystem::directory_iterator(dir), std::filesystem::directory_iterator{}) == 1);

    // Tick 4: export again
    exporter.maybeExport(reg, 4, 0);
    REQUIRE(std::distance(std::filesystem::directory_iterator(dir), std::filesystem::directory_iterator{}) == 2);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter JSON output is pretty-printed with 4-space indent", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    reg.create();
    exporter.maybeExport(reg, 1, 0);

    std::string file = dir + "/server_1.json";
    std::ifstream ifs(file);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    // Expect 4-space indentation
    REQUIRE(content.find("\n    ") != std::string::npos);

    std::filesystem::remove_all(dir);
}

TEST_CASE("ServerStateExporter timestamp is recent", "[instrumentation]") {
    std::string dir = tempOutDir();
    ServerStateExporter exporter(dir);
    exporter.setEnabled(true);
    exporter.setExportInterval(1);

    Registry reg;
    reg.create();
    using namespace std::chrono;
    auto before = steady_clock::now();
    exporter.maybeExport(reg, 123, 5000);
    auto after = steady_clock::now();

    std::string file = dir + "/server_123.json";
    std::ifstream ifs(file);
    nlohmann::json j;
    ifs >> j;

    uint64_t fileMs = j["timestamp_ms"].get<uint64_t>();
    auto now = steady_clock::now();
    uint64_t nowMs = duration_cast<milliseconds>(now.time_since_epoch()).count();

    REQUIRE(fileMs <= nowMs + 1000);
    REQUIRE(fileMs >= nowMs - 5000);

    std::filesystem::remove_all(dir);
}

// End of ServerStateExporter tests

