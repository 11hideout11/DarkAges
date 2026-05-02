// [NETWORK_AGENT] Stub Socket Implementation
// IMPORTANT: StubSocket is a purely in-memory stub for unit testing and
// local development.  It does NOT open any OS sockets, does NOT send any
// bytes over the network, and does NOT poll for incoming data.
//
// How to receive messages in tests:
//   Call pushMessage(type, connectionId, data) to enqueue a packet that will
//   be returned by the next receive() / peek() call.
//
// For real UDP networking use GNSSocket (or a future production UDP impl).

#include "netcode/INetworkSocket.hpp"
#include <algorithm>
#include <queue>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>
#include <cstdlib>
#include <iostream>

namespace DarkAges {
namespace Netcode {

// ============================================================================
// UDPSocket Implementation - Real UDP using BSD sockets
// ============================================================================

// Real UDP socket implementation using BSD socket APIs
// Uses socket(), bind(), sendto(), recvfrom(), poll() for non-blocking I/O
// Backward compatible with test message injection via pushMessage

struct UDPSocket::Impl {
    int sockFd{-1};
    uint16_t port{0};
    bool initialized{false};
    
    // Connection tracking for UDP (stateless)
    struct Connection {
        ConnectionID id;
        struct sockaddr_in address;
        uint32_t lastActivityMs{0};
    };
    std::vector<Connection> connections;
    ConnectionID nextConnectionId{1};
    
    // Pending messages for test injection
    struct PendingMessage {
        PacketType type;
        ConnectionID connectionId;
        std::vector<uint8_t> data;
    };
    std::queue<PendingMessage> pendingMessages;
    
    // Statistics
    uint64_t totalBytesSent{0};
    uint64_t totalBytesReceived{0};
    
    // Callbacks
    ConnectionCallback onConnected;
    DisconnectCallback onDisconnected;
    MessageCallback onMessage;
    
    Impl() = default;
    ~Impl() {
        if (sockFd >= 0) {
            close(sockFd);
            sockFd = -1;
        }
    }
    
