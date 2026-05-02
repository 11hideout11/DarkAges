// [NETWORK_AGENT] Protocol serialization using Protobuf
// Implements binary serialization for client-server communication

#include "netcode/NetworkManager.hpp"
#include "network_protocol.pb.h"
#include "netcode/ProtobufProtocol.hpp"
#include <cstring>

namespace DarkAges {
namespace Protocol {

// Protocol version: 1.0
constexpr uint32_t PROTOCOL_VERSION_MAJOR = 1;
constexpr uint32_t PROTOCOL_VERSION_MINOR = 0;
constexpr uint32_t PROTOCOL_VERSION = (PROTOCOL_VERSION_MAJOR << 16) | PROTOCOL_VERSION_MINOR;

// ============================================================================
// Input State Serialization (Legacy - for backwards compatibility)
// ============================================================================

std::vector<uint8_t> serializeInput(const InputState& input) {
    // Use Protobuf for serialization
    NetworkProto::ClientInput proto_input;
    proto_input.set_sequence(input.sequence);
    proto_input.set_timestamp(input.timestamp_ms);
    
    // Pack input flags
    uint32_t flags = 0;
    flags |= (input.forward ? 1 : 0) << 0;
    flags |= (input.backward ? 1 : 0) << 1;
    flags |= (input.left ? 1 : 0) << 2;
    flags |= (input.right ? 1 : 0) << 3;
    flags |= (input.jump ? 1 : 0) << 4;
    flags |= (input.attack ? 1 : 0) << 5;
    flags |= (input.block ? 1 : 0) << 6;
    flags |= (input.sprint ? 1 : 0) << 7;
    proto_input.set_input_flags(flags);
    
    proto_input.set_yaw(input.yaw);
    proto_input.set_pitch(input.pitch);
    
    std::vector<uint8_t> data;
    data.resize(proto_input.ByteSizeLong());
    proto_input.SerializeToArray(data.data(), static_cast<int>(data.size()));
    return data;
}

bool deserializeInput(std::span<const uint8_t> data, InputState& outInput) {
    NetworkProto::ClientInput proto_input;
    if (!proto_input.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
        return false;
    }
    
    outInput.sequence = proto_input.sequence();
    outInput.timestamp_ms = proto_input.timestamp();
    
    // Unpack flags
    uint32_t flags = proto_input.input_flags();
    outInput.forward = (flags >> 0) & 1;
    outInput.backward = (flags >> 1) & 1;
    outInput.left = (flags >> 2) & 1;
    outInput.right = (flags >> 3) & 1;
    outInput.jump = (flags >> 4) & 1;
    outInput.attack = (flags >> 5) & 1;
    outInput.block = (flags >> 6) & 1;
    outInput.sprint = (flags >> 7) & 1;
    
    outInput.yaw = proto_input.yaw();
    outInput.pitch = proto_input.pitch();
    
    return true;
}

// ============================================================================
// Entity State Serialization
// ============================================================================

namespace {
    // Quantization helpers
    constexpr float POSITION_SCALE = 64.0f;  // ~1.5cm precision
    constexpr float ROTATION_SCALE = 32767.0f / 3.14159265f;  // Precision for radians
    
    inline int32_t quantizePosition(float value) {
        return static_cast<int32_t>(value * POSITION_SCALE);
    }
    
    inline float dequantizePosition(int32_t value) {
        return value / POSITION_SCALE;
    }
    
    inline int32_t quantizeRotation(float value) {
        return static_cast<int32_t>(value * ROTATION_SCALE);
    }
    
