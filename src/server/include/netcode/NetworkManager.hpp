#pragma once

#include "ecs/CoreTypes.hpp"
#include "security/DDoSProtection.hpp"
#include <vector>
#include <functional>
#include <cstdint>
#include <span>
#include <optional>
#include <unordered_map>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <cstring>

// [NETWORK_AGENT] GameNetworkingSockets wrapper
// Handles UDP connections, reliable/unreliable channels, and connection quality

namespace DarkAges {

// Forward declarations for GNS types (to avoid header dependency)
struct GNSInternal;

// Connection handle
using ConnectionID = uint32_t;
static constexpr ConnectionID INVALID_CONNECTION = 0;

// Packet types
enum class PacketType : uint8_t {
    ClientInput = 1,      // Client -> Server: movement/actions
    ServerSnapshot = 2,   // Server -> Client: world state
    ReliableEvent = 3,    // Bidirectional: combat, pickups, etc.
    Ping = 4,             // Bidirectional: latency measurement
    Handshake = 5,        // Connection setup
    Disconnect = 6,       // Graceful disconnect
};

// Connection quality metrics
struct ConnectionQuality {
    uint32_t rttMs{0};
    float packetLoss{0.0f};
    float jitterMs{0.0f};
    uint32_t packetsSent{0};
    uint32_t packetsReceived{0};
    uint64_t bytesSent{0};
    uint64_t bytesReceived{0};
};

// Client input packet (from network)
struct ClientInputPacket {
    ConnectionID connectionId{INVALID_CONNECTION};
    InputState input;
    uint32_t receiveTimeMs{0};
};

// Combat action RPC packet (client -> server attack request)
struct CombatActionPacket {
    ConnectionID connectionId{INVALID_CONNECTION};
    uint8_t actionType{0};     // 1=melee, 2=ranged, 3=ability
    EntityID targetEntity{entt::null};
    uint32_t clientTimestamp{0};
    uint32_t receiveTimeMs{0};
};

// Lock-on request packet (client -> server)
struct LockOnRequestPacket {
    ConnectionID connectionId{INVALID_CONNECTION};
    EntityID targetEntity{entt::null};
    uint32_t clientTimestamp{0};
    uint32_t receiveTimeMs{0};
};

// Quest action packet (client -> server)
struct QuestActionPacket {
    ConnectionID connectionId{INVALID_CONNECTION};
    uint32_t questId{0};
    uint8_t actionType{0};  // 0=accept, 1=complete
    uint32_t receiveTimeMs{0};
};

// Quest update packet (server -> client)
struct QuestUpdatePacket {
    uint32_t questId{0};
    uint8_t objectiveIndex{0};
    uint32_t current{0};
    uint32_t required{0};
    uint8_t status{0};  // 0=in_progress, 1=complete, 2=rewarded
};

// Snapshot packet for serialization
struct SnapshotPacket {
    uint32_t serverTick{0};
    uint32_t baselineTick{0};
    std::vector<uint8_t> data;  // Serialized entity states
};

// [NETWORK_AGENT] Main network manager interface
class NetworkManager {
public:
    using ConnectionCallback = std::function<void(ConnectionID)>;
    using InputCallback = std::function<void(const ClientInputPacket&)>;
    using SnapshotCallback = std::function<void(ConnectionID, std::span<const uint8_t>)>;
    using CombatActionCallback = std::function<void(const CombatActionPacket&)>;
    using LockOnRequestCallback = std::function<void(const LockOnRequestPacket&)>;
    using ChatCallback = std::function<void(ConnectionID, const ChatMessage&)>;
    using QuestUpdateCallback = std::function<void(const QuestUpdatePacket&)>;
    using QuestActionCallback = std::function<void(const QuestActionPacket&)>;

public:
    NetworkManager();
    ~NetworkManager();
    
    // Initialize on port, return true if success
    bool initialize(uint16_t port = Constants::DEFAULT_SERVER_PORT);
    
    // Shutdown and cleanup
    void shutdown();
    
    // Non-blocking update - call every tick
    void update(uint32_t currentTimeMs);
    
    // Send unreliable snapshot to client (position updates)
    void sendSnapshot(ConnectionID connectionId, std::span<const uint8_t> data);
    
    // Send reliable event to client (combat, important state)
    void sendEvent(ConnectionID connectionId, std::span<const uint8_t> data);
    
    // Broadcast to all connected clients
    void broadcastSnapshot(std::span<const uint8_t> data);
    void broadcastEvent(std::span<const uint8_t> data);
    
    // Send to multiple specific connections
    void sendToMultiple(const std::vector<ConnectionID>& conns, std::span<const uint8_t> data);
    
    // Get pending inputs (call after update())
    [[nodiscard]] std::vector<ClientInputPacket> getPendingInputs();
    
    // Get pending combat actions (call after update())
    [[nodiscard]] std::vector<CombatActionPacket> getPendingCombatActions();
    
    // Get pending lock-on requests (call after update())
    [[nodiscard]] std::vector<LockOnRequestPacket> getPendingLockOnRequests();
    
