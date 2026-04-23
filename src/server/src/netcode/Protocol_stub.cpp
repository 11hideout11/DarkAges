// Protocol serialization helpers (stub implementation)
// Used when GNS/FlatBuffers are not available

#include "netcode/NetworkManager.hpp"
#include <cstring>
#include <unordered_map>

namespace DarkAges {

namespace Protocol {

std::vector<uint8_t> serializeInput(const InputState& input) {
    std::vector<uint8_t> data;
    data.reserve(17);

    uint8_t flags = 0;
    flags |= (input.forward  & 0x1) << 0;
    flags |= (input.backward & 0x1) << 1;
    flags |= (input.left     & 0x1) << 2;
    flags |= (input.right    & 0x1) << 3;
    flags |= (input.jump     & 0x1) << 4;
    flags |= (input.attack   & 0x1) << 5;
    flags |= (input.block    & 0x1) << 6;
    flags |= (input.sprint   & 0x1) << 7;

    data.push_back(flags);

    auto appendBytes = [&data](const void* ptr, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(ptr);
        data.insert(data.end(), bytes, bytes + size);
    };

    appendBytes(&input.yaw, sizeof(float));
    appendBytes(&input.pitch, sizeof(float));
    appendBytes(&input.sequence, sizeof(uint32_t));
    appendBytes(&input.timestamp_ms, sizeof(uint32_t));

    return data;
}

bool deserializeInput(std::span<const uint8_t> data, InputState& outInput) {
    constexpr size_t MIN_SIZE = 1 + sizeof(float) * 2 + sizeof(uint32_t) * 2;

    if (data.size() < MIN_SIZE) {
        return false;
    }

    size_t offset = 0;

    uint8_t flags = data[offset++];
    outInput.forward  = (flags >> 0) & 0x1;
    outInput.backward = (flags >> 1) & 0x1;
    outInput.left     = (flags >> 2) & 0x1;
    outInput.right    = (flags >> 3) & 0x1;
    outInput.jump     = (flags >> 4) & 0x1;
    outInput.attack   = (flags >> 5) & 0x1;
    outInput.block    = (flags >> 6) & 0x1;
    outInput.sprint   = (flags >> 7) & 0x1;

    std::memcpy(&outInput.yaw, &data[offset], sizeof(float));
    offset += sizeof(float);
    std::memcpy(&outInput.pitch, &data[offset], sizeof(float));
    offset += sizeof(float);
    std::memcpy(&outInput.sequence, &data[offset], sizeof(uint32_t));
    offset += sizeof(uint32_t);
    std::memcpy(&outInput.timestamp_ms, &data[offset], sizeof(uint32_t));

    return true;
}

bool EntityStateData::equalsPosition(const EntityStateData& other) const {
    return position.x == other.position.x &&
           position.y == other.position.y &&
           position.z == other.position.z;
}

bool EntityStateData::equalsRotation(const EntityStateData& other) const {
    return rotation.yaw == other.rotation.yaw &&
           rotation.pitch == other.rotation.pitch;
}

bool EntityStateData::equalsVelocity(const EntityStateData& other) const {
    return velocity.dx == other.velocity.dx &&
           velocity.dy == other.velocity.dy &&
           velocity.dz == other.velocity.dz;
}

std::vector<uint8_t> createDeltaSnapshot(
    uint32_t serverTick,
    uint32_t baselineTick,
    std::span<const EntityStateData> currentEntities,
    std::span<const EntityID> removedEntities,
    std::span<const EntityStateData> baselineEntities) {

    std::vector<uint8_t> data;
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
    appendUInt32(baselineTick);

    std::unordered_map<EntityID, const EntityStateData*> baselineMap;
    for (const auto& entity : baselineEntities) {
        baselineMap[entity.entity] = &entity;
    }

    struct ChangedEntity {
        const EntityStateData* entity;
        uint16_t changedFields;
    };
    std::vector<ChangedEntity> entitiesToSend;

    for (const auto& current : currentEntities) {
        auto it = baselineMap.find(current.entity);
        if (it == baselineMap.end()) {
            entitiesToSend.push_back({&current, 0xFFFF});
        } else {
            const auto* baseline = it->second;
            uint16_t changed = 0;
            if (!current.equalsPosition(*baseline)) changed |= 0x0001;
            if (!current.equalsVelocity(*baseline)) changed |= 0x0002;
            if (!current.equalsRotation(*baseline)) changed |= 0x0004;
            if (current.healthPercent != baseline->healthPercent) changed |= 0x0008;
            if (current.animState != baseline->animState) changed |= 0x0010;
            if (changed != 0) {
                entitiesToSend.push_back({&current, changed});
            }
        }
    }

    appendUInt32(static_cast<uint32_t>(entitiesToSend.size()));

    for (const auto& item : entitiesToSend) {
        const auto& entity = *item.entity;
        uint16_t changedFields = item.changedFields;

        appendUInt32(static_cast<uint32_t>(entity.entity));
        data.push_back(static_cast<uint8_t>(changedFields & 0xFF));
        data.push_back(static_cast<uint8_t>((changedFields >> 8) & 0xFF));

        if (changedFields & 0x0001) {
            appendUInt32(static_cast<uint32_t>(entity.position.x));
            appendUInt32(static_cast<uint32_t>(entity.position.y));
            appendUInt32(static_cast<uint32_t>(entity.position.z));
        }
        if (changedFields & 0x0002) {
            appendUInt32(static_cast<uint32_t>(entity.velocity.dx));
            appendUInt32(static_cast<uint32_t>(entity.velocity.dy));
            appendUInt32(static_cast<uint32_t>(entity.velocity.dz));
        }
        if (changedFields & 0x0004) {
            appendFloat(entity.rotation.yaw);
            appendFloat(entity.rotation.pitch);
        }
        if (changedFields & 0x0008) {
            data.push_back(entity.healthPercent);
        }
        if (changedFields & 0x0010) {
            data.push_back(entity.animState);
        }
        if (changedFields == 0xFFFF) {
            data.push_back(entity.entityType);
        }
    }

    appendUInt32(static_cast<uint32_t>(removedEntities.size()));
    for (const auto& entity : removedEntities) {
        appendUInt32(static_cast<uint32_t>(entity));
    }

    return data;
}

bool applyDeltaSnapshot(
    std::span<const uint8_t> data,
    std::vector<EntityStateData>& outEntities,
    uint32_t& outServerTick,
    uint32_t& outBaselineTick,
    std::vector<EntityID>& outRemovedEntities) {

    if (data.size() < 13) {
        return false;
    }

    size_t offset = 1;

    auto readUInt32 = [&data, &offset]() -> uint32_t {
        uint32_t value;
        std::memcpy(&value, &data[offset], sizeof(uint32_t));
        offset += sizeof(uint32_t);
        return value;
    };

    auto readFloat = [&data, &offset]() -> float {
        float value;
        std::memcpy(&value, &data[offset], sizeof(float));
        offset += sizeof(float);
        return value;
    };

    outServerTick = readUInt32();
    outBaselineTick = readUInt32();

    uint32_t entityCount = readUInt32();

    outEntities.clear();
    outEntities.reserve(entityCount);

    for (uint32_t i = 0; i < entityCount; ++i) {
        if (offset + 4 > data.size()) return false;

        EntityStateData entity;
        entity.entity = static_cast<EntityID>(readUInt32());

        if (offset + 2 > data.size()) return false;
        uint16_t changedFields = data[offset] | (data[offset + 1] << 8);
        offset += 2;

        if (changedFields & 0x0001) {
            if (offset + 12 > data.size()) return false;
            entity.position.x = static_cast<int32_t>(readUInt32());
            entity.position.y = static_cast<int32_t>(readUInt32());
            entity.position.z = static_cast<int32_t>(readUInt32());
        }
        if (changedFields & 0x0002) {
            if (offset + 12 > data.size()) return false;
            entity.velocity.dx = static_cast<int32_t>(readUInt32());
            entity.velocity.dy = static_cast<int32_t>(readUInt32());
            entity.velocity.dz = static_cast<int32_t>(readUInt32());
        }
        if (changedFields & 0x0004) {
            if (offset + 8 > data.size()) return false;
            entity.rotation.yaw = readFloat();
            entity.rotation.pitch = readFloat();
        }
        if (changedFields & 0x0008) {
            if (offset + 1 > data.size()) return false;
            entity.healthPercent = data[offset++];
        }
        if (changedFields & 0x0010) {
            if (offset + 1 > data.size()) return false;
            entity.animState = data[offset++];
        }
        if (changedFields == 0xFFFF) {
            if (offset + 1 > data.size()) return false;
            entity.entityType = data[offset++];
        }

        outEntities.push_back(entity);
    }

    if (offset + 4 > data.size()) return false;
    uint32_t removedCount = readUInt32();

    outRemovedEntities.clear();
    outRemovedEntities.reserve(removedCount);

    for (uint32_t i = 0; i < removedCount; ++i) {
        if (offset + 4 > data.size()) return false;
        outRemovedEntities.push_back(static_cast<EntityID>(readUInt32()));
    }

    return true;
}

// [CLIENT_AGENT] Create full snapshot in client-compatible binary format
// Matches NetworkManager_udp.cpp documented format and client NetworkManager.cs expectations
// Format: [type:1=2][server_tick:4][last_input:4][entity_count:4][entity_data...]
// Each entity: [id:4][pos_x:4f][pos_y:4f][pos_z:4f][vel_x:4f][vel_y:4f][vel_z:4f][health:1][anim:1]
std::vector<uint8_t> createFullSnapshot(
    uint32_t serverTick,
    uint32_t lastProcessedInput,
    std::span<const EntityStateData> entities) {

    std::vector<uint8_t> data;
    // Header: 1 + 4 + 4 + 4 = 13 bytes
    // Per entity: 4*7 + 1 + 1 = 30 bytes
    data.reserve(13 + entities.size() * 30);

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
    }

