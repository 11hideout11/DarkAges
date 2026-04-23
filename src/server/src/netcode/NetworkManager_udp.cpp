// Real UDP implementation for NetworkManager (no GNS dependency)
// Compatible with DarkAges client custom binary protocol
//
// Client packet formats:
//   Connection request: [type:1=6][version:4][player_id:4]
//   Input:              [type:1=1][sequence:4][timestamp:4][flags:1][yaw:2][pitch:2][target:4]
//   Ping:               [type:1=4][timestamp:4]
//
// Server packet formats:
//   Snapshot:           [type:1=2][server_tick:4][last_input:4][entity_count:4][entity_data...]
//                       Each entity: [id:4][x:4][y:4][z:4][vx:4][vy:4][vz:4][health:1][anim:1]
//   Pong:               [type:1=5][timestamp:4]
//   Connection response: [type:1=7][success:1][entity_id:4][zone_id:4]
//   Event:              [type:1=3][event_type:1][payload...]
//   Correction:         [type:1=8][server_tick:4][pos_x:4][pos_y:4][pos_z:4][vel_x:4][vel_y:4][vel_z:4][last_input:4]

#include "netcode/NetworkManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <mutex>
#include <queue>
#include <atomic>

namespace DarkAges {

// Internal UDP connection state
struct UDPConnection {
    ConnectionID id{INVALID_CONNECTION};
    sockaddr_in addr{};
    uint32_t lastSeenMs{0};
    uint32_t lastInputSequence{0};
    ConnectionQuality quality{};
    bool connected{false};
    uint32_t connectTimeMs{0};
    std::string ipAddress;
    EntityID entityId{entt::null};  // Assigned entity in zone
};

struct GNSInternal {
    int sockfd{-1};
    uint16_t port{0};
    ConnectionID nextConnectionId{1};
    std::unordered_map<ConnectionID, UDPConnection> connections;
    std::unordered_map<std::string, ConnectionID> addrToConnection;
    std::recursive_mutex connectionsMutex;
    std::atomic<bool> running{false};
    std::atomic<uint64_t> totalBytesSent{0};
    std::atomic<uint64_t> totalBytesReceived{0};
    std::queue<ClientInputPacket> inputQueue;
    std::mutex inputMutex;
    uint32_t currentTimeMs{0};
};

NetworkManager::NetworkManager()
    : internal_(std::make_unique<GNSInternal>())
    , ddosProtection_() {}

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::initialize(uint16_t port) {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    udp->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp->sockfd < 0) {
        std::cerr << "[UDP] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }

    // Set non-blocking
    int flags = fcntl(udp->sockfd, F_GETFL, 0);
    fcntl(udp->sockfd, F_SETFL, flags | O_NONBLOCK);

    // Allow address reuse
    int reuse = 1;
    setsockopt(udp->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp->sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[UDP] Failed to bind to port " << port << ": " << strerror(errno) << std::endl;
        close(udp->sockfd);
        udp->sockfd = -1;
        return false;
    }

    udp->port = port;
    udp->running = true;
    udp->currentTimeMs = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    // NOTE: All network I/O is driven by the main thread via updateNetwork()
    // to ensure EnTT registry access stays single-threaded.

    initialized_ = true;
    std::cout << "[UDP] NetworkManager initialized on port " << port << std::endl;
    return true;
}

void NetworkManager::shutdown() {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    if (!udp->running) return;

    udp->running = false;

    if (udp->sockfd >= 0) {
        close(udp->sockfd);
        udp->sockfd = -1;
    }

    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    udp->connections.clear();
    udp->addrToConnection.clear();

    initialized_ = false;
    std::cout << "[UDP] NetworkManager shutdown complete" << std::endl;
}

void NetworkManager::update(uint32_t currentTimeMs) {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    if (udp->sockfd < 0) return;
    udp->currentTimeMs = currentTimeMs;

    // Receive all pending packets
    uint8_t buffer[2048];
    sockaddr_in fromAddr{};
    socklen_t fromLen = sizeof(fromAddr);

    while (true) {
        ssize_t received = recvfrom(udp->sockfd, buffer, sizeof(buffer), 0,
                                    reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            std::cerr << "[UDP] recvfrom error: " << strerror(errno) << std::endl;
            break;
        }
        if (received < 1) continue;

        udp->totalBytesReceived += static_cast<uint64_t>(received);

        std::string addrKey = std::string(inet_ntoa(fromAddr.sin_addr)) + ":" +
                              std::to_string(ntohs(fromAddr.sin_port));

        // Find or create connection
        ConnectionID connId = INVALID_CONNECTION;
        {
            std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
            auto it = udp->addrToConnection.find(addrKey);
            if (it != udp->addrToConnection.end()) {
                connId = it->second;
                auto connIt = udp->connections.find(connId);
                if (connIt != udp->connections.end()) {
                    connIt->second.lastSeenMs = currentTimeMs;
                    connIt->second.quality.packetsReceived++;
                    connIt->second.quality.bytesReceived += static_cast<uint64_t>(received);
                }
            }
        }

        uint8_t packetType = buffer[0];

        // Connection request
        if (packetType == 6) { // PACKET_CONNECTION_REQUEST
            if (received < 9) continue;
            uint32_t version = *reinterpret_cast<uint32_t*>(buffer + 1);
            uint32_t playerId = *reinterpret_cast<uint32_t*>(buffer + 5);
            (void)version; (void)playerId;

            // DDoS check
            if (!shouldAcceptConnection(addrKey)) continue;

            // Create new connection
            std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
            if (udp->addrToConnection.count(addrKey) == 0) {
                connId = udp->nextConnectionId++;
                UDPConnection conn;
                conn.id = connId;
                conn.addr = fromAddr;
                conn.connected = true;
                conn.lastSeenMs = currentTimeMs;
                conn.connectTimeMs = currentTimeMs;
                conn.ipAddress = addrKey;
                conn.quality.packetsReceived = 1;
                conn.quality.bytesReceived = static_cast<uint64_t>(received);
                udp->connections[connId] = conn;
                udp->addrToConnection[addrKey] = connId;

                std::cout << "[UDP] Client connected: " << addrKey
                          << " (connId=" << connId << ")" << std::endl;

                if (onConnected_) {
                    onConnected_(connId);
                }

                // Send connection response
                uint8_t response[10];
                response[0] = 7; // PACKET_CONNECTION_RESPONSE
                response[1] = 1; // success
                uint32_t entityIdToSend = static_cast<uint32_t>(udp->connections[connId].entityId);
                if (entityIdToSend == 0) {
                    entityIdToSend = connId; // fallback if ZoneServer hasn't assigned yet
                }
                *reinterpret_cast<uint32_t*>(response + 2) = entityIdToSend; // actual entity id
                *reinterpret_cast<uint32_t*>(response + 6) = 1; // zone id
                sendto(udp->sockfd, response, sizeof(response), 0,
                       reinterpret_cast<sockaddr*>(&fromAddr), fromLen);
                udp->totalBytesSent += sizeof(response);
            }
            continue;
        }

        // Must have a valid connection for all other packet types
        if (connId == INVALID_CONNECTION) continue;

        // DDoS rate limiting
        if (!processPacket(connId, addrKey, static_cast<uint32_t>(received), currentTimeMs)) {
            continue;
        }

        switch (packetType) {
            case 1: { // PACKET_CLIENT_INPUT
                if (received < 18) break;
                ClientInputPacket pkt;
                pkt.connectionId = connId;
                pkt.receiveTimeMs = currentTimeMs;

                // Parse client's input format:
                // [type:1][sequence:4][timestamp:4][flags:1][yaw:2][pitch:2][target:4]
                uint32_t sequence = *reinterpret_cast<uint32_t*>(buffer + 1);
                uint32_t timestamp = *reinterpret_cast<uint32_t*>(buffer + 5);
                uint8_t flags = buffer[9];
                int16_t yawQ = *reinterpret_cast<int16_t*>(buffer + 10);
                int16_t pitchQ = *reinterpret_cast<int16_t*>(buffer + 12);
                uint32_t target = *reinterpret_cast<uint32_t*>(buffer + 14);

                pkt.input.sequence = sequence;
                pkt.input.timestamp_ms = timestamp;
                pkt.input.forward = (flags >> 0) & 1;
                pkt.input.backward = (flags >> 1) & 1;
                pkt.input.left = (flags >> 2) & 1;
                pkt.input.right = (flags >> 3) & 1;
                pkt.input.jump = (flags >> 4) & 1;
                pkt.input.attack = (flags >> 5) & 1;
                pkt.input.block = (flags >> 6) & 1;
                pkt.input.sprint = (flags >> 7) & 1;
                pkt.input.yaw = static_cast<float>(yawQ) / 10000.0f;
                pkt.input.pitch = static_cast<float>(pitchQ) / 10000.0f;
                pkt.input.targetEntity = target;

                {
                    std::lock_guard<std::mutex> lock(udp->inputMutex);
                    udp->inputQueue.push(pkt);
                }

                if (onInput_) {
                    onInput_(pkt);
                }
                break;
            }
            case 4: { // PACKET_PING
                if (received < 5) break;
                uint32_t pingTime = *reinterpret_cast<uint32_t*>(buffer + 1);
                // Send pong
                uint8_t pong[5];
                pong[0] = 5; // PACKET_PONG
                *reinterpret_cast<uint32_t*>(pong + 1) = pingTime;
                sendto(udp->sockfd, pong, sizeof(pong), 0,
                       reinterpret_cast<sockaddr*>(&fromAddr), fromLen);
                udp->totalBytesSent += sizeof(pong);

                // Update RTT estimate (simplified)
                {
                    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
                    auto it = udp->connections.find(connId);
                    if (it != udp->connections.end()) {
                        uint32_t rtt = (currentTimeMs > pingTime) ? (currentTimeMs - pingTime) : 1;
                        it->second.quality.rttMs = rtt;
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    // Timeout old connections (30 seconds without any packet)
    {
        std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
        for (auto it = udp->connections.begin(); it != udp->connections.end();) {
            if (currentTimeMs - it->second.lastSeenMs > 30000) {
                std::cout << "[UDP] Connection timed out: " << it->second.ipAddress << std::endl;
                if (onDisconnected_) {
                    onDisconnected_(it->first);
                }
                udp->addrToConnection.erase(it->second.ipAddress);
                it = udp->connections.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::vector<ClientInputPacket> NetworkManager::getPendingInputs() {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::vector<ClientInputPacket> inputs;
    std::lock_guard<std::mutex> lock(udp->inputMutex);
    while (!udp->inputQueue.empty()) {
        inputs.push_back(udp->inputQueue.front());
        udp->inputQueue.pop();
    }
    return inputs;
}

void NetworkManager::clearProcessedInputs(uint32_t upToSequence) {
    (void)upToSequence;
    // Simplified: inputs are already removed in getPendingInputs
}

void NetworkManager::sendSnapshot(ConnectionID connectionId, std::span<const uint8_t> data) {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    if (udp->sockfd < 0) return;

    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    auto it = udp->connections.find(connectionId);
    if (it == udp->connections.end()) return;

    ssize_t sent = sendto(udp->sockfd, data.data(), data.size(), 0,
                          reinterpret_cast<sockaddr*>(&it->second.addr), sizeof(it->second.addr));
    if (sent > 0) {
        udp->totalBytesSent += static_cast<uint64_t>(sent);
        it->second.quality.packetsSent++;
        it->second.quality.bytesSent += static_cast<uint64_t>(sent);
    }
}

void NetworkManager::sendEvent(ConnectionID connectionId, std::span<const uint8_t> data) {
    sendSnapshot(connectionId, data); // UDP: same path
}

void NetworkManager::broadcastSnapshot(std::span<const uint8_t> data) {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    if (udp->sockfd < 0) return;

    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    for (auto& [id, conn] : udp->connections) {
        if (!conn.connected) continue;
        ssize_t sent = sendto(udp->sockfd, data.data(), data.size(), 0,
                              reinterpret_cast<sockaddr*>(&conn.addr), sizeof(conn.addr));
        if (sent > 0) {
            udp->totalBytesSent += static_cast<uint64_t>(sent);
            conn.quality.packetsSent++;
            conn.quality.bytesSent += static_cast<uint64_t>(sent);
        }
    }
}

void NetworkManager::broadcastEvent(std::span<const uint8_t> data) {
    broadcastSnapshot(data);
}

void NetworkManager::sendToMultiple(const std::vector<ConnectionID>& conns, std::span<const uint8_t> data) {
    for (ConnectionID conn : conns) {
        sendSnapshot(conn, data);
    }
}

void NetworkManager::disconnect(ConnectionID connectionId, const char* reason) {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    auto it = udp->connections.find(connectionId);
    if (it != udp->connections.end()) {
        // Send disconnect event if reason provided
        if (reason && udp->sockfd >= 0) {
            uint8_t disc[2];
            disc[0] = 6; // PACKET_DISCONNECT
            disc[1] = static_cast<uint8_t>(std::strlen(reason));
            sendto(udp->sockfd, disc, sizeof(disc), 0,
                   reinterpret_cast<sockaddr*>(&it->second.addr), sizeof(it->second.addr));
        }

        std::cout << "[UDP] Disconnecting client: " << it->second.ipAddress
                  << " (reason: " << (reason ? reason : "none") << ")" << std::endl;

        if (onDisconnected_) {
            onDisconnected_(connectionId);
        }
        udp->addrToConnection.erase(it->second.ipAddress);
        udp->connections.erase(it);
    }
}

void NetworkManager::setConnectionEntityId(ConnectionID connectionId, EntityID entityId) {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    auto it = udp->connections.find(connectionId);
    if (it != udp->connections.end()) {
        it->second.entityId = entityId;
    }
}

bool NetworkManager::isConnected(ConnectionID connectionId) const {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    auto it = udp->connections.find(connectionId);
    return it != udp->connections.end() && it->second.connected;
}

ConnectionQuality NetworkManager::getConnectionQuality(ConnectionID connectionId) const {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    auto it = udp->connections.find(connectionId);
    if (it != udp->connections.end()) {
        return it->second.quality;
    }
    return {};
}

size_t NetworkManager::getConnectionCount() const {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    return udp->connections.size();
}

uint64_t NetworkManager::getTotalBytesSent() const {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    return udp->totalBytesSent.load();
}

uint64_t NetworkManager::getTotalBytesReceived() const {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    return udp->totalBytesReceived.load();
}

bool NetworkManager::isRateLimited(ConnectionID connectionId) const {
    auto* udp = static_cast<GNSInternal*>(internal_.get());
    std::lock_guard<std::recursive_mutex> lock(udp->connectionsMutex);
    auto it = udp->connections.find(connectionId);
    if (it == udp->connections.end()) return false;

    // Simple rate limit: >100 packets/sec
    uint32_t elapsed = udp->currentTimeMs - it->second.connectTimeMs;
    if (elapsed == 0) return false;
    float pps = static_cast<float>(it->second.quality.packetsReceived) * 1000.0f / elapsed;
    return pps > 100.0f;
}

bool NetworkManager::shouldAcceptConnection(const std::string& ipAddress) {
    return ddosProtection_.shouldAcceptConnection(ipAddress);
}

bool NetworkManager::processPacket(ConnectionID connectionId,
                                   const std::string& ipAddress,
                                   uint32_t packetSize,
                                   uint32_t currentTimeMs) {
    return ddosProtection_.processPacket(connectionId, ipAddress, packetSize, currentTimeMs);
}

} // namespace DarkAges
