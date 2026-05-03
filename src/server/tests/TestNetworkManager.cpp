// Comprehensive tests for NetworkManager types and Protocol data structures
// Tests header-only types (no GNS dependency) — PacketType, ConnectionQuality,
// ClientInputPacket, SnapshotPacket, EntityStateData, DeltaEntityState,
// DeltaFieldMask, DeltaEncoding constants
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "netcode/NetworkManager.hpp"
#include "ecs/CoreTypes.hpp"
#include "Constants.hpp"
#include <atomic>

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// Helper: Create test entity state
// ============================================================================
static Protocol::EntityStateData makeEntityState(EntityID id, float x, float y, float z) {
    Protocol::EntityStateData state;
    state.entity = id;
    state.position.x = static_cast<Constants::Fixed>(x);
    state.position.y = static_cast<Constants::Fixed>(y);
    state.position.z = static_cast<Constants::Fixed>(z);
    state.healthPercent = 100;
    state.animState = 0;
    state.entityType = 0;
    state.timestamp = 1000;
    return state;
}

// ============================================================================
// PacketType enum
// ============================================================================
TEST_CASE("PacketType enum values are distinct", "[network]") {
    CHECK(static_cast<uint8_t>(PacketType::ClientInput) == 1);
    CHECK(static_cast<uint8_t>(PacketType::ServerSnapshot) == 2);
    CHECK(static_cast<uint8_t>(PacketType::ReliableEvent) == 3);
    CHECK(static_cast<uint8_t>(PacketType::Ping) == 4);
    CHECK(static_cast<uint8_t>(PacketType::LockOnRequest) == 5);
    CHECK(static_cast<uint8_t>(PacketType::Disconnect) == 6);
}

TEST_CASE("PacketType all values unique", "[network]") {
    std::vector<uint8_t> values = {
        static_cast<uint8_t>(PacketType::ClientInput),
        static_cast<uint8_t>(PacketType::ServerSnapshot),
        static_cast<uint8_t>(PacketType::ReliableEvent),
        static_cast<uint8_t>(PacketType::Ping),
        static_cast<uint8_t>(PacketType::LockOnRequest),
        static_cast<uint8_t>(PacketType::Disconnect),
    };
    std::sort(values.begin(), values.end());
    auto last = std::unique(values.begin(), values.end());
    CHECK(last == values.end()); // All unique
}

// ============================================================================
// ConnectionQuality struct
// ============================================================================
TEST_CASE("ConnectionQuality default values", "[network]") {
    ConnectionQuality quality;
    CHECK(quality.rttMs == 0);
    CHECK(quality.packetLoss == 0.0f);
    CHECK(quality.jitterMs == 0.0f);
    CHECK(quality.packetsSent == 0);
    CHECK(quality.packetsReceived == 0);
    CHECK(quality.bytesSent == 0);
    CHECK(quality.bytesReceived == 0);
}

TEST_CASE("ConnectionQuality modification", "[network]") {
    ConnectionQuality quality;
    quality.rttMs = 50;
    quality.packetLoss = 0.05f;
    quality.jitterMs = 5.0f;
    quality.packetsSent = 1000;
    quality.packetsReceived = 950;
    quality.bytesSent = 50000;
    quality.bytesReceived = 47500;

    CHECK(quality.rttMs == 50);
    CHECK(quality.packetLoss == Approx(0.05f));
    CHECK(quality.jitterMs == Approx(5.0f));
    CHECK(quality.packetsSent == 1000);
    CHECK(quality.packetsReceived == 950);
    CHECK(quality.bytesSent == 50000);
    CHECK(quality.bytesReceived == 47500);
}

// ============================================================================
// ClientInputPacket struct
// ============================================================================
TEST_CASE("ClientInputPacket default values", "[network]") {
    ClientInputPacket packet;
    CHECK(packet.connectionId == INVALID_CONNECTION);
    CHECK(packet.receiveTimeMs == 0);
}

TEST_CASE("INVALID_CONNECTION constant", "[network]") {
    CHECK(INVALID_CONNECTION == 0);
    ConnectionID valid = 42;
    CHECK(valid != INVALID_CONNECTION);
}

// ============================================================================
// SnapshotPacket struct
// ============================================================================
TEST_CASE("SnapshotPacket default values", "[network]") {
    SnapshotPacket packet;
    CHECK(packet.serverTick == 0);
    CHECK(packet.baselineTick == 0);
    CHECK(packet.data.empty());
}

TEST_CASE("SnapshotPacket data manipulation", "[network]") {
    SnapshotPacket packet;
    packet.serverTick = 100;
    packet.baselineTick = 99;
    packet.data = {0x01, 0x02, 0x03, 0x04};

    CHECK(packet.data.size() == 4);
    CHECK(packet.data[0] == 0x01);
    CHECK(packet.data[3] == 0x04);
}