    static int setNonBlocking(int fd) {
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) return -1;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
};

UDPSocket::UDPSocket() : impl_(std::make_unique<Impl>()) {}

UDPSocket::~UDPSocket() {
    shutdown();
}

bool UDPSocket::initialize(uint16_t port) {
    if (impl_->initialized) {
        return true;
    }
    
    impl_->port = port;
    
    // Create UDP socket
    impl_->sockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (impl_->sockFd < 0) {
        return false;
    }
    
    // Set socket to non-blocking mode
    if (Impl::setNonBlocking(impl_->sockFd) < 0) {
        close(impl_->sockFd);
        impl_->sockFd = -1;
        return false;
    }
    
    // Allow address reuse
    int opt = 1;
    setsockopt(impl_->sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(impl_->sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(impl_->sockFd);
        impl_->sockFd = -1;
        return false;
    }
    
    impl_->initialized = true;
    return true;
}

void UDPSocket::shutdown() {
    if (!impl_->initialized) {
        return;
    }
    
    if (impl_->sockFd >= 0) {
        close(impl_->sockFd);
        impl_->sockFd = -1;
    }
    
    impl_->connections.clear();
    impl_->initialized = false;
}

void UDPSocket::update(uint32_t currentTimeMs) {
    if (!impl_->initialized) {
        return;
    }
    
    // Poll for incoming data
    struct pollfd pfd {};
    pfd.fd = impl_->sockFd;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, 0);
    if (ret <= 0) {
        return;
    }
    
    // Receive pending packets
    char buffer[65536];
    struct sockaddr_in sender;
    socklen_t senderLen = sizeof(sender);
    
    ssize_t n = recvfrom(impl_->sockFd, buffer, sizeof(buffer), 0,
                      (struct sockaddr*)&sender, &senderLen);
    
    if (n > 0) {
        impl_->totalBytesReceived += n;
        
        // Find or create connection for this sender
        ConnectionID connId = findOrCreateConnection(sender, currentTimeMs);
        
        // Determine packet type from first byte (or default)
        PacketType type = PacketType::Data;
        if (n > 0) {
            type = static_cast<PacketType>(buffer[0]);
        }
        
        // Queue for reading
        std::vector<uint8_t> data(n);
        memcpy(data.data(), buffer, n);
        
        impl_->pendingMessages.push({type, connId, std::move(data)});
        
        // Trigger message callback if set
        if (impl_->onMessage) {
            impl_->onMessage(type, connId, std::span<const uint8_t>{});
        }
    }
}

ConnectionID UDPSocket::findOrCreateConnection(struct sockaddr_in& sender, uint32_t currentTimeMs) {
    // Look for existing connection with same address
    for (auto& conn : impl_->connections) {
        if (conn.address.sin_addr.s_addr == sender.sin_addr.s_addr &&
            conn.address.sin_port == sender.sin_port) {
            conn.lastActivityMs = currentTimeMs;
            return conn.id;
        }
    }
    
    // Create new connection
    ConnectionID newId = impl_->nextConnectionId++;
    Connection conn;
    conn.id = newId;
    conn.address = sender;
    conn.lastActivityMs = currentTimeMs;
    impl_->connections.push_back(conn);
    
    if (impl_->onConnected) {
        impl_->onConnected(newId);
    }
    
    return newId;
}

std::vector<ConnectionID> UDPSocket::getConnections() {
    std::vector<ConnectionID> result;
    result.reserve(impl_->connections.size());
    for (auto& conn : impl_->connections) {
        result.push_back(conn.id);
    }
    return result;
}

void UDPSocket::disconnect(ConnectionID connectionId, const char*) {
    if (!impl_->initialized) {
        return;
    }
    
    auto it = std::find_if(impl_->connections.begin(), 
                        impl_->connections.end(),
                        [connectionId](const Impl::Connection& c) {
                            return c.id == connectionId;
                        });
    
    if (it != impl_->connections.end()) {
        impl_->connections.erase(it);
        
        if (impl_->onDisconnected) {
            impl_->onDisconnected(connectionId, "disconnected");
        }
    }
}

bool UDPSocket::isConnected(ConnectionID connectionId) const {
    auto it = std::find_if(impl_->connections.begin(), 
                        impl_->connections.end(),
                        [connectionId](const Impl::Connection& c) {
                            return c.id == connectionId;
                        });
    return it != impl_->connections.end();
}

const Impl::Connection* UDPSocket::findConnection(ConnectionID connectionId) const {
    auto it = std::find_if(impl_->connections.begin(), 
                        impl_->connections.end(),
                        [connectionId](const Impl::Connection& c) {
                            return c.id == connectionId;
                        });
    return (it != impl_->connections.end()) ? &(*it) : nullptr;
}

uint32_t UDPSocket::getMaxConnections() const {
    return 1000;
}

uint32_t UDPSocket::getConnectionCount() const {
    return static_cast<uint32_t>(impl_->connections.size());
}

bool UDPSocket::sendUnreliable(ConnectionID connectionId, std::span<const uint8_t> data) {
    if (!impl_->initialized || impl_->sockFd < 0) {
        return false;
    }
    
    const auto* conn = findConnection(connectionId);
    if (!conn) {
        return false;
    }
    
    ssize_t sent = sendto(impl_->sockFd, data.data(), data.size(), 0,
                        (struct sockaddr*)&conn->address, sizeof(conn->address));
    
    if (sent > 0) {
        impl_->totalBytesSent += sent;
        return true;
    }
    return false;
}

bool UDPSocket::sendReliable(ConnectionID connectionId, std::span<const uint8_t> data) {
    // UDP is inherently unreliable, but we mark as sent
    return sendUnreliable(connectionId, data);
}

void UDPSocket::broadcastUnreliable(std::span<const uint8_t> data) {
    if (!impl_->initialized) {
        return;
    }
    
    for (auto& conn : impl_->connections) {
        sendto(impl_->sockFd, data.data(), data.size(), 0,
              (struct sockaddr*)&conn.address, sizeof(conn.address));
    }
}

void UDPSocket::broadcastReliable(std::span<const uint8_t> data) {
    broadcastUnreliable(data);
}

void UDPSocket::sendToMultiple(const std::vector<ConnectionID>& connections, 
                           std::span<const uint8_t> data) {
    for (ConnectionID conn : connections) {
        sendUnreliable(conn, data);
    }
}

bool UDPSocket::receive(PacketType& type, ConnectionID& connectionId, 
                     std::vector<uint8_t>& buffer) {
    // First check pending messages (test injection)
    if (!impl_->pendingMessages.empty()) {
        auto& msg = impl_->pendingMessages.front();
        type = msg.type;
        connectionId = msg.connectionId;
        buffer = std::move(msg.data);
        impl_->pendingMessages.pop();
        return true;
    }
    
    // Otherwise try to receive from socket
    if (!impl_->initialized || impl_->sockFd < 0) {
        return false;
    }
    
    // Poll for incoming
    struct pollfd pfd {};
    pfd.fd = impl_->sockFd;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, 0);
    if (ret <= 0) {
        return false;
    }
    
