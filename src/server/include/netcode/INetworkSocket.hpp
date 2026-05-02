// [NETWORK_AGENT] Network Socket Interface
// Abstracts UDP networking to allow runtime switching between stub and GNS implementations
// Without requiring compile-time ENABLE_GNS flag. Enables production deployment with GNS
// while maintaining easy local development with stub.

#pragma once

#include "ecs/CoreTypes.hpp"
#include "netcode/Protocol.hpp"
#include <vector>
#include <functional>
#include <cstdint>
#include <span>
#include <memory>
#include <string>

namespace DarkAges {
namespace Netcode {

// ============================================================================
// Connection Handle
// ============================================================================

using ConnectionID = uint32_t;
constexpr ConnectionID INVALID_CONNECTION_ID = 0;

// ============================================================================
// Connection Quality
// ============================================================================

struct ConnectionQuality {
    uint32_t rttMs{0};
    float packetLoss{0.0f};
    float jitterMs{0.0f};
    uint32_t packetsSent{0};
    uint32_t packetsReceived{0};
    uint64_t bytesSent{0};
    uint64_t bytesReceived{0};
};

// ============================================================================
// Packet Types
// ============================================================================

// Alias to the canonical Protocol::PacketType to avoid a third parallel enum.
// Use DarkAges::Protocol::PacketType throughout the codebase; this alias
// exists purely so existing Netcode code compiles without full qualification.
using PacketType = ::DarkAges::Protocol::PacketType;

// ============================================================================
// INetworkSocket Interface
// ============================================================================

// [PRD-012] Abstract socket interface for GNS/stub switching
class INetworkSocket {
public:
    virtual ~INetworkSocket() = default;

    // ------------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------------

    // Initialize socket on port, return true on success
    virtual bool initialize(uint16_t port) = 0;

    // Shutdown and cleanup
    virtual void shutdown() = 0;

    // Non-blocking update - call every tick
    virtual void update(uint32_t currentTimeMs) = 0;

    // ------------------------------------------------------------------------
    // Connection Management
    // ------------------------------------------------------------------------

    // Get list of currently connected connection IDs
    virtual std::vector<ConnectionID> getConnections() = 0;

    // Disconnect a specific connection
    virtual void disconnect(ConnectionID connectionId, const char* reason = nullptr) = 0;

    // Check if connection is still active
    virtual bool isConnected(ConnectionID connectionId) const = 0;

    // Get max number of connections
    virtual uint32_t getMaxConnections() const = 0;

    // Get current connection count
    virtual uint32_t getConnectionCount() const = 0;

    // ------------------------------------------------------------------------
    // Sending
    // ------------------------------------------------------------------------

    // Send unreliable (unordered) data to connection
    virtual bool sendUnreliable(ConnectionID connectionId, std::span<const uint8_t> data) = 0;

    // Send reliable (ordered, guaranteed) data to connection
    virtual bool sendReliable(ConnectionID connectionId, std::span<const uint8_t> data) = 0;

    // Broadcast unreliable data to all connections
    virtual void broadcastUnreliable(std::span<const uint8_t> data) = 0;

    // Broadcast reliable data to all connections
    virtual void broadcastReliable(std::span<const uint8_t> data) = 0;

    // Send to multiple specific connections
    virtual void sendToMultiple(const std::vector<ConnectionID>& connections, 
                                std::span<const uint8_t> data) = 0;

    // ------------------------------------------------------------------------
    // Receiving
    // ------------------------------------------------------------------------

    // Get next pending message. Returns type and fills buffer.
    // Returns true if data was available
    virtual bool receive(PacketType& type, ConnectionID& connectionId, 
                       std::vector<uint8_t>& buffer) = 0;

    // Peek at next message without consuming
    virtual bool peek(PacketType& type, ConnectionID& connectionId) const = 0;

    // Consume the peeked message
    virtual void consume() = 0;

    // ------------------------------------------------------------------------
    // Statistics
    // ------------------------------------------------------------------------

    // Get connection quality metrics
    virtual ConnectionQuality getConnectionQuality(ConnectionID connectionId) const = 0;

    // Get total bytes sent
    virtual uint64_t getTotalBytesSent() const = 0;

    // Get total bytes received
    virtual uint64_t getTotalBytesReceived() const = 0;

    // ------------------------------------------------------------------------
    // Callbacks (for event-driven receiving)
    // ------------------------------------------------------------------------