TEST_CASE("SnapshotPacket copy semantics", "[network]") {
    SnapshotPacket original;
    original.serverTick = 50;
    original.data = {0xAA, 0xBB};

    SnapshotPacket copy = original;
    CHECK(copy.serverTick == 50);
    CHECK(copy.data.size() == 2);
    CHECK(copy.data[0] == 0xAA);
}

// ============================================================================
// EntityStateData struct
// ============================================================================
TEST_CASE("EntityStateData default values", "[network]") {
    Protocol::EntityStateData state{};
    CHECK(state.healthPercent == 0);
    CHECK(state.animState == 0);
    CHECK(state.entityType == 0);
    CHECK(state.timestamp == 0);
}

TEST_CASE("EntityStateData entityType semantics", "[network]") {
    // 0=player, 1=projectile, 2=loot, 3=NPC
    Protocol::EntityStateData state;
    state.entityType = 0;
    CHECK(state.entityType == 0); // player

    state.entityType = 1;
    CHECK(state.entityType == 1); // projectile

    state.entityType = 2;
    CHECK(state.entityType == 2); // loot

    state.entityType = 3;
    CHECK(state.entityType == 3); // NPC
}

TEST_CASE("EntityStateData healthPercent range", "[network]") {
    Protocol::EntityStateData state;
    state.healthPercent = 0;
    CHECK(state.healthPercent == 0);

    state.healthPercent = 50;
    CHECK(state.healthPercent == 50);

    state.healthPercent = 100;
    CHECK(state.healthPercent == 100);
}

TEST_CASE("EntityStateData equality helpers", "[network]") {
    auto state1 = makeEntityState(static_cast<EntityID>(1), 10.0f, 20.0f, 0.0f);
    auto state2 = makeEntityState(static_cast<EntityID>(1), 10.0f, 20.0f, 0.0f);
    auto state3 = makeEntityState(static_cast<EntityID>(1), 15.0f, 20.0f, 0.0f);

    CHECK(state1.equalsPosition(state2));
    CHECK_FALSE(state1.equalsPosition(state3));
}

TEST_CASE("EntityStateData field assignment", "[network]") {
    auto state = makeEntityState(static_cast<EntityID>(42), 100.0f, 200.0f, 50.0f);
    CHECK(static_cast<uint32_t>(state.entity) == 42);
    CHECK(state.healthPercent == 100);
    CHECK(state.timestamp == 1000);
}

// ============================================================================
// DeltaFieldMask enum
// ============================================================================
TEST_CASE("DeltaFieldMask bit values", "[network]") {
    CHECK(Protocol::DELTA_POSITION == (1 << 0));
    CHECK(Protocol::DELTA_ROTATION == (1 << 1));
    CHECK(Protocol::DELTA_VELOCITY == (1 << 2));
    CHECK(Protocol::DELTA_HEALTH == (1 << 3));
    CHECK(Protocol::DELTA_ANIM_STATE == (1 << 4));
    CHECK(Protocol::DELTA_ENTITY_TYPE == (1 << 5));
    CHECK(Protocol::DELTA_NEW_ENTITY == 0xFFFF);
}

TEST_CASE("DeltaFieldMask combinations", "[network]") {
    uint16_t changed = Protocol::DELTA_POSITION | Protocol::DELTA_HEALTH;
    CHECK((changed & Protocol::DELTA_POSITION) != 0);
    CHECK((changed & Protocol::DELTA_HEALTH) != 0);
    CHECK((changed & Protocol::DELTA_ROTATION) == 0);
    CHECK((changed & Protocol::DELTA_VELOCITY) == 0);
    CHECK((changed & Protocol::DELTA_ANIM_STATE) == 0);
    CHECK((changed & Protocol::DELTA_ENTITY_TYPE) == 0);
}

TEST_CASE("DeltaFieldMask all bits distinct", "[network]") {
    std::vector<uint16_t> bits = {
        Protocol::DELTA_POSITION,
        Protocol::DELTA_ROTATION,
        Protocol::DELTA_VELOCITY,
        Protocol::DELTA_HEALTH,
        Protocol::DELTA_ANIM_STATE,
        Protocol::DELTA_ENTITY_TYPE,
    };
    // Each should have exactly one bit set
    for (auto b : bits) {
        CHECK(__builtin_popcount(b) == 1);
    }
    // All different
    std::sort(bits.begin(), bits.end());
    auto last = std::unique(bits.begin(), bits.end());
    CHECK(last == bits.end());
}

// ============================================================================
// DeltaEntityState struct
// ============================================================================
TEST_CASE("DeltaEntityState field mask isolation", "[network]") {
    Protocol::DeltaEntityState delta;
    delta.entity = static_cast<EntityID>(42);
    delta.changedFields = Protocol::DELTA_POSITION | Protocol::DELTA_VELOCITY;

    CHECK((delta.changedFields & Protocol::DELTA_POSITION) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_VELOCITY) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_ROTATION) == 0);
    CHECK((delta.changedFields & Protocol::DELTA_HEALTH) == 0);
    CHECK((delta.changedFields & Protocol::DELTA_ANIM_STATE) == 0);
    CHECK((delta.changedFields & Protocol::DELTA_ENTITY_TYPE) == 0);
}