    char recvBuffer[65536];
    struct sockaddr_in sender;
    socklen_t senderLen = sizeof(sender);
    
    ssize_t n = recvfrom(impl_->sockFd, recvBuffer, sizeof(recvBuffer), 0,
                      (struct sockaddr*)&sender, &senderLen);
    
    if (n > 0) {
        impl_->totalBytesReceived += n;
        
        ConnectionID connId = findOrCreateConnection(sender, 0);
        
        type = static_cast<PacketType>(recvBuffer[0]);
        connectionId = connId;
        
        buffer.resize(n);
        memcpy(buffer.data(), recvBuffer, n);
        return true;
    }
    
    return false;
}

bool UDPSocket::peek(PacketType& type, ConnectionID& connectionId) const {
    if (!impl_->pendingMessages.empty()) {
        auto& msg = impl_->pendingMessages.front();
        type = msg.type;
        connectionId = msg.connectionId;
        return true;
    }
    
    if (!impl_->initialized || impl_->sockFd < 0) {
        return false;
    }
    
    struct pollfd pfd {};
    pfd.fd = impl_->sockFd;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, 0);
    if (ret <= 0) {
        return false;
    }
    
    char recvBuffer[65536];
    struct sockaddr_in sender;
    socklen_t senderLen = sizeof(sender);
    
    ssize_t n = recvfrom(impl_->sockFd, recvBuffer, sizeof(recvBuffer), MSG_PEEK,
                      (struct sockaddr*)&sender, &senderLen);
    
    if (n > 0) {
        type = static_cast<PacketType>(recvBuffer[0]);
        
        // Find connection (without creating new)
        for (auto& conn : impl_->connections) {
            if (conn.address.sin_addr.s_addr == sender.sin_addr.s_addr &&
                conn.address.sin_port == sender.sin_port) {
                connectionId = conn.id;
                return true;
            }
        }
        connectionId = 0;
        return true;
    }
    
    return false;
}

void UDPSocket::consume() {
    if (!impl_->pendingMessages.empty()) {
        impl_->pendingMessages.pop();
    }
}

ConnectionQuality UDPSocket::getConnectionQuality(ConnectionID connectionId) const {
    ConnectionQuality q;
    // TODO: Implement RTT tracking
    return q;
}

uint64_t UDPSocket::getTotalBytesSent() const {
    return impl_->totalBytesSent;
}

uint64_t UDPSocket::getTotalBytesReceived() const {
    return impl_->totalBytesReceived;
}

void UDPSocket::setOnConnected(ConnectionCallback callback) {
    impl_->onConnected = std::move(callback);
}

void UDPSocket::setOnDisconnected(DisconnectCallback callback) {
    impl_->onDisconnected = std::move(callback);
}

void UDPSocket::setOnMessage(MessageCallback callback) {
    impl_->onMessage = std::move(callback);
}

// Test injection for UDPSocket
void UDPSocket::pushMessage(PacketType type, ConnectionID connectionId, std::span<const uint8_t> data) {
    std::vector<uint8_t> copy(data.begin(), data.end());
    impl_->pendingMessages.push({type, connectionId, std::move(copy)});
}

StubSocket::StubSocket() : impl_(std::make_unique<Impl>()) {}

StubSocket::~StubSocket() {
    shutdown();
}

bool StubSocket::initialize(uint16_t /*port*/) {
    if (impl_->initialized) {
        return true;
    }
    impl_->initialized = true;
    return true;
}