    using ConnectionCallback = std::function<void(ConnectionID)>;
    using DisconnectCallback = std::function<void(ConnectionID, const char*)>;
    using MessageCallback = std::function<void(PacketType, ConnectionID, std::span<const uint8_t>)>;

    virtual void setOnConnected(ConnectionCallback callback) = 0;
    virtual void setOnDisconnected(DisconnectCallback callback) = 0;
    virtual void setOnMessage(MessageCallback callback) = 0;
};

// ============================================================================
// StubSocket - Development-friendly UDP implementation
// ============================================================================

// Stub implementation using basic POSIX sockets
// For local development and testing (no GNS dependency)
class StubSocket : public INetworkSocket {
public:
    StubSocket();
    ~StubSocket() override;

    bool initialize(uint16_t port) override;
    void shutdown() override;
    void update(uint32_t currentTimeMs) override;

    std::vector<ConnectionID> getConnections() override;
    void disconnect(ConnectionID connectionId, const char* reason = nullptr) override;
    bool isConnected(ConnectionID connectionId) const override;
    uint32_t getMaxConnections() const override;
    uint32_t getConnectionCount() const override;

    bool sendUnreliable(ConnectionID connectionId, std::span<const uint8_t> data) override;
    bool sendReliable(ConnectionID connectionId, std::span<const uint8_t> data) override;
    void broadcastUnreliable(std::span<const uint8_t> data) override;
    void broadcastReliable(std::span<const uint8_t> data) override;
    void sendToMultiple(const std::vector<ConnectionID>& connections, 
                    std::span<const uint8_t> data) override;

    bool receive(PacketType& type, ConnectionID& connectionId, 
               std::vector<uint8_t>& buffer) override;
    bool peek(PacketType& type, ConnectionID& connectionId) const override;
    void consume() override;

    ConnectionQuality getConnectionQuality(ConnectionID connectionId) const override;
    uint64_t getTotalBytesSent() const override;
    uint64_t getTotalBytesReceived() const override;

    void setOnConnected(ConnectionCallback callback) override;
    void setOnDisconnected(DisconnectCallback callback) override;
    void setOnMessage(MessageCallback callback) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// GNSSocket - GameNetworkingSockets implementation
// ============================================================================

// Production implementation using Steam's GameNetworkingSockets
// Requires GNS library at runtime
class GNSSocket : public INetworkSocket {
public:
    GNSSocket();
    ~GNSSocket() override;

    bool initialize(uint16_t port) override;
    void shutdown() override;
    void update(uint32_t currentTimeMs) override;

    std::vector<ConnectionID> getConnections() override;
    void disconnect(ConnectionID connectionId, const char* reason = nullptr) override;
    bool isConnected(ConnectionID connectionId) const override;
    uint32_t getMaxConnections() const override;
    uint32_t getConnectionCount() const override;

    bool sendUnreliable(ConnectionID connectionId, std::span<const uint8_t> data) override;
    bool sendReliable(ConnectionID connectionId, std::span<const uint8_t> data) override;
    void broadcastUnreliable(std::span<const uint8_t> data) override;
    void broadcastReliable(std::span<const uint8_t> data) override;
    void sendToMultiple(const std::vector<ConnectionID>& connections, 
                    std::span<const uint8_t> data) override;

    bool receive(PacketType& type, ConnectionID& connectionId, 
               std::vector<uint8_t>& buffer) override;
    bool peek(PacketType& type, ConnectionID& connectionId) const override;
    void consume() override;

    ConnectionQuality getConnectionQuality(ConnectionID connectionId) const override;
    uint64_t getTotalBytesSent() const override;
    uint64_t getTotalBytesReceived() const override;

    void setOnConnected(ConnectionCallback callback) override;
    void setOnDisconnected(DisconnectCallback callback) override;
    void setOnMessage(MessageCallback callback) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// NetworkSocketFactory - Create appropriate socket implementation
// ============================================================================

// Factory for creating socket instances
// Checks for GNS availability and returns appropriate implementation
class NetworkSocketFactory {
public:
    enum class SocketType {
        Auto,    // Auto-detect (GNS if available, stub otherwise)
        Stub,    // Force stub implementation
        GNS      // Force GNS implementation
    };

    // Create socket instance based on type
    // Auto-detect uses GNS if available, falls back to stub
    static std::unique_ptr<INetworkSocket> create(SocketType type = SocketType::Auto);

    // Check if GNS is available at runtime
    static bool isGNSAvailable();

    // Get the current socket type being used
    static SocketType getCurrentType();
};

} // namespace Netcode
} // namespace DarkAges