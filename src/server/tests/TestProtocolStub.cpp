// [NETWORK_AGENT] Unit tests for Protocol stub implementation (client-compatible format)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "netcode/NetworkManager.hpp"
#include <vector>
#include <cstring>

using namespace DarkAges;

TEST_CASE("Protocol stub - createFullSnapshot basic format", "[protocol][network]") {
    std::vector<Protocol::EntityStateData> entities;

    Protocol::EntityStateData e1;
    e1.entity = static_cast<EntityID>(1);
    e1.position = Position::fromVec3(glm::vec3(100.0f, 50.0f, 200.0f));
    e1.velocity.dx = Constants::Fixed(100);
    e1.velocity.dy = Constants::Fixed(0);
    e1.velocity.dz = Constants::Fixed(-50);
    e1.healthPercent = 85;
    e1.animState = 1;
    entities.push_back(e1);

    auto data = Protocol::createFullSnapshot(1000, 42, entities);

    REQUIRE(data.size() == 13 + 102);  // header + 1 entity (102 bytes with Interactable)
    REQUIRE(data[0] == static_cast<uint8_t>(PacketType::ServerSnapshot));

    uint32_t tick, lastInput, count;
    std::memcpy(&tick, &data[1], 4);
    std::memcpy(&lastInput, &data[5], 4);
    std::memcpy(&count, &data[9], 4);

    REQUIRE(tick == 1000);
    REQUIRE(lastInput == 42);
    REQUIRE(count == 1);

    uint32_t entityId;
    std::memcpy(&entityId, &data[13], 4);
    REQUIRE(entityId == 1);

    float posX, posY, posZ;
    std::memcpy(&posX, &data[17], 4);
    std::memcpy(&posY, &data[21], 4);
    std::memcpy(&posZ, &data[25], 4);

    REQUIRE(posX == Catch::Approx(100.0f).margin(0.01f));
    REQUIRE(posY == Catch::Approx(50.0f).margin(0.01f));
    REQUIRE(posZ == Catch::Approx(200.0f).margin(0.01f));

    float velX, velY, velZ;
    std::memcpy(&velX, &data[29], 4);
    std::memcpy(&velY, &data[33], 4);
    std::memcpy(&velZ, &data[37], 4);

    REQUIRE(velX == Catch::Approx(0.1f).margin(0.001f));   // 100 / 1000
    REQUIRE(velY == Catch::Approx(0.0f).margin(0.001f));
    REQUIRE(velZ == Catch::Approx(-0.05f).margin(0.001f)); // -50 / 1000

    REQUIRE(data[41] == 85);   // health
    REQUIRE(data[42] == 1);    // animState
}

TEST_CASE("Protocol stub - createFullSnapshot empty entities", "[protocol][network]") {
    auto data = Protocol::createFullSnapshot(500, 0, {});
    REQUIRE(data.size() == 13);

    uint32_t tick, lastInput, count;
    std::memcpy(&tick, &data[1], 4);
    std::memcpy(&lastInput, &data[5], 4);
    std::memcpy(&count, &data[9], 4);

    REQUIRE(tick == 500);
    REQUIRE(lastInput == 0);
    REQUIRE(count == 0);
}

TEST_CASE("Protocol stub - createFullSnapshot multiple entities", "[protocol][network]") {
    std::vector<Protocol::EntityStateData> entities;

    for (int i = 0; i < 5; ++i) {
        Protocol::EntityStateData e;
        e.entity = static_cast<EntityID>(i + 10);
        e.position = Position::fromVec3(glm::vec3(i * 10.0f, 0.0f, i * 20.0f));
        e.velocity.dx = Constants::Fixed(i * 50);
        e.healthPercent = static_cast<uint8_t>(100 - i * 10);
        e.animState = static_cast<uint8_t>(i);
        entities.push_back(e);
    }

    auto data = Protocol::createFullSnapshot(2000, 99, entities);
    REQUIRE(data.size() == 13 + 5 * 102);  // header + 5 entities (102 bytes each with Interactable)

    uint32_t count;
    std::memcpy(&count, &data[9], 4);
    REQUIRE(count == 5);
}