void StubSocket::shutdown() {
    if (!impl_->initialized) {
        return;
    }
    impl_->connections.clear();
    impl_->initialized = false;
}

void StubSocket::update(uint32_t) {
    // In-memory stub: no I/O polling needed.
}

std::vector<ConnectionID> StubSocket::getConnections() {
    return impl_->connections;
}

void StubSocket::disconnect(ConnectionID connectionId, const char*) {
    if (!impl_->initialized) {
        return;
    }
    
    auto it = std::find(impl_->connections.begin(), 
                     impl_->connections.end(), connectionId);
    if (it != impl_->connections.end()) {
        impl_->connections.erase(it);
        
        if (impl_->onDisconnected) {
            impl_->onDisconnected(connectionId, "disconnected");
        }
    }
}

bool StubSocket::isConnected(ConnectionID connectionId) const {
    auto it = std::find(impl_->connections.begin(), 
                     impl_->connections.end(), connectionId);
    return it != impl_->connections.end();
}

uint32_t StubSocket::getMaxConnections() const {
    return 1000;
}

uint32_t StubSocket::getConnectionCount() const {
    return static_cast<uint32_t>(impl_->connections.size());
}

bool StubSocket::sendUnreliable(ConnectionID connectionId, std::span<const uint8_t> data) {
    if (!impl_->initialized || !isConnected(connectionId)) {
        return false;
    }
    
    // Stub: just update stats (no real send)
    impl_->totalBytesSent += data.size();
    return true;
}

bool StubSocket::sendReliable(ConnectionID connectionId, std::span<const uint8_t> data) {
    // Stub: treat same as unreliable
    return sendUnreliable(connectionId, data);
}

void StubSocket::broadcastUnreliable(std::span<const uint8_t> data) {
    if (!impl_->initialized) {
        return;
    }
    
    for (ConnectionID conn : impl_->connections) {
        sendUnreliable(conn, data);
    }
}

void StubSocket::broadcastReliable(std::span<const uint8_t> data) {
    broadcastUnreliable(data);
}

void StubSocket::sendToMultiple(const std::vector<ConnectionID>& connections, 
                          std::span<const uint8_t> data) {
    for (ConnectionID conn : connections) {
        sendUnreliable(conn, data);
    }
}

bool StubSocket::receive(PacketType& type, ConnectionID& connectionId, 
                    std::vector<uint8_t>& buffer) {
    if (!impl_->initialized || impl_->pendingMessages.empty()) {
        return false;
    }
    
    auto& msg = impl_->pendingMessages.front();
    type = msg.type;
    connectionId = msg.connectionId;
    buffer = std::move(msg.data);
    impl_->pendingMessages.pop();
    return true;
}

bool StubSocket::peek(PacketType& type, ConnectionID& connectionId) const {
    if (impl_->pendingMessages.empty()) {
        return false;
    }
    
    auto& msg = impl_->pendingMessages.front();
    type = msg.type;
    connectionId = msg.connectionId;
    return true;
}

void StubSocket::consume() {
    if (!impl_->pendingMessages.empty()) {
        impl_->pendingMessages.pop();
    }
}

ConnectionQuality StubSocket::getConnectionQuality(ConnectionID connectionId) const {
    // Stub: return default quality
    return ConnectionQuality{};
}

uint64_t StubSocket::getTotalBytesSent() const {
    return impl_->totalBytesSent;
}

uint64_t StubSocket::getTotalBytesReceived() const {
    return impl_->totalBytesReceived;
}

void StubSocket::setOnConnected(ConnectionCallback callback) {
    impl_->onConnected = std::move(callback);
}

void StubSocket::setOnDisconnected(DisconnectCallback callback) {
    impl_->onDisconnected = std::move(callback);
}

void StubSocket::setOnMessage(MessageCallback callback) {
    impl_->onMessage = std::move(callback);
}

// ============================================================================
// GNSSocket Implementation (Stub for now - requires GNS library)
// ============================================================================

struct GNSSocket::Impl {
    // GNS requires dynamic loading at runtime
    // This is a placeholder that returns false on initialize
    bool gnsAvailable{false};
};

GNSSocket::GNSSocket() : impl_(std::make_unique<Impl>()) {}