    // Get pending game events (respawn requests, etc.)
    [[nodiscard]] std::vector<EntityID> getPendingRespawnRequests();
    
    // Get pending chat messages (call after update)
    [[nodiscard]] std::vector<ChatMessage> getPendingChatMessages();
    
    // Get pending quest actions from clients (accept/complete)
    [[nodiscard]] std::vector<QuestActionPacket> getPendingQuestActions();
    
    // Clear processed inputs up to a sequence number
    void clearProcessedInputs(uint32_t upToSequence);
    
    // Connection management
    void disconnect(ConnectionID connectionId, const char* reason = nullptr);
    [[nodiscard]] bool isConnected(ConnectionID connectionId) const;
    [[nodiscard]] ConnectionQuality getConnectionQuality(ConnectionID connectionId) const;
    
    // [ZONE_AGENT] Set the game entity ID associated with a network connection
    // Must be called before connection response is sent for accurate client entity mapping
    void setConnectionEntityId(ConnectionID connectionId, EntityID entityId);
    
    // Callbacks
    void setOnClientConnected(ConnectionCallback callback) { onConnected_ = std::move(callback); }
    void setOnClientDisconnected(ConnectionCallback callback) { onDisconnected_ = std::move(callback); }
    void setOnInputReceived(InputCallback callback) { onInput_ = std::move(callback); }
    void setOnCombatAction(CombatActionCallback callback) { onCombatAction_ = std::move(callback); }
    void setOnLockOnRequest(LockOnRequestCallback callback) { onLockOnRequest_ = std::move(callback); }
    void setOnChatReceived(ChatCallback callback) { onChat_ = std::move(callback); }
    void setOnQuestActionReceived(QuestActionCallback callback) { onQuestAction_ = std::move(callback); }
    
    // Send combat result to a specific client
    // Format: [type:1=11][result:1][damage:4][target_id:4][is_critical:1][timestamp:4]
    // Result codes: 0=hit, 1=miss, 2=blocked, 3=cooldown, 4=gcd_active
    void sendCombatResult(ConnectionID connectionId, uint8_t resultCode, int32_t damage, 
                          uint32_t targetId, bool isCritical, uint32_t timestamp);

    // Send lock-on confirmation to client
    // Format: [type:1=12][target_entity:4]
    void sendLockOnConfirmed(ConnectionID connectionId, EntityID targetEntity);

    // Send lock-on failure to client
    // Format: [type:1=13][target_entity:4][reason:1]
    // Reason codes: 0=out_of_range, 1=not_visible, 2=not_alive, 3=invalid_target, 4=busy, 5=error
    void sendLockOnFailed(ConnectionID connectionId, EntityID targetEntity, uint8_t reason);

    // Send chat message to a specific client
    void sendChatMessage(ConnectionID connectionId, const ChatMessage& msg);
    
    // Send quest update to a specific client
    void sendQuestUpdate(ConnectionID connectionId, const QuestUpdatePacket& msg);

    // Statistics
    [[nodiscard]] size_t getConnectionCount() const;
    [[nodiscard]] uint64_t getTotalBytesSent() const;
    [[nodiscard]] uint64_t getTotalBytesReceived() const;
    
    // Rate limiting check
    [[nodiscard]] bool isRateLimited(ConnectionID connectionId) const;
    
    // [SECURITY_AGENT] DDoS protection integration
    [[nodiscard]] Security::DDoSProtection& getDDoSProtection() { return ddosProtection_; }
    [[nodiscard]] const Security::DDoSProtection& getDDoSProtection() const { return ddosProtection_; }
    
    // Check if connection should be accepted (DDoS protection)
    [[nodiscard]] bool shouldAcceptConnection(const std::string& ipAddress);
    
    // Process packet with DDoS protection
    [[nodiscard]] bool processPacket(ConnectionID connectionId, 
                                     const std::string& ipAddress,
                                     uint32_t packetSize,
                                     uint32_t currentTimeMs);

private:
    std::unique_ptr<GNSInternal> internal_;
    Security::DDoSProtection ddosProtection_;
    
    ConnectionCallback onConnected_;
    ConnectionCallback onDisconnected_;
    InputCallback onInput_;
    CombatActionCallback onCombatAction_;
    LockOnRequestCallback onLockOnRequest_;
    ChatCallback onChat_;
    QuestActionCallback onQuestAction_;  
    
    std::vector<ClientInputPacket> pendingInputs_;
    std::vector<LockOnRequestPacket> pendingLockOnRequests_;
    std::vector<QuestActionPacket> pendingQuestActions_;
    
    bool initialized_{false};
};

// [NETWORK_AGENT] Protocol serialization helpers
namespace Protocol {
    // Serialize input state to binary
    std::vector<uint8_t> serializeInput(const InputState& input);
    
    // Deserialize input state from binary
    bool deserializeInput(std::span<const uint8_t> data, InputState& outInput);
    