TEST_CASE("DeltaEntityState new entity mask", "[network]") {
    Protocol::DeltaEntityState delta;
    delta.changedFields = Protocol::DELTA_NEW_ENTITY;

    // All standard fields should be set
    CHECK((delta.changedFields & Protocol::DELTA_POSITION) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_ROTATION) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_VELOCITY) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_HEALTH) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_ANIM_STATE) != 0);
    CHECK((delta.changedFields & Protocol::DELTA_ENTITY_TYPE) != 0);
}

// ============================================================================
// DeltaEncoding threshold constants
// ============================================================================
TEST_CASE("DeltaEncoding threshold values", "[network]") {
    CHECK(Protocol::DeltaEncoding::SMALL_DELTA_THRESHOLD == 127);
    CHECK(Protocol::DeltaEncoding::MEDIUM_DELTA_THRESHOLD == 32767);
    CHECK(Protocol::DeltaEncoding::SMALL_DELTA_THRESHOLD < Protocol::DeltaEncoding::MEDIUM_DELTA_THRESHOLD);
}

TEST_CASE("DeltaEncoding thresholds fit encoding sizes", "[network]") {
    // Small threshold fits in int8 (127)
    CHECK(Protocol::DeltaEncoding::SMALL_DELTA_THRESHOLD <= 127);
    // Medium threshold fits in int16 (32767)
    CHECK(Protocol::DeltaEncoding::MEDIUM_DELTA_THRESHOLD <= 32767);
}

// ============================================================================
// Connection constants
// ============================================================================
TEST_CASE("ConnectionID type properties", "[network]") {
    CHECK(sizeof(ConnectionID) == sizeof(uint32_t));
}

TEST_CASE("ConnectionID values", "[network]") {
    ConnectionID id1 = static_cast<ConnectionID>(1);
    ConnectionID id2 = static_cast<ConnectionID>(2);
    CHECK(id1 != id2);
    CHECK(id1 != INVALID_CONNECTION);
    CHECK(id2 != INVALID_CONNECTION);
}

TEST_CASE("ConnectionID can store large values", "[network]") {
    ConnectionID large = UINT32_MAX;
    CHECK(large != INVALID_CONNECTION);
    CHECK(large == UINT32_MAX);
}


    // ============================================================================
    // NetworkManager behavioral tests (autonomous expansion)
    // ============================================================================


    // ============================================================================
    // NetworkManager behavioral tests (autonomous expansion)
    // ============================================================================

    TEST_CASE("NetworkManager is a complete type", "[networkmanager]") {
        REQUIRE(sizeof(NetworkManager) > 0);
    }

    TEST_CASE("NetworkManager initialize/shutdown lifecycle", "[networkmanager]") {
        NetworkManager obj;

        SECTION("initialize returns true") {
            REQUIRE(obj.initialize());
        }

        SECTION("shutdown before init is safe") {
            REQUIRE_NOTHROW(obj.shutdown());
        }

        SECTION("double initialize is safe") {
            REQUIRE(obj.initialize());
            REQUIRE_NOTHROW(obj.initialize());
        }

        SECTION("shutdown after init is safe") {
            obj.initialize();
            REQUIRE_NOTHROW(obj.shutdown());
        }
    }

    TEST_CASE("NetworkManager update requires time parameter", "[networkmanager]") {
        NetworkManager obj;
        obj.initialize();
        REQUIRE_NOTHROW(obj.update(0));
        REQUIRE_NOTHROW(obj.update(100));
        for (int i = 0; i < 10; ++i) {
            REQUIRE_NOTHROW(obj.update(i * 16));
        }
    }

    TEST_CASE("NetworkManager callback setters are noexcept", "[networkmanager]") {
        NetworkManager obj;
        obj.initialize();
        // Public callback setters should accept any callable without throwing
        REQUIRE_NOTHROW(obj.setOnClientConnected([](ConnectionID) {}));
        REQUIRE_NOTHROW(obj.setOnClientDisconnected([](ConnectionID) {}));
        REQUIRE_NOTHROW(obj.setOnInputReceived([](const ClientInputPacket&) {}));
        REQUIRE_NOTHROW(obj.setOnCombatAction([](const CombatActionPacket&) {}));
        REQUIRE_NOTHROW(obj.setOnLockOnRequest([](const LockOnRequestPacket&) {}));
        REQUIRE_NOTHROW(obj.setOnChatReceived([](ConnectionID, const ChatMessage&) {}));
        REQUIRE_NOTHROW(obj.setOnQuestActionReceived([](const QuestActionPacket&) {}));
        REQUIRE_NOTHROW(obj.setOnDialogueResponseReceived([](const Protocol::DialogueResponsePacket&) {}));
    }

    // Note: Connection status callbacks (SteamNetConnectionStatusChangedCallback,
    // onConnectionStatusChanged) are internal GNS mechanisms exercised via init/shutdown.