    return data;
}

uint32_t getProtocolVersion() {
    return (1u << 16) | 0u;  // Version 1.0
}

bool isVersionCompatible(uint32_t clientVersion) {
    uint16_t clientMajor = static_cast<uint16_t>(clientVersion >> 16);
    uint16_t serverMajor = static_cast<uint16_t>(getProtocolVersion() >> 16);
    return clientMajor == serverMajor;
}

std::vector<uint8_t> serializeCorrection(
    uint32_t serverTick,
    const Position& position,
    const Velocity& velocity,
    uint32_t lastProcessedInput) {

    std::vector<uint8_t> data;
    data.reserve(1 + 4 + 4*3 + 4*3 + 4);  // type + tick + pos + vel + lastInput

    data.push_back(static_cast<uint8_t>(PacketType::ReliableEvent));  // Placeholder type

    auto appendUInt32 = [&data](uint32_t value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(uint32_t));
    };

    auto appendFloat = [&data](float value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), bytes, bytes + sizeof(float));
    };

    appendUInt32(serverTick);

    float posX = position.x * Constants::FIXED_TO_FLOAT;
    float posY = position.y * Constants::FIXED_TO_FLOAT;
    float posZ = position.z * Constants::FIXED_TO_FLOAT;
    appendFloat(posX);
    appendFloat(posY);
    appendFloat(posZ);

    float velX = velocity.dx * Constants::FIXED_TO_FLOAT;
    float velY = velocity.dy * Constants::FIXED_TO_FLOAT;
    float velZ = velocity.dz * Constants::FIXED_TO_FLOAT;
    appendFloat(velX);
    appendFloat(velY);
    appendFloat(velZ);

    appendUInt32(lastProcessedInput);

    return data;
}

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

} // namespace Protocol

namespace Protocol {
namespace DeltaEncoding {

size_t encodePositionDelta(uint8_t* buffer, size_t bufferSize,
                           const Position& current, const Position& baseline) {
    if (bufferSize < 12) return 0;
    int32_t dx = current.x - baseline.x;
    int32_t dy = current.y - baseline.y;
    int32_t dz = current.z - baseline.z;
    std::memcpy(buffer, &dx, 4);
    std::memcpy(buffer + 4, &dy, 4);
    std::memcpy(buffer + 8, &dz, 4);
    return 12;
}

size_t decodePositionDelta(const uint8_t* buffer, size_t bufferSize,
                           Position& outPosition, const Position& baseline) {
    if (bufferSize < 12) return 0;
    int32_t dx, dy, dz;
    std::memcpy(&dx, buffer, 4);
    std::memcpy(&dy, buffer + 4, 4);
    std::memcpy(&dz, buffer + 8, 4);
    outPosition.x = baseline.x + dx;
    outPosition.y = baseline.y + dy;
    outPosition.z = baseline.z + dz;
    return 12;
}

} // namespace DeltaEncoding
} // namespace Protocol

} // namespace DarkAges
