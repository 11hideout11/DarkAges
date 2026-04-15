// [NETWORK_AGENT] Unit tests for GNSNetworkManager
// Tests for construction, initialization, connection management, and state tracking

#include <catch2/catch_test_macros.hpp>
#include "netcode/GNSNetworkManager.hpp"

#include <cstring>
#include <vector>

using namespace DarkAges::Netcode;

// ============================================================================
// Always-available type tests (header is includable even without ENABLE_GNS)
// ============================================================================

TEST_CASE("GNSConfig default values", "[network][gns]") {
    GNSConfig config;

    SECTION("Default port is 7777") {
        REQUIRE(config.port == 7777);
    }

    SECTION("Default max connections is 1000") {
        REQUIRE(config.maxConnections == 1000);
    }

    SECTION("Encryption enabled by default") {
        REQUIRE(config.enableEncryption == true);
    }

    SECTION("NAT punchthrough enabled by default") {
        REQUIRE(config.enableNATPunchthrough == true);
    }

    SECTION("Default send rate is 256 KB/s") {
        REQUIRE(config.maxSendRate == 256 * 1024);
    }

    SECTION("Default recv rate is 64 KB/s") {
        REQUIRE(config.maxRecvRate == 64 * 1024);
    }

    SECTION("Default connection timeout is 10 seconds") {
        REQUIRE(config.connectionTimeoutMs == 10000);
    }

    SECTION("Default heartbeat interval is 500ms") {
        REQUIRE(config.heartbeatIntervalMs == 500);
    }
}

TEST_CASE("GNSConfig custom values", "[network][gns]") {
    GNSConfig config;
    config.port = 9000;
    config.maxConnections = 500;
    config.enableEncryption = false;
    config.enableNATPunchthrough = false;
    config.maxSendRate = 512 * 1024;
    config.maxRecvRate = 128 * 1024;
    config.connectionTimeoutMs = 5000;
    config.heartbeatIntervalMs = 250;

    REQUIRE(config.port == 9000);
    REQUIRE(config.maxConnections == 500);
    REQUIRE(config.enableEncryption == false);
    REQUIRE(config.enableNATPunchthrough == false);
    REQUIRE(config.maxSendRate == 512 * 1024);
    REQUIRE(config.maxRecvRate == 128 * 1024);
    REQUIRE(config.connectionTimeoutMs == 5000);
    REQUIRE(config.heartbeatIntervalMs == 250);
}

TEST_CASE("ConnectionStats default values", "[network][gns]") {
    ConnectionStats stats{};

    REQUIRE(stats.ping_ms == 0.0f);
    REQUIRE(stats.packet_loss_percent == 0.0f);
    REQUIRE(stats.bytes_sent == 0);
    REQUIRE(stats.bytes_received == 0);
    REQUIRE(stats.packets_sent == 0);
    REQUIRE(stats.packets_received == 0);
    REQUIRE(stats.send_rate_kbps == 0.0f);
    REQUIRE(stats.recv_rate_kbps == 0.0f);
}

TEST_CASE("ConnectionState enum values", "[network][gns]") {
    // Verify enum values exist and are distinct
    auto disconnected = ConnectionState::Disconnected;
    auto connecting = ConnectionState::Connecting;
    auto connected = ConnectionState::Connected;
    auto disconnecting = ConnectionState::Disconnecting;

    REQUIRE(disconnected != connecting);
    REQUIRE(disconnected != connected);
    REQUIRE(disconnected != disconnecting);
    REQUIRE(connecting != connected);
    REQUIRE(connecting != disconnecting);
    REQUIRE(connected != disconnecting);
}

TEST_CASE("ClientInputPacket default values", "[network][gns]") {
    ClientInputPacket packet{};

    REQUIRE(packet.connectionId == 0);
    REQUIRE(packet.data.empty());
    REQUIRE(packet.sequence == 0);
    REQUIRE(packet.receiveTimeMs == 0);
}

TEST_CASE("ClientInputPacket data manipulation", "[network][gns]") {
    ClientInputPacket packet{};
    packet.connectionId = 42;
    packet.sequence = 100;
    packet.receiveTimeMs = 1234567890;

    // Populate data
    packet.data = {0x01, 0x02, 0x03, 0x04, 0x05};

    REQUIRE(packet.connectionId == 42);
    REQUIRE(packet.sequence == 100);
    REQUIRE(packet.receiveTimeMs == 1234567890);
    REQUIRE(packet.data.size() == 5);
    REQUIRE(packet.data[0] == 0x01);
    REQUIRE(packet.data[4] == 0x05);
}

