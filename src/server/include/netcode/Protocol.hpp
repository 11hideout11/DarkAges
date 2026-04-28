#ifndef DARKAGES_PROTOCOL_HPP
#define DARKAGES_PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <string>
#include "ecs/CoreTypes.hpp"

namespace DarkAges {

// Forward declaration for the chat message struct, defined in CoreTypes.hpp
struct ChatMessage;

// Forward declaration for quest update packet
struct QuestUpdatePacket;

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
    DialogueStart = 17,    // Server -> Client: Begin NPC dialogue
    DialogueResponse = 8, // Client -> Server: Player selected dialogue option
    PACKET_CHAT = 14,
    PACKET_QUEST_UPDATE = 15,
    PACKET_QUEST_ACTION = 16
};

// Serialize ChatMessage for UDP delivery
std::vector<uint8_t> serializeChatMessage(const ChatMessage& msg);

// Serialize QuestUpdatePacket to UDP buffer
std::vector<uint8_t> serializeQuestUpdate(const QuestUpdatePacket& pkt);

// ============================================================================
// Dialogue Packet Payloads
// ============================================================================

// Server -> Client: initiate dialogue with an NPC
// Wire format (payload only, no type byte):
//   npcId:4 dialogueId:4 npcName:32 dialogueText:256 optionCount:1 [options...]
// Each option: length:1 text:length (max 128)
struct DialogueStartPacket {
    EntityID npcId{0};           // NPC entity ID
    uint32_t dialogueId{0};      // Dialogue tree/root ID
    char npcName[32]{0};         // Display name of NPC
    char dialogueText[256]{0};   // NPC greeting text
    uint8_t optionCount{0};      // Number of response options (<=6)
    std::vector<std::string> options; // Dynamically allocated option strings
};

// Client -> Server: player selected a dialogue option
// Format: dialogueId:4 selectedOption:1
// Note: connectionId and receiveTimeMs are server-side metadata, not on wire.
struct DialogueResponsePacket {
    uint32_t connectionId{0};    // Filled by NetworkManager upon receipt
    uint32_t dialogueId{0};      // Dialogue session ID
    uint8_t selectedOption{0};   // Zero-based index of chosen response
    uint32_t receiveTimeMs{0};   // Filled by NetworkManager upon receipt
};

// Serialize DialogueStartPacket to UDP buffer
std::vector<uint8_t> serializeDialogueStart(const DialogueStartPacket& pkt);

// Deserialize DialogueStartPacket from UDP buffer
bool deserializeDialogueStart(std::span<const uint8_t> data, DialogueStartPacket& outPkt);

// Serialize DialogueResponsePacket to UDP buffer
std::vector<uint8_t> serializeDialogueResponse(const DialogueResponsePacket& pkt);

// Deserialize DialogueResponsePacket from UDP buffer
bool deserializeDialogueResponse(std::span<const uint8_t> data, DialogueResponsePacket& outPkt);

} // namespace Protocol
} // namespace DarkAges

#endif // DARKAGES_PROTOCOL_HPP