    inline float dequantizeRotation(int32_t value) {
        return value / ROTATION_SCALE;
    }
}

void entityToProto(const EntityStateData& entity, NetworkProto::EntityState* proto) {
    proto->set_entity_id(static_cast<uint32_t>(entity.entity));
    proto->set_type(entity.entityType);
    
    // Quantize position
    proto->set_pos_x(quantizePosition(entity.position.x * Constants::FIXED_TO_FLOAT));
    proto->set_pos_y(quantizePosition(entity.position.y * Constants::FIXED_TO_FLOAT));
    proto->set_pos_z(quantizePosition(entity.position.z * Constants::FIXED_TO_FLOAT));
    
    // Quantize velocity
    proto->set_vel_x(quantizePosition(entity.velocity.dx * Constants::FIXED_TO_FLOAT));
    proto->set_vel_y(quantizePosition(entity.velocity.dy * Constants::FIXED_TO_FLOAT));
    proto->set_vel_z(quantizePosition(entity.velocity.dz * Constants::FIXED_TO_FLOAT));
    
    // Quantize rotation
    proto->set_yaw(quantizeRotation(entity.rotation.yaw));
    proto->set_pitch(quantizeRotation(entity.rotation.pitch));
    
    proto->set_health_percent(entity.healthPercent);
    proto->set_anim_state(entity.animState);
    proto->set_team_id(entity.teamId);
    proto->set_changed_fields(entity.timestamp);  // Use timestamp as changed fields marker
}

EntityStateData protoToEntity(const NetworkProto::EntityState& proto) {
    EntityStateData entity;
    entity.entity = static_cast<EntityID>(proto.entity_id());
    entity.entityType = static_cast<uint8_t>(proto.type());
    
    // Dequantize position
    entity.position.x = static_cast<Constants::Fixed>(dequantizePosition(proto.pos_x()) * Constants::FLOAT_TO_FIXED);
    entity.position.y = static_cast<Constants::Fixed>(dequantizePosition(proto.pos_y()) * Constants::FLOAT_TO_FIXED);
    entity.position.z = static_cast<Constants::Fixed>(dequantizePosition(proto.pos_z()) * Constants::FLOAT_TO_FIXED);
    
    // Dequantize velocity
    entity.velocity.dx = static_cast<Constants::Fixed>(dequantizePosition(proto.vel_x()) * Constants::FLOAT_TO_FIXED);
    entity.velocity.dy = static_cast<Constants::Fixed>(dequantizePosition(proto.vel_y()) * Constants::FLOAT_TO_FIXED);
    entity.velocity.dz = static_cast<Constants::Fixed>(dequantizePosition(proto.vel_z()) * Constants::FLOAT_TO_FIXED);
    
    // Dequantize rotation
    entity.rotation.yaw = dequantizeRotation(proto.yaw());
    entity.rotation.pitch = dequantizeRotation(proto.pitch());
    
    entity.healthPercent = static_cast<uint8_t>(proto.health_percent());
    entity.animState = static_cast<uint8_t>(proto.anim_state());
    entity.teamId = static_cast<uint8_t>(proto.team_id());
    entity.timestamp = proto.changed_fields();
    
    return entity;
}

// ============================================================================
// Snapshot Serialization
// ============================================================================

std::vector<uint8_t> serializeSnapshot(uint32_t serverTick, uint32_t baselineTick,
                                       std::span<const EntityStateData> entities,
                                       std::span<const EntityID> removed) {
    NetworkProto::ServerSnapshot proto_snapshot;
    proto_snapshot.set_server_tick(serverTick);
    proto_snapshot.set_baseline_tick(baselineTick);
    
    // Add entity states
    for (const auto& entity : entities) {
        auto* proto_entity = proto_snapshot.add_entities();
        entityToProto(entity, proto_entity);
    }
    
    // Add removed entities
    for (EntityID id : removed) {
        proto_snapshot.add_removed_entities(static_cast<uint32_t>(id));
    }
    
    std::vector<uint8_t> data;
    data.resize(proto_snapshot.ByteSizeLong());
    proto_snapshot.SerializeToArray(data.data(), static_cast<int>(data.size()));
    return data;
}

bool deserializeSnapshot(std::span<const uint8_t> data,
                         std::vector<EntityStateData>& outEntities,
                         std::vector<EntityID>& outRemoved,
                         uint32_t& outServerTick,
                         uint32_t& outBaselineTick) {
    NetworkProto::ServerSnapshot proto_snapshot;
    if (!proto_snapshot.ParseFromArray(data.data(), static_cast<int>(data.size()))) {
        return false;
    }
    
    outServerTick = proto_snapshot.server_tick();
    outBaselineTick = proto_snapshot.baseline_tick();
    
    outEntities.clear();
    for (const auto& proto_entity : proto_snapshot.entities()) {
        outEntities.push_back(protoToEntity(proto_entity));
    }
    
    outRemoved.clear();
    for (uint32_t id : proto_snapshot.removed_entities()) {
        outRemoved.push_back(static_cast<EntityID>(id));
    }
    
    return true;
}

// ============================================================================
// Delta Compression
// ============================================================================

std::vector<uint8_t> createDeltaSnapshot(
    uint32_t serverTick,
    uint32_t baselineTick,
    std::span<const EntityStateData> currentEntities,
    std::span<const EntityID> removedEntities,
    std::span<const EntityStateData> baselineEntities) {
    
    // If no baseline, send full state
    if (baselineEntities.empty()) {
        return serializeSnapshot(serverTick, baselineTick, currentEntities, removedEntities);
    }
    
    // Build lookup for baseline entities
    std::unordered_map<EntityID, const EntityStateData*> baselineLookup;
    for (const auto& entity : baselineEntities) {
        baselineLookup[entity.entity] = &entity;
    }
    
    // Find changed entities
    std::vector<EntityStateData> changedEntities;
    for (const auto& current : currentEntities) {
        auto it = baselineLookup.find(current.entity);
        if (it == baselineLookup.end()) {
            // New entity - send full state
            changedEntities.push_back(current);
        } else {
            // Existing entity - check what changed
            const auto* baseline = it->second;
            EntityStateData delta = current;
            delta.timestamp = 0;  // Will set changed fields
            
            uint16_t changed = 0;
            if (!current.equalsPosition(*baseline)) {
                changed |= DELTA_POSITION;
            } else {
                delta.position = baseline->position;  // Same, will be compressed
            }
            
            if (!current.equalsRotation(*baseline)) {
                changed |= DELTA_ROTATION;
            } else {
                delta.rotation = baseline->rotation;
            }
            
            if (!current.equalsVelocity(*baseline)) {
                changed |= DELTA_VELOCITY;
            } else {
                delta.velocity = baseline->velocity;
            }
            
            if (current.healthPercent != baseline->healthPercent) {
                changed |= DELTA_HEALTH;
            }
            
            if (current.animState != baseline->animState) {
                changed |= DELTA_ANIM_STATE;
            }
            
            if (changed != 0) {
                delta.timestamp = changed;  // Store changed fields mask
                changedEntities.push_back(delta);
            }
        }
    }
    
    return serializeSnapshot(serverTick, baselineTick, changedEntities, removedEntities);
}

bool applyDeltaSnapshot(
    std::span<const uint8_t> data,
    std::vector<EntityStateData>& outEntities,
    uint32_t& outServerTick,
    uint32_t& outBaselineTick,
    std::vector<EntityID>& outRemovedEntities) {
    
    return deserializeSnapshot(data, outEntities, outRemovedEntities, outServerTick, outBaselineTick);
}

// ============================================================================
// Server Correction Serialization
// ============================================================================

std::vector<uint8_t> serializeCorrection(
    uint32_t serverTick,
    const Position& position,
    const Velocity& velocity,
    uint32_t lastProcessedInput) {
    
    NetworkProto::ServerCorrection proto_correction;
    proto_correction.set_server_tick(serverTick);
    proto_correction.set_last_processed_input(lastProcessedInput);
    
    proto_correction.set_pos_x(quantizePosition(position.x * Constants::FIXED_TO_FLOAT));
    proto_correction.set_pos_y(quantizePosition(position.y * Constants::FIXED_TO_FLOAT));
    proto_correction.set_pos_z(quantizePosition(position.z * Constants::FIXED_TO_FLOAT));
    
    proto_correction.set_vel_x(quantizePosition(velocity.dx * Constants::FIXED_TO_FLOAT));
    proto_correction.set_vel_y(quantizePosition(velocity.dy * Constants::FIXED_TO_FLOAT));
    proto_correction.set_vel_z(quantizePosition(velocity.dz * Constants::FIXED_TO_FLOAT));
    
    std::vector<uint8_t> data;
    data.resize(proto_correction.ByteSizeLong());
    proto_correction.SerializeToArray(data.data(), static_cast<int>(data.size()));
    return data;
}

// ============================================================================
// Version Management
// ============================================================================

uint32_t getProtocolVersion() {
    return PROTOCOL_VERSION;
}

bool isVersionCompatible(uint32_t clientVersion) {
    // For now, require exact match on major version
    uint32_t clientMajor = clientVersion >> 16;
    uint32_t serverMajor = PROTOCOL_VERSION >> 16;
    return clientMajor == serverMajor;
}

// ============================================================================
// Combat Event Serialization (binary, no protobuf dependency)
// ============================================================================

// [COMBAT_AGENT] Simple binary combat event format (no protobuf dependency)
// Format: [type:1=3][subtype:1][attacker_id:4][target_id:4][damage:4][health_pct:1][timestamp:4]
// Subtypes: 1=Damage, 2=Death, 3=Heal
std::vector<uint8_t> serializeCombatEvent(
    uint8_t subtype,
    uint32_t attackerId,
    uint32_t targetId,
    int32_t damage,
    uint8_t healthPercent,
    uint32_t timestampMs) {

    std::vector<uint8_t> data;
    data.reserve(1 + 1 + 4 + 4 + 4 + 1 + 4);  // 19 bytes

    data.push_back(static_cast<uint8_t>(PacketType::ReliableEvent));
    data.push_back(subtype);

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };

    auto appendInt32 = [&data](int32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(int32_t));
    };

    appendUInt32(attackerId);
    appendUInt32(targetId);
    appendInt32(damage);
    data.push_back(healthPercent);
    appendUInt32(timestampMs);

    return data;
}

bool deserializeCombatEvent(
    std::span<const uint8_t> data,
    uint8_t& outSubtype,
    uint32_t& outAttackerId,
    uint32_t& outTargetId,
    int32_t& outDamage,
    uint8_t& outHealthPercent,
    uint32_t& outTimestampMs) {

    constexpr size_t EVENT_SIZE = 1 + 1 + 4 + 4 + 4 + 1 + 4;  // 19 bytes (including type byte)

    if (data.size() < EVENT_SIZE) {
        return false;
    }

    size_t offset = 1;  // Skip packet type byte

    outSubtype = data[offset++];
    std::memcpy(&outAttackerId, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);
    std::memcpy(&outTargetId, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);
    std::memcpy(&outDamage, &data[offset], sizeof(int32_t));
    offset += sizeof(int32_t);
    outHealthPercent = data[offset++];
    std::memcpy(&outTimestampMs, &data[offset], sizeof(uint32_t));

    return true;
}

// ============================================================================
// Full Snapshot Serialization
// ============================================================================

// [CLIENT_AGENT] Create full snapshot in client-compatible binary format
// Matches NetworkManager_udp.cpp documented format and client NetworkManager.cs expectations
// Format: [type:1=2][server_tick:4][last_input:4][entity_count:4][entity_data...]
// Each entity: [id:4][pos_x:4f][pos_y:4f][pos_z:4f][vel_x:4f][vel_y:4f][vel_z:4f][health:1][anim:1][interaction_range:4f][prompt:64][dialogue_tree:4]
std::vector<uint8_t> createFullSnapshot(
    uint32_t serverTick,
    uint32_t lastProcessedInput,
    std::span<const EntityStateData> entities) {

    std::vector<uint8_t> data;
    // Header: 1 (type) + 4 (tick) + 4 (input) + 4 (count) = 13 bytes
    // Per entity (with Interactable): 4(id) + 3*4(pos) + 3*4(vel) + 1(health) + 1(anim) + 4(range) + 64(prompt) + 4(treeId) = 102 bytes
    data.reserve(13 + entities.size() * 102);

    data.push_back(static_cast<uint8_t>(PacketType::ServerSnapshot));

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };

    auto appendFloat = [&data](float value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(float));
    };

    appendUInt32(serverTick);
    appendUInt32(lastProcessedInput);
    appendUInt32(static_cast<uint32_t>(entities.size()));

    for (const auto& entity : entities) {
        appendUInt32(static_cast<uint32_t>(entity.entity));

        // Convert fixed-point positions to float for client compatibility
        float posX = entity.position.x * Constants::FIXED_TO_FLOAT;
        float posY = entity.position.y * Constants::FIXED_TO_FLOAT;
        float posZ = entity.position.z * Constants::FIXED_TO_FLOAT;
        appendFloat(posX);
        appendFloat(posY);
        appendFloat(posZ);

        // Convert fixed-point velocity to float
        float velX = entity.velocity.dx * Constants::FIXED_TO_FLOAT;
        float velY = entity.velocity.dy * Constants::FIXED_TO_FLOAT;
        float velZ = entity.velocity.dz * Constants::FIXED_TO_FLOAT;
        appendFloat(velX);
        appendFloat(velY);
        appendFloat(velZ);

        data.push_back(entity.healthPercent);
        data.push_back(entity.animState);

        // Interactable fields
        appendFloat(entity.interactionRange);
        data.insert(data.end(), entity.promptText, entity.promptText + sizeof(entity.promptText));
        appendUInt32(entity.dialogueTreeId);
    }

    return data;
}

