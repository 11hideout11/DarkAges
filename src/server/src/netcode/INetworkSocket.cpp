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

namespace DarkAges {
namespace Netcode {

// ============================================================================
// StubSocket Implementation
// ============================================================================

struct StubSocket::Impl {
    bool initialized{false};
    
    std::vector<ConnectionID> connections;
    ConnectionID nextConnectionId{1};
    
    // Pending messages queue (populated via pushMessage for tests)
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
    ~Impl() = default;
};

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

std::unique_ptr<INetworkSocket> NetworkSocketFactory::create(SocketType type) {
    if (type == SocketType::Auto) {
        type = isGNSAvailable() ? SocketType::GNS : SocketType::Stub;
    }
    
    switch (type) {
        case SocketType::GNS:
            return std::make_unique<GNSSocket>();
        case SocketType::Stub:
        default:
            return std::make_unique<StubSocket>();
    }
}

bool NetworkSocketFactory::isGNSAvailable() {
    // In production, check for GNS library
    // dlopen("libgameNetworkingSockets.so") would be called
    // For now, return false to use stub
    return false;
}

NetworkSocketFactory::SocketType NetworkSocketFactory::getCurrentType() {
    return isGNSAvailable() ? SocketType::GNS : SocketType::Stub;
}

} // namespace Netcode
} // namespace DarkAges