TEST_CASE("Protocol stub - versioning", "[protocol][network]") {
    SECTION("getProtocolVersion returns 1.0") {
        uint32_t version = Protocol::getProtocolVersion();
        REQUIRE(version == 0x00010000);

        uint16_t major = static_cast<uint16_t>(version >> 16);
        uint16_t minor = static_cast<uint16_t>(version & 0xFFFF);
        REQUIRE(major == 1);
        REQUIRE(minor == 0);
    }

    SECTION("Same major version is compatible") {
        REQUIRE(Protocol::isVersionCompatible(0x00010000));
        REQUIRE(Protocol::isVersionCompatible(0x00010001));
        REQUIRE(Protocol::isVersionCompatible(0x00010FFF));
    }

    SECTION("Different major version is incompatible") {
        REQUIRE_FALSE(Protocol::isVersionCompatible(0x00020000));
        REQUIRE_FALSE(Protocol::isVersionCompatible(0x00000000));
    }
}

TEST_CASE("Protocol stub - serializeCorrection", "[protocol][network]") {
    Position pos = Position::fromVec3(glm::vec3(100.0f, 50.0f, 200.0f));
    Velocity vel;
    vel.dx = Constants::Fixed(500);
    vel.dy = Constants::Fixed(0);
    vel.dz = Constants::Fixed(300);

    auto data = Protocol::serializeCorrection(1500, pos, vel, 1234);

    REQUIRE(data.size() == 1 + 4 + 12 + 12 + 4);  // type + tick + pos + vel + lastInput
    REQUIRE(data[0] == static_cast<uint8_t>(PacketType::ReliableEvent));

    uint32_t tick;
    std::memcpy(&tick, &data[1], 4);
    REQUIRE(tick == 1500);

    float posX, posY, posZ;
    std::memcpy(&posX, &data[5], 4);
    std::memcpy(&posY, &data[9], 4);
    std::memcpy(&posZ, &data[13], 4);

    REQUIRE(posX == Catch::Approx(100.0f).margin(0.01f));
    REQUIRE(posY == Catch::Approx(50.0f).margin(0.01f));
    REQUIRE(posZ == Catch::Approx(200.0f).margin(0.01f));

    uint32_t lastInput;
    std::memcpy(&lastInput, &data[29], 4);
    REQUIRE(lastInput == 1234);
}

TEST_CASE("Protocol stub - createFullSnapshot matches client expected layout", "[protocol][network]") {
    // This test verifies the snapshot format matches what the Godot client
    // NetworkManager.cs ProcessSnapshot expects:
    // [type:1][server_tick:4][last_input:4][entity_count:4]
    // Each entity: [id:4][x:4f][y:4f][z:4f][vx:4f][vy:4f][vz:4f][health:1][anim:1]

    std::vector<Protocol::EntityStateData> entities;
    Protocol::EntityStateData e;
    e.entity = static_cast<EntityID>(42);
    e.position = Position::fromVec3(glm::vec3(1.0f, 2.0f, 3.0f));
    e.velocity.dx = Constants::Fixed(10);
    e.velocity.dy = Constants::Fixed(20);
    e.velocity.dz = Constants::Fixed(30);
    e.healthPercent = 100;
    e.animState = 0;
    entities.push_back(e);

    auto data = Protocol::createFullSnapshot(1234, 5678, entities);

    // Verify exact offsets match client expectations
    REQUIRE(data[0] == 2);  // PacketType::ServerSnapshot

    uint32_t serverTick;
    std::memcpy(&serverTick, &data[1], 4);
    REQUIRE(serverTick == 1234);

    uint32_t lastInput;
    std::memcpy(&lastInput, &data[5], 4);
    REQUIRE(lastInput == 5678);

    uint32_t entityCount;
    std::memcpy(&entityCount, &data[9], 4);
    REQUIRE(entityCount == 1);

    uint32_t entityId;
    std::memcpy(&entityId, &data[13], 4);
    REQUIRE(entityId == 42);

    float x, y, z;
    std::memcpy(&x, &data[17], 4);
    std::memcpy(&y, &data[21], 4);
    std::memcpy(&z, &data[25], 4);
    REQUIRE(x == Catch::Approx(1.0f).margin(0.001f));
    REQUIRE(y == Catch::Approx(2.0f).margin(0.001f));
    REQUIRE(z == Catch::Approx(3.0f).margin(0.001f));

    float vx, vy, vz;
    std::memcpy(&vx, &data[29], 4);
    std::memcpy(&vy, &data[33], 4);
    std::memcpy(&vz, &data[37], 4);
    REQUIRE(vx == Catch::Approx(0.01f).margin(0.0001f));
    REQUIRE(vy == Catch::Approx(0.02f).margin(0.0001f));
    REQUIRE(vz == Catch::Approx(0.03f).margin(0.0001f));

    REQUIRE(data[41] == 100);  // health
    REQUIRE(data[42] == 0);    // animState
}