// ============================================================================
// Chat / Quest / Dialogue Serialization (stub — used by GNS path)
// ============================================================================

std::vector<uint8_t> serializeChatMessage(const ChatMessage& msg) {
    std::vector<uint8_t> data;
    data.reserve(1 + 4 + 1 + 4 + 4 + 4 + CHAT_SENDER_NAME_MAX + CHAT_MESSAGE_MAX_LEN);

    data.push_back(static_cast<uint8_t>(PacketType::PACKET_CHAT));

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };

    appendUInt32(msg.messageId);
    data.push_back(static_cast<uint8_t>(msg.channel));
    appendUInt32(msg.senderId);
    appendUInt32(msg.targetId);
    appendUInt32(msg.timestampMs);

    for (uint32_t i = 0; i < CHAT_SENDER_NAME_MAX; ++i)
        data.push_back(static_cast<uint8_t>(msg.senderName[i]));
    for (uint32_t i = 0; i < CHAT_MESSAGE_MAX_LEN; ++i)
        data.push_back(static_cast<uint8_t>(msg.content[i]));

    return data;
}

std::vector<uint8_t> serializeQuestUpdate(const QuestUpdatePacket& pkt) {
    std::vector<uint8_t> data;
    data.reserve(1 + 4 + 1 + 4 + 4 + 1);

    data.push_back(static_cast<uint8_t>(PacketType::PACKET_QUEST_UPDATE));

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };

    appendUInt32(pkt.questId);
    data.push_back(pkt.objectiveIndex);
    appendUInt32(pkt.current);
    appendUInt32(pkt.required);
    data.push_back(pkt.status);

    return data;
}