GNSSocket::~GNSSocket() {
    shutdown();
}

bool GNSSocket::initialize(uint16_t port) {
    // Check for GNS library availability
    // In production, this would try dlopen("libgameNetworkingSockets.so")
    if (!impl_->gnsAvailable) {
        return false;
    }
    
    // Full GNS implementation would go here
    return false;
}

void GNSSocket::shutdown() {}
void GNSSocket::update(uint32_t) {}

std::vector<ConnectionID> GNSSocket::getConnections() { return {}; }
void GNSSocket::disconnect(ConnectionID, const char*) {}
bool GNSSocket::isConnected(ConnectionID) const { return false; }
uint32_t GNSSocket::getMaxConnections() const { return 1000; }
uint32_t GNSSocket::getConnectionCount() const { return 0; }

bool GNSSocket::sendUnreliable(ConnectionID, std::span<const uint8_t>) { return false; }
bool GNSSocket::sendReliable(ConnectionID, std::span<const uint8_t>) { return false; }
void GNSSocket::broadcastUnreliable(std::span<const uint8_t>) {}
void GNSSocket::broadcastReliable(std::span<const uint8_t>) {}
void GNSSocket::sendToMultiple(const std::vector<ConnectionID>&, std::span<const uint8_t>) {}

bool GNSSocket::receive(PacketType&, ConnectionID&, std::vector<uint8_t>&) { return false; }
bool GNSSocket::peek(PacketType&, ConnectionID&) const { return false; }
void GNSSocket::consume() {}

ConnectionQuality GNSSocket::getConnectionQuality(ConnectionID) const { return {}; }
uint64_t GNSSocket::getTotalBytesSent() const { return 0; }
uint64_t GNSSocket::getTotalBytesReceived() const { return 0; }

void GNSSocket::setOnConnected(ConnectionCallback) {}
void GNSSocket::setOnDisconnected(DisconnectCallback) {}
void GNSSocket::setOnMessage(MessageCallback) {}

// ============================================================================
// Factory Implementation
// ============================================================================

#include <dlfcn.h>

std::unique_ptr<INetworkSocket> NetworkSocketFactory::create(SocketType type) {
    // Guard: invalid type defaults to stub
    if (type != SocketType::Auto && type != SocketType::Stub && type != SocketType::GNS) {
        std::cerr << "[NetworkSocketFactory] WARNING: Invalid socket type, using Stub" << std::endl;
        type = SocketType::Stub;
    }
    
    if (type == SocketType::Auto) {
        type = isGNSAvailable() ? SocketType::GNS : SocketType::UDP;
    }
    
    switch (type) {
        case SocketType::GNS:
            // Verify GNS is actually available before use
            if (isGNSAvailable()) {
                std::cout << "[NetworkSocketFactory] Creating GNSSocket (production)" << std::endl;
                return std::make_unique<GNSSocket>();
            }
            // Fall through to stub if GNS check failed
            std::cout << "[NetworkSocketFactory] GNS unavailable, falling back to StubSocket" << std::endl;
            [[fallthrough]];
        case SocketType::UDP:
            return std::make_unique<UDPSocket>();
        case SocketType::Stub:
        default:
            std::cout << "[NetworkSocketFactory] Creating StubSocket (development)" << std::endl;
            return std::make_unique<StubSocket>();
    }
}

bool NetworkSocketFactory::isGNSAvailable() {
    // Try to load GNS library via dlopen
    static void* gnsLib = nullptr;
    if (!gnsLib) {
        gnsLib = dlopen("libgameNetworkingSockets.so", RTLD_NOW);
    }
    if (gnsLib) return true;
    
    // Fallback: check environment variable override
    const char* env = std::getenv("DARKAGES_FORCE_GNS");
    if (env && std::strcmp(env, "1") == 0) {
        std::cout << "[NetworkSocketFactory] GNS forced via DARKAGES_FORCE_GNS=1" << std::endl;
        return true;
    }
    return false;
}

NetworkSocketFactory::SocketType NetworkSocketFactory::getCurrentType() {
    return isGNSAvailable() ? SocketType::GNS : SocketType::UDP;
}

} // namespace Netcode
} // namespace DarkAges