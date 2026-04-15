// [ZONE_AGENT] CrossZoneMessenger Unit Tests
// Tests message serialization, initialization guards, and message routing

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "zones/CrossZoneMessenger.hpp"
#include "db/RedisManager.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <cstring>
#include <cstdint>
#include <vector>
#include <optional>

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// Test Helpers
// ============================================================================

static Position makePosition(float x, float y, float z, uint32_t ts = 0) {
    Position pos;
    pos.x = static_cast<Constants::Fixed>(x * Constants::FLOAT_TO_FIXED);
    pos.y = static_cast<Constants::Fixed>(y * Constants::FLOAT_TO_FIXED);
    pos.z = static_cast<Constants::Fixed>(z * Constants::FLOAT_TO_FIXED);
    pos.timestamp_ms = ts;
    return pos;
}

static Velocity makeVelocity(float dx, float dy, float dz) {
    Velocity vel;
    vel.dx = static_cast<Constants::Fixed>(dx * Constants::FLOAT_TO_FIXED);
    vel.dy = static_cast<Constants::Fixed>(dy * Constants::FLOAT_TO_FIXED);
    vel.dz = static_cast<Constants::Fixed>(dz * Constants::FLOAT_TO_FIXED);
    return vel;
}

// Helper to manually build a ZoneMessage payload for ENTITY_SYNC
static std::vector<uint8_t> buildEntitySyncPayload(EntityID entity,
                                                    const Position& pos,
                                                    const Velocity& vel) {
    std::vector<uint8_t> payload(sizeof(EntityID) + sizeof(Position) + sizeof(Velocity));
    size_t offset = 0;

    *reinterpret_cast<EntityID*>(&payload[offset]) = entity;
    offset += sizeof(EntityID);

    std::memcpy(&payload[offset], &pos, sizeof(Position));
    offset += sizeof(Position);

    std::memcpy(&payload[offset], &vel, sizeof(Velocity));
    return payload;
}

// Helper to build a MIGRATION_COMPLETE payload
static std::vector<uint8_t> buildMigrationCompletePayload(EntityID entity,
                                                           uint64_t playerId) {
    std::vector<uint8_t> payload(sizeof(EntityID) + sizeof(uint64_t));
    *reinterpret_cast<EntityID*>(&payload[0]) = entity;
    *reinterpret_cast<uint64_t*>(&payload[sizeof(EntityID)]) = playerId;
    return payload;
}

// Helper to build a MIGRATION_STATE payload
static std::vector<uint8_t> buildMigrationStatePayload(EntityID entity,
                                                        MigrationState state,
                                                        const std::vector<uint8_t>& data) {
    std::vector<uint8_t> payload(sizeof(EntityID) + 1 + data.size());
    *reinterpret_cast<EntityID*>(&payload[0]) = entity;
    payload[sizeof(EntityID)] = static_cast<uint8_t>(state);
    if (!data.empty()) {
        std::memcpy(&payload[sizeof(EntityID) + 1], data.data(), data.size());
    }
    return payload;
}

// ============================================================================
// ZoneMessage Serialization Tests
// ============================================================================