std::vector<uint8_t> serializeDialogueStart(const DialogueStartPacket& pkt) {
    std::vector<uint8_t> data;
    size_t optionsSize = 0;
    for (const auto& opt : pkt.options) {
        optionsSize += 1 + opt.size();
    }
    data.reserve(4 + 4 + 32 + 256 + 1 + optionsSize);

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };

    appendUInt32(static_cast<uint32_t>(pkt.npcId));
    appendUInt32(pkt.dialogueId);
    data.insert(data.end(), pkt.npcName, pkt.npcName + sizeof(pkt.npcName));
    data.insert(data.end(), pkt.dialogueText, pkt.dialogueText + sizeof(pkt.dialogueText));
    uint8_t optCount = static_cast<uint8_t>(pkt.options.size());
    data.push_back(optCount);
    for (const auto& opt : pkt.options) {
        uint8_t len = static_cast<uint8_t>(opt.size());
        data.push_back(len);
        data.insert(data.end(), opt.begin(), opt.end());
    }

    return data;
}

std::vector<uint8_t> serializeDialogueResponse(const DialogueResponsePacket& pkt) {
    std::vector<uint8_t> data;
    data.reserve(4 + 1);

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };
    appendUInt32(pkt.dialogueId);
    data.push_back(pkt.selectedOption);
    return data;
}

} // namespace Protocol
} // namespace DarkAges