    // Serialize entity state for snapshot
    struct EntityStateData {
        EntityID entity;
        Position position;
        Velocity velocity;
        Rotation rotation;
        uint8_t healthPercent{0};
        uint8_t animState{0};
        uint8_t entityType{0};  // 0=player, 1=projectile, 2=loot, 3=NPC
        uint32_t timestamp{0};  // For lag compensation and reconciliation
        
        // Equality comparison for delta detection
        [[nodiscard]] bool equalsPosition(const EntityStateData& other) const;
        [[nodiscard]] bool equalsRotation(const EntityStateData& other) const;
        [[nodiscard]] bool equalsVelocity(const EntityStateData& other) const;
    };
    
    // Delta entity state - only includes changed fields
    struct DeltaEntityState {
        EntityID entity;
        uint16_t changedFields;  // Bit mask of changed fields
        Position position;       // Only valid if bit 0 set
        Rotation rotation;       // Only valid if bit 1 set
        Velocity velocity;       // Only valid if bit 2 set
        uint8_t healthPercent;   // Only valid if bit 3 set
        uint8_t animState;       // Only valid if bit 4 set
        uint8_t entityType;      // Only valid if bit 5 set (for new entities)
    };
    
    // Bit masks for changedFields
    enum DeltaFieldMask : uint16_t {
        DELTA_POSITION = 1 << 0,
        DELTA_ROTATION = 1 << 1,
        DELTA_VELOCITY = 1 << 2,
        DELTA_HEALTH = 1 << 3,
        DELTA_ANIM_STATE = 1 << 4,
        DELTA_ENTITY_TYPE = 1 << 5,
        DELTA_NEW_ENTITY = 0xFFFF  // All fields for new entities
    };
    
    // Delta compression for snapshots
    // If baselineEntities is empty, sends full state for all entities
    std::vector<uint8_t> createDeltaSnapshot(
        uint32_t serverTick,
        uint32_t baselineTick,
        std::span<const EntityStateData> currentEntities,
        std::span<const EntityID> removedEntities,
        std::span<const EntityStateData> baselineEntities = {}
    );
    
    // [CLIENT_AGENT] Create full snapshot in client-compatible format
    // Format: [type:1=2][server_tick:4][last_input:4][entity_count:4][entity_data...]
    // Each entity: [id:4][pos_x:4f][pos_y:4f][pos_z:4f][vel_x:4f][vel_y:4f][vel_z:4f][health:1][anim:1]
    std::vector<uint8_t> createFullSnapshot(
        uint32_t serverTick,
        uint32_t lastProcessedInput,
        std::span<const EntityStateData> entities
    );
    
    // Apply delta snapshot - reconstructs full state from baseline + delta
    // Returns true on success, false if baseline mismatch
    bool applyDeltaSnapshot(
        std::span<const uint8_t> data,
        std::vector<EntityStateData>& outEntities,
        uint32_t& outServerTick,
        uint32_t& outBaselineTick,
        std::vector<EntityID>& outRemovedEntities
    );
    
    // Position delta encoding helpers
    namespace DeltaEncoding {
        // Encode a position delta using variable-length encoding
        // Returns bytes written (1, 3, or 6 bytes per component based on delta magnitude)
        size_t encodePositionDelta(uint8_t* buffer, size_t bufferSize,
                                   const Position& current, const Position& baseline);
        
        // Decode a position delta
        // Returns bytes read
        size_t decodePositionDelta(const uint8_t* buffer, size_t bufferSize,
                                   Position& outPosition, const Position& baseline);
        
        // Thresholds for variable-length encoding (in fixed-point units)
        inline constexpr int32_t SMALL_DELTA_THRESHOLD = 127;      // Fits in int8
        inline constexpr int32_t MEDIUM_DELTA_THRESHOLD = 32767;   // Fits in int16
    }
    
    // [NETWORK_AGENT] Protocol versioning
    // Returns the current protocol version (major << 16 | minor)
    uint32_t getProtocolVersion();
    
    // Check if client version is compatible with server
    bool isVersionCompatible(uint32_t clientVersion);
    
    // [NETWORK_AGENT] Serialize server correction for client reconciliation
    // Sent when server detects client misprediction
    std::vector<uint8_t> serializeCorrection(
        uint32_t serverTick,
        const Position& position,
        const Velocity& velocity,
        uint32_t lastProcessedInput
    );

    // [COMBAT_AGENT] Simple binary combat event format (no protobuf dependency)
    // Format: [type:1=3][subtype:1][attacker_id:4][target_id:4][damage:4][health_pct:1][timestamp:4]
    // Subtypes: 1=Damage, 2=Death, 3=Heal
    std::vector<uint8_t> serializeCombatEvent(
        uint8_t subtype,
        uint32_t attackerId,
        uint32_t targetId,
        int32_t damage,
        uint8_t healthPercent,
        uint32_t timestampMs
    );

    bool deserializeCombatEvent(
        std::span<const uint8_t> data,
        uint8_t& outSubtype,
        uint32_t& outAttackerId,
        uint32_t& outTargetId,
        int32_t& outDamage,
        uint8_t& outHealthPercent,
        uint32_t& outTimestampMs
    );
}

} // namespace DarkAges