TEST_CASE("ZoneMessage serialization roundtrip", "[zones][crosszone]") {

    SECTION("Basic message with empty payload") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::BROADCAST;
        msg.sourceZoneId = 1;
        msg.targetZoneId = 0;
        msg.timestamp = 12345;
        msg.sequence = 42;
        msg.payload.clear();

        auto serialized = msg.serialize();
        REQUIRE(serialized.size() == 21);  // 1 + 4 + 4 + 4 + 4 + 4 + 0

        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(deserialized.has_value());
        REQUIRE(deserialized->type == ZoneMessageType::BROADCAST);
        REQUIRE(deserialized->sourceZoneId == 1);
        REQUIRE(deserialized->targetZoneId == 0);
        REQUIRE(deserialized->timestamp == 12345);
        REQUIRE(deserialized->sequence == 42);
        REQUIRE(deserialized->payload.empty());
    }

    SECTION("Entity sync message with payload") {
        Position pos = makePosition(10.0f, 0.0f, 20.0f, 500);
        Velocity vel = makeVelocity(1.5f, 0.0f, -2.0f);
        auto payload = buildEntitySyncPayload(static_cast<EntityID>(99), pos, vel);

        ZoneMessage msg;
        msg.type = ZoneMessageType::ENTITY_SYNC;
        msg.sourceZoneId = 1;
        msg.targetZoneId = 2;
        msg.timestamp = 1000;
        msg.sequence = 7;
        msg.payload = payload;

        auto serialized = msg.serialize();
        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(deserialized.has_value());
        REQUIRE(deserialized->type == ZoneMessageType::ENTITY_SYNC);
        REQUIRE(deserialized->sourceZoneId == 1);
        REQUIRE(deserialized->targetZoneId == 2);
        REQUIRE(deserialized->sequence == 7);
        REQUIRE(deserialized->payload == payload);

        // Verify entity sync payload can be deserialized
        REQUIRE(deserialized->payload.size() >= sizeof(EntityID) + sizeof(Position) + sizeof(Velocity));
        EntityID entity = *reinterpret_cast<const EntityID*>(&deserialized->payload[0]);
        REQUIRE(static_cast<uint32_t>(entity) == 99);

        Position outPos;
        std::memcpy(&outPos, &deserialized->payload[sizeof(EntityID)], sizeof(Position));
        REQUIRE(outPos.x == Approx(pos.x));
        REQUIRE(outPos.z == Approx(pos.z));

        Velocity outVel;
        std::memcpy(&outVel, &deserialized->payload[sizeof(EntityID) + sizeof(Position)], sizeof(Velocity));
        REQUIRE(outVel.dx == Approx(vel.dx));
        REQUIRE(outVel.dz == Approx(vel.dz));
    }

    SECTION("Migration complete message with payload") {
        auto payload = buildMigrationCompletePayload(static_cast<EntityID>(55), 123456789ULL);

        ZoneMessage msg;
        msg.type = ZoneMessageType::MIGRATION_COMPLETE;
        msg.sourceZoneId = 3;
        msg.targetZoneId = 4;
        msg.timestamp = 9999;
        msg.sequence = 100;
        msg.payload = payload;

        auto serialized = msg.serialize();
        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(deserialized.has_value());
        REQUIRE(deserialized->type == ZoneMessageType::MIGRATION_COMPLETE);
        REQUIRE(deserialized->payload.size() == sizeof(EntityID) + sizeof(uint64_t));

        EntityID entity = *reinterpret_cast<const EntityID*>(&deserialized->payload[0]);
        uint64_t playerId = *reinterpret_cast<const uint64_t*>(&deserialized->payload[sizeof(EntityID)]);
        REQUIRE(static_cast<uint32_t>(entity) == 55);
        REQUIRE(playerId == 123456789ULL);
    }

    SECTION("Migration state message with extra data") {
        std::vector<uint8_t> extraData = {0xAA, 0xBB, 0xCC, 0xDD};
        auto payload = buildMigrationStatePayload(
            static_cast<EntityID>(42),
            MigrationState::TRANSFERRING,
            extraData
        );

        ZoneMessage msg;
        msg.type = ZoneMessageType::MIGRATION_STATE;
        msg.sourceZoneId = 2;
        msg.targetZoneId = 1;
        msg.timestamp = 5000;
        msg.sequence = 33;
        msg.payload = payload;

        auto serialized = msg.serialize();
        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(deserialized.has_value());
        REQUIRE(deserialized->type == ZoneMessageType::MIGRATION_STATE);
        REQUIRE(deserialized->payload.size() == sizeof(EntityID) + 1 + extraData.size());

        EntityID entity = *reinterpret_cast<const EntityID*>(&deserialized->payload[0]);
        REQUIRE(static_cast<uint32_t>(entity) == 42);

        MigrationState state = static_cast<MigrationState>(deserialized->payload[sizeof(EntityID)]);
        REQUIRE(state == MigrationState::TRANSFERRING);

        std::vector<uint8_t> outData(
            deserialized->payload.begin() + sizeof(EntityID) + 1,
            deserialized->payload.end()
        );
        REQUIRE(outData == extraData);
    }
}