// ============================================================================
// GNS-dependent tests (only compile when ENABLE_GNS is defined)
// These test the actual GNSNetworkManager class behavior.
// Without GNS, the class has no compiled implementation, so we skip these.
// ============================================================================

#ifdef ENABLE_GNS

TEST_CASE("GNSNetworkManager construction", "[network][gns]") {
    SECTION("Default construction succeeds") {
        GNSNetworkManager manager;
        // After construction, manager is not initialized
        REQUIRE(manager.getConnectionCount() == 0);
    }

    SECTION("Static instance is set after construction") {
        // Construction sets the static instance pointer
        GNSNetworkManager manager;
        // We can verify indirectly by checking that queries work
        REQUIRE(manager.getConnectionCount() == 0);
        REQUIRE(manager.getConnectedClients().empty());
    }

    SECTION("Destructor cleans up properly") {
        {
            GNSNetworkManager manager;
            // Manager goes out of scope - should not crash
        }
        REQUIRE(true); // If we get here, destructor was clean
    }
}

TEST_CASE("GNSNetworkManager initialization", "[network][gns]") {
    SECTION("Initialize with port number") {
        GNSNetworkManager manager;
        bool result = manager.initialize(static_cast<uint16_t>(17777));
        REQUIRE(result);
        REQUIRE(manager.getConnectionCount() == 0);
        manager.shutdown();
    }

    SECTION("Initialize with config") {
        GNSNetworkManager manager;
        GNSConfig config;
        config.port = 17778;
        config.maxConnections = 100;
        config.enableEncryption = false;

        bool result = manager.initialize(config);
        REQUIRE(result);
        manager.shutdown();
    }

    SECTION("Double initialization returns false") {
        GNSNetworkManager manager;
        bool first = manager.initialize(static_cast<uint16_t>(17779));
        REQUIRE(first);

        // Second init should fail (already initialized)
        bool second = manager.initialize(static_cast<uint16_t>(17780));
        REQUIRE_FALSE(second);

        manager.shutdown();
    }

    SECTION("Shutdown without initialization is safe") {
        GNSNetworkManager manager;
        // Should not crash
        manager.shutdown();
        REQUIRE(true);
    }

    SECTION("Initialize after shutdown works") {
        GNSNetworkManager manager;
        manager.initialize(static_cast<uint16_t>(17781));
        manager.shutdown();

        // Re-initialize should work
        bool result = manager.initialize(static_cast<uint16_t>(17782));
        REQUIRE(result);
        manager.shutdown();
    }
}

TEST_CASE("GNSNetworkManager connection queries on empty manager", "[network][gns]") {
    GNSNetworkManager manager;
    manager.initialize(static_cast<uint16_t>(17783));

    SECTION("No connections initially") {
        REQUIRE(manager.getConnectionCount() == 0);
    }

    SECTION("Connected clients list is empty") {
        auto clients = manager.getConnectedClients();
        REQUIRE(clients.empty());
    }

    SECTION("Any client ID is not connected") {
        REQUIRE_FALSE(manager.isClientConnected(1));
        REQUIRE_FALSE(manager.isClientConnected(999));
    }

    SECTION("Connection stats for unknown client are default") {
        auto stats = manager.getConnectionStats(999);
        REQUIRE(stats.ping_ms == 0.0f);
        REQUIRE(stats.packet_loss_percent == 0.0f);
        REQUIRE(stats.bytes_sent == 0);
        REQUIRE(stats.packets_sent == 0);
    }

    SECTION("Get pending inputs returns empty") {
        auto inputs = manager.getPendingInputs();
        REQUIRE(inputs.empty());
    }

    manager.shutdown();
}

TEST_CASE("GNSNetworkManager update loop", "[network][gns]") {
    GNSNetworkManager manager;
    manager.initialize(static_cast<uint16_t>(17784));

    SECTION("Update with no connections is safe") {
        for (uint32_t tick = 0; tick < 60; ++tick) {
            manager.update(tick * 16);
        }
        REQUIRE(manager.getConnectionCount() == 0);
    }

    SECTION("Update after shutdown does nothing") {
        manager.shutdown();
        // Should not crash
        manager.update(0);
        REQUIRE(true);
    }

    manager.shutdown();
}

