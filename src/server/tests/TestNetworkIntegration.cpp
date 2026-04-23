// [NETWORK_AGENT] End-to-end UDP network integration tests
// Validates actual socket I/O between a test client and the NetworkManager server
//
// These tests exercise the real NetworkManager_udp.cpp implementation,
// verifying client-server protocol compatibility without requiring Godot.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "netcode/NetworkManager.hpp"
#include "ecs/CoreTypes.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

using namespace DarkAges;

namespace {

// Simple test client using raw BSD sockets
class TestUdpClient {
public:
    int sockfd = -1;
    sockaddr_in serverAddr{};

    bool connect(const char* ip, uint16_t port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) return false;

        // Set non-blocking for recv with timeout
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &serverAddr.sin_addr);
        return true;
    }

    void disconnect() {
        if (sockfd >= 0) {
            close(sockfd);
            sockfd = -1;
        }
    }

    bool send(const uint8_t* data, size_t len) {
        if (sockfd < 0) return false;
        ssize_t sent = ::sendto(sockfd, data, len, 0,
                                reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        return sent == static_cast<ssize_t>(len);
    }

    ssize_t receive(uint8_t* buffer, size_t len, int timeoutMs = 500) {
        if (sockfd < 0) return -1;

        auto start = std::chrono::steady_clock::now();
        while (true) {
            sockaddr_in from{};
            socklen_t fromLen = sizeof(from);
            ssize_t recvd = ::recvfrom(sockfd, buffer, len, 0,
                                       reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (recvd >= 0) return recvd;

            if (errno != EAGAIN && errno != EWOULDBLOCK) return -1;

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeoutMs) return -1;

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    ~TestUdpClient() { disconnect(); }
};

// Packet type constants (must match NetworkManager_udp.cpp and client)
constexpr uint8_t PACKET_CLIENT_INPUT = 1;
constexpr uint8_t PACKET_SNAPSHOT = 2;
constexpr uint8_t PACKET_EVENT = 3;
constexpr uint8_t PACKET_PING = 4;
constexpr uint8_t PACKET_PONG = 5;
constexpr uint8_t PACKET_CONNECTION_REQUEST = 6;
constexpr uint8_t PACKET_CONNECTION_RESPONSE = 7;
constexpr uint8_t PACKET_SERVER_CORRECTION = 8;

// Helper: find an available port starting from base
uint16_t findAvailablePort(uint16_t base) {
    for (uint16_t p = base; p < base + 100; ++p) {
        int testSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (testSock < 0) continue;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(p);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(testSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            close(testSock);
            return p;
        }
        close(testSock);
    }
    return 0;
}

} // namespace

TEST_CASE("Network integration - connection handshake", "[network][integration]") {
    uint16_t port = findAvailablePort(28777);
    REQUIRE(port != 0);

    NetworkManager server;
    std::atomic<bool> connected{false};
    std::atomic<ConnectionID> connId{INVALID_CONNECTION};

    server.setOnClientConnected([&](ConnectionID id) {
        connId.store(id);
        connected.store(true);
    });

    REQUIRE(server.initialize(port));

    TestUdpClient client;
    REQUIRE(client.connect("127.0.0.1", port));

    // Send connection request: [type:1=6][version:4][player_id:4]
    uint8_t req[9];
    req[0] = PACKET_CONNECTION_REQUEST;
    *reinterpret_cast<uint32_t*>(req + 1) = 1; // version
    *reinterpret_cast<uint32_t*>(req + 5) = 12345; // player id
    REQUIRE(client.send(req, sizeof(req)));

    // Wait for server to process and respond
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Receive connection response: [type:1=7][success:1][entity_id:4][zone_id:4]
    uint8_t resp[256];
    ssize_t recvd = client.receive(resp, sizeof(resp), 500);
    REQUIRE(recvd >= 6);
    REQUIRE(resp[0] == PACKET_CONNECTION_RESPONSE);
    REQUIRE(resp[1] == 1); // success

    uint32_t assignedEntity = *reinterpret_cast<uint32_t*>(resp + 2);
    REQUIRE(assignedEntity != 0);

    // Verify server registered connection
    REQUIRE(connected.load());
    REQUIRE(connId.load() != INVALID_CONNECTION);
    REQUIRE(server.getConnectionCount() == 1);

    server.shutdown();
}

TEST_CASE("Network integration - ping/pong roundtrip", "[network][integration]") {
    uint16_t port = findAvailablePort(28877);
    REQUIRE(port != 0);

    NetworkManager server;
    REQUIRE(server.initialize(port));

    TestUdpClient client;
    REQUIRE(client.connect("127.0.0.1", port));

    // First establish connection
    uint8_t req[9];
    req[0] = PACKET_CONNECTION_REQUEST;
    *reinterpret_cast<uint32_t*>(req + 1) = 1;
    *reinterpret_cast<uint32_t*>(req + 5) = 1;
    REQUIRE(client.send(req, sizeof(req)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Drain connection response
    uint8_t drain[256];
    client.receive(drain, sizeof(drain), 200);

    // Send ping: [type:1=4][timestamp:4]
    uint8_t ping[5];
    ping[0] = PACKET_PING;
    *reinterpret_cast<uint32_t*>(ping + 1) = 12345678;
    REQUIRE(client.send(ping, sizeof(ping)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Receive pong: [type:1=5][timestamp:4]
    uint8_t pong[256];
    ssize_t recvd = client.receive(pong, sizeof(pong), 500);
    REQUIRE(recvd == 5);
    REQUIRE(pong[0] == PACKET_PONG);
    uint32_t pongTime = *reinterpret_cast<uint32_t*>(pong + 1);
    REQUIRE(pongTime == 12345678);

    server.shutdown();
}

TEST_CASE("Network integration - client input received by server", "[network][integration]") {
    uint16_t port = findAvailablePort(28977);
    REQUIRE(port != 0);

    NetworkManager server;
    REQUIRE(server.initialize(port));

    TestUdpClient client;
    REQUIRE(client.connect("127.0.0.1", port));

    // Establish connection
    uint8_t req[9];
    req[0] = PACKET_CONNECTION_REQUEST;
    *reinterpret_cast<uint32_t*>(req + 1) = 1;
    *reinterpret_cast<uint32_t*>(req + 5) = 1;
    REQUIRE(client.send(req, sizeof(req)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Drain connection response
    uint8_t drain[256];
    client.receive(drain, sizeof(drain), 200);

    // Clear any pending inputs
    (void)server.getPendingInputs();

    // Send client input: [type:1=1][sequence:4][timestamp:4][flags:1][yaw:2][pitch:2][target:4]
    uint8_t input[18];
    input[0] = PACKET_CLIENT_INPUT;
    *reinterpret_cast<uint32_t*>(input + 1) = 1; // sequence
    *reinterpret_cast<uint32_t*>(input + 5) = 1000; // timestamp
    input[9] = 0x01 | 0x08; // forward + right
    *reinterpret_cast<int16_t*>(input + 10) = 1500; // yaw
    *reinterpret_cast<int16_t*>(input + 12) = 200; // pitch
    *reinterpret_cast<uint32_t*>(input + 14) = 0; // target
    REQUIRE(client.send(input, sizeof(input)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify server received the input
    auto inputs = server.getPendingInputs();
    REQUIRE(inputs.size() >= 1);

    // Find our input
    bool found = false;
    for (const auto& pkt : inputs) {
        if (pkt.input.sequence == 1) {
            found = true;
            REQUIRE(pkt.input.forward == true);
            REQUIRE(pkt.input.right == true);
            REQUIRE(pkt.input.backward == false);
            REQUIRE(pkt.input.left == false);
            REQUIRE(pkt.input.yaw == Catch::Approx(0.15f).margin(0.001f)); // 1500/10000
            REQUIRE(pkt.input.pitch == Catch::Approx(0.02f).margin(0.001f)); // 200/10000
            break;
        }
    }
    REQUIRE(found);

    server.shutdown();
}

TEST_CASE("Network integration - snapshot broadcast received by client", "[network][integration]") {
    uint16_t port = findAvailablePort(29077);
    REQUIRE(port != 0);

    NetworkManager server;
    REQUIRE(server.initialize(port));

    TestUdpClient client;
    REQUIRE(client.connect("127.0.0.1", port));

    // Establish connection
    uint8_t req[9];
    req[0] = PACKET_CONNECTION_REQUEST;
    *reinterpret_cast<uint32_t*>(req + 1) = 1;
    *reinterpret_cast<uint32_t*>(req + 5) = 1;
    REQUIRE(client.send(req, sizeof(req)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Drain connection response
    uint8_t drain[256];
    client.receive(drain, sizeof(drain), 200);

    // Build a test snapshot in client-compatible format
    std::vector<Protocol::EntityStateData> entities;
    Protocol::EntityStateData e;
    e.entity = static_cast<EntityID>(42);
    e.position = Position::fromVec3(glm::vec3(10.0f, 20.0f, 30.0f));
    e.velocity.dx = Constants::Fixed(100);
    e.velocity.dy = Constants::Fixed(0);
    e.velocity.dz = Constants::Fixed(-50);
    e.healthPercent = 85;
    e.animState = 2;
    entities.push_back(e);

    auto snapshot = Protocol::createFullSnapshot(100, 5, entities);
    REQUIRE(snapshot.size() == 13 + 30);

    // Broadcast snapshot to all connections
    server.broadcastSnapshot(snapshot);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Client should receive the snapshot
    uint8_t recvBuf[512];
    ssize_t recvd = client.receive(recvBuf, sizeof(recvBuf), 500);
    REQUIRE(recvd == static_cast<ssize_t>(snapshot.size()));
    REQUIRE(recvBuf[0] == PACKET_SNAPSHOT);

    uint32_t tick = *reinterpret_cast<uint32_t*>(recvBuf + 1);
    uint32_t lastInput = *reinterpret_cast<uint32_t*>(recvBuf + 5);
    uint32_t count = *reinterpret_cast<uint32_t*>(recvBuf + 9);
    REQUIRE(tick == 100);
    REQUIRE(lastInput == 5);
    REQUIRE(count == 1);

    uint32_t entityId = *reinterpret_cast<uint32_t*>(recvBuf + 13);
    REQUIRE(entityId == 42);

    float x, y, z;
    std::memcpy(&x, &recvBuf[17], 4);
    std::memcpy(&y, &recvBuf[21], 4);
    std::memcpy(&z, &recvBuf[25], 4);
    REQUIRE(x == Catch::Approx(10.0f).margin(0.01f));
    REQUIRE(y == Catch::Approx(20.0f).margin(0.01f));
    REQUIRE(z == Catch::Approx(30.0f).margin(0.01f));

    server.shutdown();
}

TEST_CASE("Network integration - multiple clients connect", "[network][integration]") {
    uint16_t port = findAvailablePort(29177);
    REQUIRE(port != 0);

    NetworkManager server;
    std::atomic<int> connectCount{0};
    server.setOnClientConnected([&](ConnectionID) {
        connectCount.fetch_add(1);
    });

    REQUIRE(server.initialize(port));

    TestUdpClient client1, client2;
    REQUIRE(client1.connect("127.0.0.1", port));
    REQUIRE(client2.connect("127.0.0.1", port));

    // Both clients send connection requests
    uint8_t req[9];
    req[0] = PACKET_CONNECTION_REQUEST;
    *reinterpret_cast<uint32_t*>(req + 1) = 1;
    *reinterpret_cast<uint32_t*>(req + 5) = 1;

    REQUIRE(client1.send(req, sizeof(req)));
    REQUIRE(client2.send(req, sizeof(req)));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Both should get connection responses
    uint8_t resp1[256], resp2[256];
    ssize_t r1 = client1.receive(resp1, sizeof(resp1), 500);
    ssize_t r2 = client2.receive(resp2, sizeof(resp2), 500);
    REQUIRE(r1 >= 6);
    REQUIRE(r2 >= 6);
    REQUIRE(resp1[0] == PACKET_CONNECTION_RESPONSE);
    REQUIRE(resp2[0] == PACKET_CONNECTION_RESPONSE);

    // Server should track both connections
    REQUIRE(server.getConnectionCount() == 2);
    REQUIRE(connectCount.load() == 2);

    server.shutdown();
}

TEST_CASE("Network integration - connection timeout", "[network][integration]") {
    uint16_t port = findAvailablePort(29277);
    REQUIRE(port != 0);

    NetworkManager server;
    std::atomic<bool> disconnected{false};
    server.setOnClientDisconnected([&](ConnectionID) {
        disconnected.store(true);
    });

    REQUIRE(server.initialize(port));

    TestUdpClient client;
    REQUIRE(client.connect("127.0.0.1", port));

    // Connect
    uint8_t req[9];
    req[0] = PACKET_CONNECTION_REQUEST;
    *reinterpret_cast<uint32_t*>(req + 1) = 1;
    *reinterpret_cast<uint32_t*>(req + 5) = 1;
    REQUIRE(client.send(req, sizeof(req)));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    uint8_t drain[256];
    client.receive(drain, sizeof(drain), 200);

    REQUIRE(server.getConnectionCount() == 1);

    // Stop sending any packets. Server timeout is 30 seconds,
    // so we manually disconnect to test the disconnect path.
    server.disconnect(1, "test disconnect");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(server.getConnectionCount() == 0);
    REQUIRE(disconnected.load());

    server.shutdown();
}