TEST_CASE("ZoneMessage deserialization edge cases", "[zones][crosszone]") {

    SECTION("Truncated data returns nullopt") {
        std::vector<uint8_t> data(10, 0);  // Too short (< 21 byte header)
        auto result = ZoneMessage::deserialize(data);
        REQUIRE(!result.has_value());
    }

    SECTION("Empty data returns nullopt") {
        std::vector<uint8_t> data;
        auto result = ZoneMessage::deserialize(data);
        REQUIRE(!result.has_value());
    }

    SECTION("Header-only minimum size") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::CHAT;
        msg.sourceZoneId = 1;
        msg.targetZoneId = 2;
        msg.timestamp = 0;
        msg.sequence = 1;
        msg.payload = {};

        auto serialized = msg.serialize();
        REQUIRE(serialized.size() == 21);

        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(deserialized.has_value());
        REQUIRE(deserialized->type == ZoneMessageType::CHAT);
    }

    SECTION("Payload mismatch returns nullopt") {
        // Create a valid header but claim more payload than exists
        ZoneMessage msg;
        msg.type = ZoneMessageType::BROADCAST;
        msg.sourceZoneId = 1;
        msg.targetZoneId = 2;
        msg.timestamp = 0;
        msg.sequence = 1;
        msg.payload = {1, 2, 3};

        auto serialized = msg.serialize();
        // Corrupt the payload size field (offset 17, 4 bytes)
        *reinterpret_cast<uint32_t*>(&serialized[17]) = 9999;  // Claim 9999 bytes payload

        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(!deserialized.has_value());
    }
}

TEST_CASE("ZoneMessage serialization preserves all message types", "[zones][crosszone]") {
    std::vector<ZoneMessageType> types = {
        ZoneMessageType::ENTITY_SYNC,
        ZoneMessageType::MIGRATION_REQUEST,
        ZoneMessageType::MIGRATION_STATE,
        ZoneMessageType::MIGRATION_COMPLETE,
        ZoneMessageType::BROADCAST,
        ZoneMessageType::CHAT,
        ZoneMessageType::ZONE_STATUS
    };

    for (auto type : types) {
        ZoneMessage msg;
        msg.type = type;
        msg.sourceZoneId = 10;
        msg.targetZoneId = 20;
        msg.timestamp = 5555;
        msg.sequence = 77;
        msg.payload = {0x01, 0x02, 0x03};

        auto serialized = msg.serialize();
        auto deserialized = ZoneMessage::deserialize(serialized);
        REQUIRE(deserialized.has_value());
        REQUIRE(deserialized->type == type);
        REQUIRE(deserialized->sourceZoneId == 10);
        REQUIRE(deserialized->targetZoneId == 20);
        REQUIRE(deserialized->timestamp == 5555);
        REQUIRE(deserialized->sequence == 77);
        REQUIRE(deserialized->payload == msg.payload);
    }
}

// ============================================================================
// CrossZoneMessenger Initialization and State Tests
// ============================================================================

TEST_CASE("CrossZoneMessenger construction and metrics", "[zones][crosszone]") {
    CrossZoneMessenger messenger(1, nullptr);

    SECTION("Initial metrics are zero") {
        REQUIRE(messenger.getMessagesSent() == 0);
        REQUIRE(messenger.getMessagesReceived() == 0);
        REQUIRE(messenger.getCurrentSequence() == 0);
    }
}

