#ifndef DARKAGES_PROTOCOL_HPP
#define DARKAGES_PROTOCOL_HPP

#include <cstdint>
#include <vector>

namespace DarkAges {

// Forward declaration for the chat message struct, defined in CoreTypes.hpp
struct ChatMessage;

namespace Protocol {

enum class MessageType : uint8_t {
    Unknown = 0,
    PlayerMove = 1,
    PlayerUpdate = 2,
    CombatHit = 3,
    ZoneTransfer = 4
};

// Raw UDP packet types (mirrored in NetworkManager.hpp and client)
enum class PacketType : uint8_t {
    ClientInput = 1,
    ServerSnapshot = 2,
    ReliableEvent = 3,
    Ping = 4,
    Handshake = 5,
    Disconnect = 6,
    PACKET_CHAT = 14
};

// Serialize ChatMessage for UDP delivery
std::vector<uint8_t> serializeChatMessage(const ChatMessage& msg);

} // namespace Protocol
} // namespace DarkAges

#endif // DARKAGES_PROTOCOL_HPP