TEST_CASE("GNSNetworkManager send operations on empty manager", "[network][gns]") {
    GNSNetworkManager manager;
    manager.initialize(static_cast<uint16_t>(17785));

    SECTION("Send to non-existent client is safe") {
        uint8_t data[] = {0x01, 0x02, 0x03};
        manager.sendToClient(999, data, sizeof(data), true);
        REQUIRE(true); // Should not crash
    }

    SECTION("Send unreliable to non-existent client is safe") {
        uint8_t data[] = {0xAA, 0xBB};
        manager.sendToClient(42, data, sizeof(data), false);
        REQUIRE(true);
    }

    SECTION("Broadcast with no connections is safe") {
        uint8_t data[] = {0xFF};
        manager.broadcast(data, sizeof(data), true);
        REQUIRE(manager.getConnectionCount() == 0);
    }

    SECTION("Broadcast with empty data is safe") {
        manager.broadcast(nullptr, 0, true);
        REQUIRE(true);
    }

    SECTION("Broadcast except with no connections is safe") {
        uint8_t data[] = {0x01};
        manager.broadcastExcept(1, data, sizeof(data), false);
        REQUIRE(manager.getConnectionCount() == 0);
    }

    manager.shutdown();
}

TEST_CASE("GNSNetworkManager disconnect operations", "[network][gns]") {
    GNSNetworkManager manager;
    manager.initialize(static_cast<uint16_t>(17786));

    SECTION("Disconnect non-existent client is safe") {
        manager.disconnectClient(999, "test disconnect");
        REQUIRE(true);
    }

    SECTION("Disconnect all with no connections is safe") {
        manager.disconnectAll("server shutdown");
        REQUIRE(manager.getConnectionCount() == 0);
    }

    SECTION("Disconnect with null reason is safe") {
        manager.disconnectClient(1, nullptr);
        REQUIRE(true);
    }

    manager.shutdown();
}

TEST_CASE("GNSNetworkManager callback registration", "[network][gns]") {
    GNSNetworkManager manager;

    SECTION("Set connected callback") {
        bool callbackSet = false;
        manager.setOnClientConnected([&callbackSet](ConnectionID) {
            callbackSet = true;
        });
        // Callback is stored, not called yet
        REQUIRE_FALSE(callbackSet);
    }

    SECTION("Set disconnected callback") {
        bool callbackSet = false;
        manager.setOnClientDisconnected([&callbackSet](ConnectionID, const char*) {
            callbackSet = true;
        });
        REQUIRE_FALSE(callbackSet);
    }

    SECTION("Set input callback") {
        bool callbackSet = false;
        manager.setOnClientInput([&callbackSet](ConnectionID, const void*, size_t) {
            callbackSet = true;
        });
        REQUIRE_FALSE(callbackSet);
    }

    SECTION("Replace callbacks") {
        manager.setOnClientConnected([](ConnectionID) {});
        manager.setOnClientDisconnected([](ConnectionID, const char*) {});
        manager.setOnClientInput([](ConnectionID, const void*, size_t) {});
        // Should not crash
        REQUIRE(true);
    }
}

TEST_CASE("GNSNetworkManager state consistency", "[network][gns]") {
    SECTION("State after init-shutdown-init cycle") {
        GNSNetworkManager manager;

        manager.initialize(static_cast<uint16_t>(17787));
        REQUIRE(manager.getConnectionCount() == 0);

        manager.shutdown();
        REQUIRE(manager.getConnectionCount() == 0);

        manager.initialize(static_cast<uint16_t>(17788));
        REQUIRE(manager.getConnectionCount() == 0);

        manager.shutdown();
    }

    SECTION("Multiple shutdowns are safe") {
        GNSNetworkManager manager;
        manager.initialize(static_cast<uint16_t>(17789));
        manager.shutdown();
        manager.shutdown();
        manager.shutdown();
        REQUIRE(true);
    }

    SECTION("Update then shutdown is safe") {
        GNSNetworkManager manager;
        manager.initialize(static_cast<uint16_t>(17790));
        manager.update(0);
        manager.update(16);
        manager.update(32);
        manager.shutdown();
        REQUIRE(true);
    }
}

TEST_CASE("GNSNetworkManager dumpStats", "[network][gns]") {
    GNSNetworkManager manager;
    manager.initialize(static_cast<uint16_t>(17791));

    SECTION("Dump stats with no connections") {
        // Should not crash, just prints to stdout
        manager.dumpStats();
        REQUIRE(true);
    }

    manager.shutdown();
}

#endif // ENABLE_GNS