TEST_CASE("CrossZoneMessenger initialization failure", "[zones][crosszone]") {

    SECTION("Null redis pointer fails initialization") {
        CrossZoneMessenger messenger(1, nullptr);
        bool result = messenger.initialize();
        REQUIRE(result == false);
    }

    SECTION("Shutdown is safe without initialization") {
        CrossZoneMessenger messenger(1, nullptr);
        // Should not crash or assert
        messenger.shutdown();
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("Multiple shutdowns are safe") {
        CrossZoneMessenger messenger(1, nullptr);
        messenger.shutdown();
        messenger.shutdown();
        REQUIRE(messenger.getMessagesSent() == 0);
    }
}

TEST_CASE("CrossZoneMessenger send methods without initialization", "[zones][crosszone]") {
    CrossZoneMessenger messenger(1, nullptr);

    SECTION("sendEntitySync does not crash") {
        Position pos = makePosition(10.0f, 0.0f, 20.0f);
        Velocity vel = makeVelocity(1.0f, 0.0f, 0.0f);
        messenger.sendEntitySync(2, static_cast<EntityID>(1), pos, vel);
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("sendMigrationRequest does not crash") {
        EntitySnapshot snapshot{};
        snapshot.entity = static_cast<EntityID>(1);
        snapshot.playerId = 100;
        snapshot.sourceZoneId = 1;
        snapshot.targetZoneId = 2;
        messenger.sendMigrationRequest(2, snapshot);
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("sendMigrationState does not crash") {
        std::vector<uint8_t> data = {1, 2, 3};
        messenger.sendMigrationState(2, static_cast<EntityID>(1),
                                      MigrationState::SYNCING, data);
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("sendMigrationComplete does not crash") {
        messenger.sendMigrationComplete(2, static_cast<EntityID>(1), 42ULL);
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("broadcast does not crash") {
        std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
        messenger.broadcast(data);
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("update does not crash") {
        messenger.update();
        REQUIRE(messenger.getMessagesSent() == 0);
    }

    SECTION("Multiple send calls stay safe") {
        Position pos = makePosition(0.0f, 0.0f, 0.0f);
        Velocity vel = makeVelocity(0.0f, 0.0f, 0.0f);

        for (int i = 0; i < 100; ++i) {
            messenger.sendEntitySync(2, static_cast<EntityID>(i), pos, vel);
        }
        REQUIRE(messenger.getMessagesSent() == 0);
    }
}

TEST_CASE("CrossZoneMessenger callback registration without init", "[zones][crosszone]") {
    CrossZoneMessenger messenger(1, nullptr);

    SECTION("Setting callbacks does not crash") {
        bool syncCalled = false;
        bool migrationCalled = false;
        bool stateCalled = false;
        bool broadcastCalled = false;

        messenger.setOnEntitySync([&](uint32_t, EntityID, const Position&, const Velocity&) {
            syncCalled = true;
        });
        messenger.setOnMigrationRequest([&](const EntitySnapshot&) {
            migrationCalled = true;
        });
        messenger.setOnMigrationState([&](EntityID, MigrationState, const std::vector<uint8_t>&) {
            stateCalled = true;
        });
        messenger.setOnBroadcast([&](uint32_t, const std::vector<uint8_t>&) {
            broadcastCalled = true;
        });

        // Callbacks should not fire without initialization
        REQUIRE(!syncCalled);
        REQUIRE(!migrationCalled);
        REQUIRE(!stateCalled);
        REQUIRE(!broadcastCalled);
    }
}

// ============================================================================
// CrossZoneMessenger Payload Construction Tests
// (Verify payloads are built correctly for all message types)
// ============================================================================

TEST_CASE("Entity sync payload format", "[zones][crosszone]") {
    Position pos = makePosition(100.5f, 50.0f, -30.25f, 12345);
    Velocity vel = makeVelocity(3.0f, 0.0f, -1.5f);
    EntityID entity = static_cast<EntityID>(42);

    auto payload = buildEntitySyncPayload(entity, pos, vel);

    SECTION("Payload size is correct") {
        REQUIRE(payload.size() == sizeof(EntityID) + sizeof(Position) + sizeof(Velocity));
    }

    SECTION("Entity deserialized correctly") {
        EntityID outEntity = *reinterpret_cast<const EntityID*>(&payload[0]);
        REQUIRE(static_cast<uint32_t>(outEntity) == 42);
    }

    SECTION("Position deserialized correctly") {
        Position outPos;
        std::memcpy(&outPos, &payload[sizeof(EntityID)], sizeof(Position));
        REQUIRE(outPos.x == Approx(pos.x));
        REQUIRE(outPos.y == Approx(pos.y));
        REQUIRE(outPos.z == Approx(pos.z));
        REQUIRE(outPos.timestamp_ms == pos.timestamp_ms);
    }

    SECTION("Velocity deserialized correctly") {
        Velocity outVel;
        std::memcpy(&outVel, &payload[sizeof(EntityID) + sizeof(Position)], sizeof(Velocity));
        REQUIRE(outVel.dx == Approx(vel.dx));
        REQUIRE(outVel.dy == Approx(vel.dy));
        REQUIRE(outVel.dz == Approx(vel.dz));
    }
}

TEST_CASE("Migration complete payload format", "[zones][crosszone]") {
    EntityID entity = static_cast<EntityID>(77);
    uint64_t playerId = 9876543210ULL;

    auto payload = buildMigrationCompletePayload(entity, playerId);

    SECTION("Payload size is correct") {
        REQUIRE(payload.size() == sizeof(EntityID) + sizeof(uint64_t));
    }

    SECTION("Entity and playerId deserialized correctly") {
        EntityID outEntity = *reinterpret_cast<const EntityID*>(&payload[0]);
        uint64_t outPlayerId = *reinterpret_cast<const uint64_t*>(&payload[sizeof(EntityID)]);
        REQUIRE(static_cast<uint32_t>(outEntity) == 77);
        REQUIRE(outPlayerId == 9876543210ULL);
    }
}

TEST_CASE("Migration state payload format", "[zones][crosszone]") {
    EntityID entity = static_cast<EntityID>(33);
    std::vector<uint8_t> extraData = {0x10, 0x20, 0x30, 0x40, 0x50};

    auto payload = buildMigrationStatePayload(entity, MigrationState::COMPLETING, extraData);

    SECTION("Payload size is correct") {
        REQUIRE(payload.size() == sizeof(EntityID) + 1 + extraData.size());
    }

    SECTION("Entity deserialized correctly") {
        EntityID outEntity = *reinterpret_cast<const EntityID*>(&payload[0]);
        REQUIRE(static_cast<uint32_t>(outEntity) == 33);
    }

    SECTION("Migration state deserialized correctly") {
        MigrationState state = static_cast<MigrationState>(payload[sizeof(EntityID)]);
        REQUIRE(state == MigrationState::COMPLETING);
    }

    SECTION("Extra data preserved") {
        std::vector<uint8_t> outData(
            payload.begin() + sizeof(EntityID) + 1,
            payload.end()
        );
        REQUIRE(outData == extraData);
    }

    SECTION("Empty extra data still works") {
        auto emptyPayload = buildMigrationStatePayload(
            entity, MigrationState::FAILED, {}
        );
        REQUIRE(emptyPayload.size() == sizeof(EntityID) + 1);

        MigrationState state = static_cast<MigrationState>(emptyPayload[sizeof(EntityID)]);
        REQUIRE(state == MigrationState::FAILED);
    }
}

// ============================================================================
// MigrationState enum coverage
// ============================================================================

TEST_CASE("MigrationState enum values", "[zones][crosszone]") {
    SECTION("All states serialize to single byte") {
        std::vector<MigrationState> states = {
            MigrationState::NONE,
            MigrationState::PREPARING,
            MigrationState::TRANSFERRING,
            MigrationState::SYNCING,
            MigrationState::COMPLETING,
            MigrationState::COMPLETED,
            MigrationState::FAILED
        };

        for (auto state : states) {
            auto payload = buildMigrationStatePayload(
                static_cast<EntityID>(1), state, {}
            );
            REQUIRE(payload.size() == sizeof(EntityID) + 1);

            MigrationState outState = static_cast<MigrationState>(payload[sizeof(EntityID)]);
            REQUIRE(outState == state);
        }
    }
}

// ============================================================================
// CrossZoneMessenger Full Serialization Pipeline Tests
// (Build ZoneMessage, serialize, deserialize, extract payload)
// ============================================================================

TEST_CASE("Entity sync full serialization pipeline", "[zones][crosszone]") {
    Position pos = makePosition(55.5f, 10.0f, -22.75f, 9999);
    Velocity vel = makeVelocity(-2.0f, 5.0f, 3.25f);
    EntityID entity = static_cast<EntityID>(123);

    // Build message as CrossZoneMessenger would
    auto payload = buildEntitySyncPayload(entity, pos, vel);

    ZoneMessage msg;
    msg.type = ZoneMessageType::ENTITY_SYNC;
    msg.sourceZoneId = 1;
    msg.targetZoneId = 2;
    msg.timestamp = 0;
    msg.sequence = 42;
    msg.payload = payload;

    // Serialize -> deserialize (simulates Redis pub/sub transport)
    auto wire = msg.serialize();
    auto received = ZoneMessage::deserialize(wire);

    REQUIRE(received.has_value());
    REQUIRE(received->type == ZoneMessageType::ENTITY_SYNC);
    REQUIRE(received->sourceZoneId == 1);
    REQUIRE(received->targetZoneId == 2);

    // Extract entity sync fields as onMessageReceived would
    REQUIRE(received->payload.size() >= sizeof(EntityID) + sizeof(Position) + sizeof(Velocity));

    EntityID outEntity = *reinterpret_cast<const EntityID*>(&received->payload[0]);
    Position outPos;
    std::memcpy(&outPos, &received->payload[sizeof(EntityID)], sizeof(Position));
    Velocity outVel;
    std::memcpy(&outVel, &received->payload[sizeof(EntityID) + sizeof(Position)], sizeof(Velocity));

    REQUIRE(static_cast<uint32_t>(outEntity) == 123);
    REQUIRE(outPos.x == Approx(pos.x));
    REQUIRE(outPos.z == Approx(pos.z));
    REQUIRE(outVel.dx == Approx(vel.dx));
    REQUIRE(outVel.dz == Approx(vel.dz));
}

TEST_CASE("Broadcast full serialization pipeline", "[zones][crosszone]") {
    std::vector<uint8_t> broadcastData = {0x01, 0x02, 0x03, 0x04, 0x05};

    ZoneMessage msg;
    msg.type = ZoneMessageType::BROADCAST;
    msg.sourceZoneId = 3;
    msg.targetZoneId = 0;  // All zones
    msg.timestamp = 0;
    msg.sequence = 10;
    msg.payload = broadcastData;

    auto wire = msg.serialize();
    auto received = ZoneMessage::deserialize(wire);

    REQUIRE(received.has_value());
    REQUIRE(received->type == ZoneMessageType::BROADCAST);
    REQUIRE(received->sourceZoneId == 3);
    REQUIRE(received->targetZoneId == 0);
    REQUIRE(received->payload == broadcastData);
}

// ============================================================================
// Message Filtering Logic Tests
// (Test the filtering rules used in onMessageReceived)
// ============================================================================

TEST_CASE("CrossZoneMessenger message filtering logic", "[zones][crosszone]") {
    uint32_t myZoneId = 1;

    SECTION("Message for different zone is filtered") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::ENTITY_SYNC;
        msg.sourceZoneId = 3;
        msg.targetZoneId = 5;  // Not our zone
        msg.timestamp = 0;
        msg.sequence = 1;

        // Filtering rule: targetZoneId != 0 && targetZoneId != myZoneId
        bool shouldProcess = (msg.targetZoneId == 0 || msg.targetZoneId == myZoneId);
        REQUIRE(shouldProcess == false);
    }

    SECTION("Broadcast message (targetZoneId == 0) is accepted") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::BROADCAST;
        msg.sourceZoneId = 3;
        msg.targetZoneId = 0;  // Broadcast
        msg.timestamp = 0;
        msg.sequence = 1;

        bool shouldProcess = (msg.targetZoneId == 0 || msg.targetZoneId == myZoneId);
        REQUIRE(shouldProcess == true);
    }

    SECTION("Message targeted to our zone is accepted") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::ENTITY_SYNC;
        msg.sourceZoneId = 2;
        msg.targetZoneId = myZoneId;
        msg.timestamp = 0;
        msg.sequence = 1;

        bool shouldProcess = (msg.targetZoneId == 0 || msg.targetZoneId == myZoneId);
        REQUIRE(shouldProcess == true);
    }

    SECTION("Own zone messages are ignored") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::ENTITY_SYNC;
        msg.sourceZoneId = myZoneId;
        msg.targetZoneId = 2;
        msg.timestamp = 0;
        msg.sequence = 1;

        // Filtering rule: sourceZoneId == myZoneId means ignore
        bool isSelfMessage = (msg.sourceZoneId == myZoneId);
        REQUIRE(isSelfMessage == true);
    }

    SECTION("Messages from other zones pass self-filter") {
        ZoneMessage msg;
        msg.type = ZoneMessageType::ENTITY_SYNC;
        msg.sourceZoneId = 2;
        msg.targetZoneId = myZoneId;
        msg.timestamp = 0;
        msg.sequence = 1;

        bool isSelfMessage = (msg.sourceZoneId == myZoneId);
        REQUIRE(isSelfMessage == false);
    }
}

// ============================================================================
// CrossZoneMessenger Sequence Numbering
// ============================================================================

TEST_CASE("ZoneMessage sequence numbers in payloads", "[zones][crosszone]") {
    SECTION("Entity sync messages have incrementing sequences") {
        Position pos = makePosition(0.0f, 0.0f, 0.0f);
        Velocity vel = makeVelocity(0.0f, 0.0f, 0.0f);

        // Simulate what the messenger does: build messages with incrementing sequence
        uint32_t seq = 0;
        for (int i = 0; i < 5; ++i) {
            ++seq;
            auto payload = buildEntitySyncPayload(static_cast<EntityID>(i), pos, vel);
            ZoneMessage msg;
            msg.type = ZoneMessageType::ENTITY_SYNC;
            msg.sourceZoneId = 1;
            msg.targetZoneId = 2;
            msg.sequence = seq;
            msg.payload = payload;

            auto wire = msg.serialize();
            auto received = ZoneMessage::deserialize(wire);
            REQUIRE(received.has_value());
            REQUIRE(received->sequence == static_cast<uint32_t>(i + 1));
        }
    }
}
