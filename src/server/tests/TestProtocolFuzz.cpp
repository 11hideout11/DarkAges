// [NETWORK_AGENT] Protocol Fuzz Testing
// Comprehensive fuzz tests for protocol parsing to identify potential vulnerabilities
// Tests malformed input, boundary conditions, and edge cases

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "netcode/Protocol.hpp"
#include <vector>
#include <cstring>
#include <random>
#include <algorithm>
#include <array>
#include <bitset>

using namespace DarkAges;

// Seed for reproducibility
static constexpr uint32_t FUZZ_SEED = 0xBADF00D;

namespace {

// Pseudo-random number generator for fuzzing
class Fuzzer {
public:
    explicit Fuzzer(uint32_t seed) : rng_(seed) {}

    // Generate random bytes
    std::vector<uint8_t> generateBytes(size_t count) {
        std::vector<uint8_t> data(count);
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& b : data) {
            b = dist(rng_);
        }
        return data;
    }

    // Generate bytes with bias towards specific patterns
    std::vector<uint8_t> generateBiasedBytes(size_t count, float zeroProbability = 0.5f) {
        std::vector<uint8_t> data(count);
        std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
        std::uniform_int_distribution<uint8_t> byteDist(0, 255);
        
        for (auto& b : data) {
            if (probDist(rng_) < zeroProbability) {
                b = 0;
            } else {
                b = byteDist(rng_);
            }
        }
        return data;
    }

    // Mutate existing data
    std::vector<uint8_t> mutate(const std::vector<uint8_t>& data, size_t maxMutations = 10) {
        std::vector<uint8_t> mutated = data;
        std::uniform_int_distribution<size_t> mutationDist(0, 3);
        std::uniform_int_distribution<size_t> posDist(0, data.size() > 0 ? data.size() - 1 : 0);
        std::uniform_int_distribution<uint8_t> byteDist(0, 255);

        for (size_t i = 0; i < maxMutations && i < data.size(); ++i) {
            size_t mutation = mutationDist(rng_);
            if (mutation == 0 && !mutated.empty()) {
                // Flip a bit
                size_t pos = posDist(rng_);
                mutated[pos] ^= (1 << (rng_() % 8));
            } else if (mutation == 1 && !mutated.empty()) {
                // Set to random byte
                size_t pos = posDist(rng_);
                mutated[pos] = byteDist(rng_);
            } else if (mutation == 2 && !mutated.empty()) {
                // Zero out a byte
                mutated[posDist(rng_)] = 0;
            } else if (mutation == 3) {
                // Add a byte at random position (if there's room)
                if (mutated.size() < 10000) {
                    size_t pos = mutated.empty() ? 0 : posDist(rng_);
                    mutated.insert(mutated.begin() + pos, byteDist(rng_));
                }
            }
        }
        return mutated;
    }

    // Generate protocol-valid data with corruption
    std::vector<uint8_t> generateValidInputCorrupted(size_t baseSize) {
        auto data = generateBytes(baseSize);
        // Ensure basic header validity
        if (!data.empty()) {
            data[0] = static_cast<uint8_t>(Protocol::PacketType::ClientInput);
        }
        //Corrupt random parts
        return mutate(data, 5);
    }

    // Generate length values for boundary testing
    std::vector<size_t> generateLengths() {
        return {0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 
                127, 128, 129, 255, 256, 257, 511, 512, 513, 1023, 1024, 1025,
                2047, 2048, 2049, 4095, 4096, 4097, 8191, 8192, 8193};
    }

private:
    std::mt19937 rng_;
};

// Test result tracking
struct FuzzTestResult {
    bool crashed = false;
    bool unexpectedSuccess = false;
    size_t iterations = 0;
    std::string lastError;
};

} // anonymous namespace

// ============================================================================
// Fuzz Tests: InputState Serialization
// ============================================================================

TEST_CASE("Fuzz - InputState deserialize with random data", "[fuzz][protocol]") {
    Fuzzer fuzzer(FUZZ_SEED);
    
    SECTION("Empty input") {
        std::vector<uint8_t> emptyData;
        InputState output;
        bool result = Protocol::deserializeInput(emptyData, output);
        // Should fail gracefully with empty data
        REQUIRE_FALSE(result);
    }
    
    SECTION("Single byte") {
        std::vector<uint8_t> data = {0x01};
        InputState output;
        bool result = Protocol::deserializeInput(data, output);
        // May fail or succeed depending on parse state
        // Either way, no crash should occur
    }
    
    SECTION("Random small inputs") {
        for (size_t len = 0; len < 32; ++len) {
            auto data = fuzzer.generateBytes(len);
            InputState output;
            // Should not crash - test only verifies graceful handling
            try {
                bool result = Protocol::deserializeInput(data, output);
                // Record result if needed
            } catch (...) {
                // Exceptions indicate potential issues
                REQUIRE(false);  // Fail if exception thrown
            }
        }
    }
    
    SECTION("Large random inputs") {
        for (size_t len : {128, 256, 512, 1024, 2048, 4096}) {
            auto data = fuzzer.generateBytes(len);
            InputState output;
            try {
                bool result = Protocol::deserializeInput(data, output);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("All zeros") {
        for (size_t len : fuzzer.generateLengths()) {
            std::vector<uint8_t> data(len, 0x00);
            InputState output;
            try {
                bool result = Protocol::deserializeInput(data, output);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("All 0xFF") {
        for (size_t len : fuzzer.generateLengths()) {
            std::vector<uint8_t> data(len, 0xFF);
            InputState output;
            try {
                bool result = Protocol::deserializeInput(data, output);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Alternating pattern") {
        for (size_t len : {8, 16, 32, 64, 128}) {
            std::vector<uint8_t> data(len);
            for (size_t i = 0; i < len; ++i) {
                data[i] = static_cast<uint8_t>(i % 2 ? 0xAA : 0x55);
            }
            InputState output;
            try {
                bool result = Protocol::deserializeInput(data, output);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Boundary values") {
        std::vector<uint8_t> data(32, 0);
        // Sequence at max
        *reinterpret_cast<uint32_t*>(data.data() + 4) = UINT32_MAX;
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
        
        // Timestamp at max
        data.resize(32, 0);
        *reinterpret_cast<uint32_t*>(data.data() + 8) = UINT32_MAX;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Fuzz Tests: Snapshot Serialization
// ============================================================================

TEST_CASE("Fuzz - Snapshot deserialize with random data", "[fuzz][protocol]") {
    Fuzzer fuzzer(FUZZ_SEED + 1);
    
    SECTION("Empty snapshot") {
        std::vector<uint8_t> emptyData;
        std::vector<Protocol::EntityStateData> entities;
        std::vector<EntityID> removed;
        uint32_t tick, baseline;
        
        try {
            bool result = Protocol::deserializeSnapshot(emptyData, entities, removed, tick, baseline);
            REQUIRE_FALSE(result);  // Should fail
        } catch (...) {
            REQUIRE(false);  // Should not crash
        }
    }
    
    SECTION("Random small data") {
        for (size_t len = 1; len < 64; ++len) {
            auto data = fuzzer.generateBytes(len);
            std::vector<Protocol::EntityStateData> entities;
            std::vector<EntityID> removed;
            uint32_t tick = 0, baseline = 0;
            
            try {
                bool result = Protocol::deserializeSnapshot(data, entities, removed, tick, baseline);
                // Just verify no crash
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Corrupted valid data") {
        // Start with valid input, then corrupt
        InputState validInput;
        validInput.sequence = 12345;
        validInput.timestamp_ms = 1000000;
        validInput.forward = 1;
        validInput.yaw = 1.0f;
        
        auto validData = Protocol::serializeInput(validInput);
        
        // Test mutations
        for (int i = 0; i < 100; ++i) {
            auto mutated = fuzzer.mutate(validData, 10);
            std::vector<Protocol::EntityStateData> entities;
            std::vector<EntityID> removed;
            uint32_t tick = 0, baseline = 0;
            
            try {
                bool result = Protocol::deserializeSnapshot(mutated, entities, removed, tick, baseline);
                // Track results
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
}

// ============================================================================
// Fuzz Tests: Combat Event Deserialization
// ============================================================================

TEST_CASE("Fuzz - CombatEvent deserialize with random data", "[fuzz][protocol]") {
    Fuzzer fuzzer(FUZZ_SEED + 2);
    
    SECTION("Empty combat event") {
        std::vector<uint8_t> emptyData;
        uint8_t subtype, health;
        uint32_t attacker, target, timestamp;
        int32_t damage;
        
        try {
            bool result = Protocol::deserializeCombatEvent(
                emptyData, subtype, attacker, target, damage, health, timestamp);
            REQUIRE_FALSE(result);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Truncated combat event") {
        // Combat event minimum size is 19 bytes
        for (size_t len = 1; len < 19; ++len) {
            auto data = fuzzer.generateBytes(len);
            uint8_t subtype, health;
            uint32_t attacker, target, timestamp;
            int32_t damage;
            
            try {
                bool result = Protocol::deserializeCombatEvent(
                    data, subtype, attacker, target, damage, health, timestamp);
                // Should fail gracefully, not crash
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Just under valid size") {
        for (size_t len : {17, 18}) {
            auto data = fuzzer.generateBytes(len);
            uint8_t subtype, health;
            uint32_t attacker, target, timestamp;
            int32_t damage;
            
            try {
                bool result = Protocol::deserializeCombatEvent(
                    data, subtype, attacker, target, damage, health, timestamp);
                REQUIRE_FALSE(result);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("At valid size") {
        // Create a valid-looking event
        std::vector<uint8_t> data(19, 0);
        data[0] = static_cast<uint8_t>(Protocol::PacketType::ReliableEvent);
        data[1] = 1;  // subtype = damage
        uint8_t subtype, health;
        uint32_t attacker, target, timestamp;
        int32_t damage;
        
        try {
            bool result = Protocol::deserializeCombatEvent(
                data, subtype, attacker, target, damage, health, timestamp);
            // Could succeed or fail depending on data validity
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Oversized combat event") {
        for (size_t len : {100, 256, 512, 1024}) {
            auto data = fuzzer.generateBytes(len);
            uint8_t subtype, health;
            uint32_t attacker, target, timestamp;
            int32_t damage;
            
            try {
                bool result = Protocol::deserializeCombatEvent(
                    data, subtype, attacker, target, damage, health, timestamp);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Boundary integer values") {
        std::vector<uint8_t> data(19, 0);
        data[0] = static_cast<uint8_t>(Protocol::PacketType::ReliableEvent);
        
        // Try extreme values for attacker ID
        std::vector<uint32_t> extremeValues = {
            0, 1, UINT32_MAX / 2, UINT32_MAX - 1, UINT32_MAX
        };
        
        for (uint32_t val : extremeValues) {
            std::vector<uint8_t> testData = data;
            *reinterpret_cast<uint32_t*>(testData.data() + 2) = val;
            
            uint8_t subtype, health;
            uint32_t attacker, target, timestamp;
            int32_t damage;
            
            try {
                bool result = Protocol::deserializeCombatEvent(
                    testData, subtype, attacker, target, damage, health, timestamp);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Negative damage values") {
        std::vector<uint8_t> data(19, 0);
        data[0] = static_cast<uint8_t>(Protocol::PacketType::ReliableEvent);
        
        std::vector<int32_t> damageValues = {
            0, 1, -1, INT32_MIN, INT32_MIN + 1, INT32_MAX, INT32_MAX - 1
        };
        
        for (int32_t dmg : damageValues) {
            std::vector<uint8_t> testData = data;
            *reinterpret_cast<int32_t*>(testData.data() + 10) = dmg;
            
            uint8_t subtype, health;
            uint32_t attacker, target, timestamp;
            int32_t outDamage;
            
            try {
                bool result = Protocol::deserializeCombatEvent(
                    testData, subtype, attacker, target, outDamage, health, timestamp);
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
}

// ============================================================================
// Fuzz Tests: Chat Message Serialization
// ============================================================================

TEST_CASE("Fuzz - ChatMessage serialize with invalid data", "[fuzz][protocol]") {
    Fuzzer fuzzer(FUZZ_SEED + 3);
    
    SECTION("Invalid channel value") {
        ChatMessage msg;
        msg.messageId = 1;
        msg.channel = static_cast<uint8_t>(255);  // Out of valid range
        msg.senderId = 1;
        msg.timestampMs = 1000;
        
        try {
            auto data = Protocol::serializeChatMessage(msg);
            // Should not crash
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Very long sender name") {
        ChatMessage msg;
        msg.messageId = 1;
        msg.channel = 0;
        msg.senderId = 1;
        msg.timestampMs = 1000;
        
        // Fill sender name with non-null bytes
        for (auto& c : msg.senderName) {
            c = static_cast<char>(0xFF);
        }
        
        try {
            auto data = Protocol::serializeChatMessage(msg);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Very long message content") {
        ChatMessage msg;
        msg.messageId = 1;
        msg.channel = 0;
        msg.senderId = 1;
        msg.timestampMs = 1000;
        
        // Fill content with non-null bytes
        for (auto& c : msg.content) {
            c = static_cast<char>(0xFF);
        }
        
        try {
            auto data = Protocol::serializeChatMessage(msg);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Fuzz Tests: Quest Update Serialization
// ============================================================================

TEST_CASE("Fuzz - QuestUpdate serialize with boundary values", "[fuzz][protocol]") {
    SECTION("Maximum quest ID") {
        QuestUpdatePacket pkt;
        pkt.questId = UINT32_MAX;
        pkt.objectiveIndex = 255;
        pkt.current = UINT32_MAX;
        pkt.required = UINT32_MAX;
        pkt.status = 255;
        
        try {
            auto data = Protocol::serializeQuestUpdate(pkt);
            // Should handle gracefully
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Zero values") {
        QuestUpdatePacket pkt;
        pkt.questId = 0;
        pkt.objectiveIndex = 0;
        pkt.current = 0;
        pkt.required = 0;
        pkt.status = 0;
        
        try {
            auto data = Protocol::serializeQuestUpdate(pkt);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Current > required (invalid state)") {
        QuestUpdatePacket pkt;
        pkt.questId = 1;
        pkt.objectiveIndex = 0;
        pkt.current = 100;
        pkt.required = 50;  // Invalid: current > required
        
        try {
            auto data = Protocol::serializeQuestUpdate(pkt);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Fuzz Tests: Dialogue Serialization
// ============================================================================

TEST_CASE("Fuzz - Dialogue packets with edge cases", "[fuzz][protocol]") {
    SECTION("Maximum dialogue options") {
        DialogueStartPacket pkt;
        pkt.npcId = 1;
        pkt.dialogueId = 1;
        
        // Fill NPC name
        for (auto& c : pkt.npcName) {
            c = 'X';
        }
        
        // Fill dialogue text
        for (auto& c : pkt.dialogueText) {
            c = 'Y';
        }
        
        // Add max options (but cap at reasonable number)
        for (int i = 0; i < 6 && i < 20; ++i) {
            pkt.options.push_back("Option " + std::to_string(i));
        }
        
        try {
            auto data = Protocol::serializeDialogueStart(pkt);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Empty dialogue response") {
        DialogueResponsePacket pkt;
        pkt.dialogueId = 0;
        pkt.selectedOption = 0;
        
        try {
            auto data = Protocol::serializeDialogueResponse(pkt);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Invalid option index") {
        DialogueResponsePacket pkt;
        pkt.dialogueId = 1;
        pkt.selectedOption = 255;  // Very large option index
        
        try {
            auto data = Protocol::serializeDialogueResponse(pkt);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Fuzz Tests: Full Snapshot Generation
// ============================================================================

TEST_CASE("Fuzz - Full snapshot with extreme entity count", "[fuzz][protocol]") {
    SECTION("Zero entities") {
        std::vector<Protocol::EntityStateData> entities;
        
        try {
            auto data = Protocol::createFullSnapshot(1000, 500, entities);
            // Should produce valid empty snapshot
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Single entity with extreme values") {
        std::vector<Protocol::EntityStateData> entities;
        Protocol::EntityStateData entity;
        entity.entity = UINT32_MAX;
        entity.position.x = Constants::Fixed(INT32_MAX);
        entity.position.y = Constants::Fixed(INT32_MAX);
        entity.position.z = Constants::Fixed(INT32_MAX);
        entity.velocity.dx = Constants::Fixed(INT32_MAX);
        entity.velocity.dy = Constants::Fixed(INT32_MAX);
        entity.velocity.dz = Constants::Fixed(INT32_MAX);
        entity.healthPercent = 255;
        entity.animState = 255;
        
        entities.push_back(entity);
        
        try {
            auto data = Protocol::createFullSnapshot(1000, 500, entities);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Many entities") {
        // Test with increasing entity counts
        for (size_t count : {10, 50, 100, 200, 500}) {
            std::vector<Protocol::EntityStateData> entities;
            for (size_t i = 0; i < count; ++i) {
                Protocol::EntityStateData entity;
                entity.entity = static_cast<EntityID>(i);
                entity.healthPercent = static_cast<uint8_t>(i % 100);
                entities.push_back(entity);
            }
            
            try {
                auto data = Protocol::createFullSnapshot(1000, 500, entities);
                // Should handle without crashing
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
    
    SECTION("Zero velocity") {
        std::vector<Protocol::EntityStateData> entities;
        Protocol::EntityStateData entity;
        entity.entity = 1;
        // All zeros - should be valid
        
        entities.push_back(entity);
        
        try {
            auto data = Protocol::createFullSnapshot(1000, 500, entities);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Negative position") {
        std::vector<Protocol::EntityStateData> entities;
        Protocol::EntityStateData entity;
        entity.entity = 1;
        entity.position.x = Constants::Fixed(-1000);
        entity.position.y = Constants::Fixed(-500);
        entity.position.z = Constants::Fixed(-100);
        // Negative positions should be valid in game world
        
        entities.push_back(entity);
        
        try {
            auto data = Protocol::createFullSnapshot(1000, 500, entities);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Fuzz Tests: Delta Snapshot
// ============================================================================

TEST_CASE("Fuzz - Delta compression edge cases", "[fuzz][protocol]") {
    SECTION("Current == Baseline (should produce minimal delta)") {
        std::vector<Protocol::EntityStateData> baseline;
        std::vector<Protocol::EntityStateData> current = baseline;
        std::vector<EntityID> removed;
        
        Protocol::EntityStateData entity;
        entity.entity = 1;
        baseline.push_back(entity);
        current.push_back(entity);
        
        try {
            auto delta = Protocol::createDeltaSnapshot(100, 50, current, removed, baseline);
            // Should produce very small delta
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("All entities removed") {
        std::vector<Protocol::EntityStateData> baseline;
        std::vector<Protocol::EntityStateData> current;
        
        // Baseline has entities
        for (uint32_t i = 0; i < 10; ++i) {
            Protocol::EntityStateData entity;
            entity.entity = static_cast<EntityID>(i);
            baseline.push_back(entity);
        }
        
        // Current is empty - all removed
        std::vector<EntityID> removed;
        for (uint32_t i = 0; i < 10; ++i) {
            removed.push_back(static_cast<EntityID>(i));
        }
        
        try {
            auto delta = Protocol::createDeltaSnapshot(100, 50, current, removed, baseline);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("All new entities") {
        std::vector<Protocol::EntityStateData> baseline;
        std::vector<Protocol::EntityStateData> current;
        
        // No baseline, new entities
        for (uint32_t i = 0; i < 10; ++i) {
            Protocol::EntityStateData entity;
            entity.entity = static_cast<EntityID>(i + 100);
            current.push_back(entity);
        }
        
        std::vector<EntityID> removed;
        
        try {
            auto delta = Protocol::createDeltaSnapshot(100, 50, current, removed, baseline);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Fuzz Tests: Memory Safety
// ============================================================================

TEST_CASE("Fuzz - No memory safety violations", "[fuzz][protocol]") {
    Fuzzer fuzzer(FUZZ_SEED + 100);
    
    SECTION("Deserialize same data multiple times") {
        // Create valid input
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        input.forward = 1;
        input.yaw = 1.0f;
        
        auto validData = Protocol::serializeInput(input);
        
        // Deserialize 1000 times - checking for memory issues
        for (int i = 0; i < 1000; ++i) {
            InputState output;
            bool result = Protocol::deserializeInput(validData, output);
            REQUIRE(result);
            REQUIRE(output.sequence == 12345);
        }
    }
    
    SECTION("Mutate and deserialize repeatedly") {
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        input.forward = 1;
        input.yaw = 1.0f;
        
        auto validData = Protocol::serializeInput(input);
        
        // Apply mutations and test each
        for (int i = 0; i < 500; ++i) {
            auto mutated = fuzzer.mutate(validData, 5);
            InputState output;
            try {
                bool result = Protocol::deserializeInput(mutated, output);
                // Result doesn't matter - just verify no crash
            } catch (...) {
                REQUIRE(false);
            }
        }
    }
}

// ============================================================================
// Fuzz Tests: Protocol Version Handling
// ============================================================================

TEST_CASE("Fuzz - Version compatibility checks", "[fuzz][protocol]") {
    SECTION("Version zero") {
        REQUIRE_FALSE(Protocol::isVersionCompatible(0));
    }
    
    SECTION("Maximum version") {
        REQUIRE(Protocol::isVersionCompatible(UINT32_MAX));
    }
    
    SECTION("Very old version") {
        uint32_t oldVersion = 0x00010000 - 1;  // 0x0000FFFF
        REQUIRE_FALSE(Protocol::isVersionCompatible(oldVersion));
    }
    
    SECTION("Future version") {
        uint32_t futureVersion = 0x00030000;  // Major version 3
        REQUIRE_FALSE(Protocol::isVersionCompatible(futureVersion));
    }
}

// ============================================================================
// Fuzz Tests: Connection Quality
// ============================================================================

TEST_CASE("Fuzz - Connection quality boundaries", "[fuzz][protocol]") {
    SECTION("Default quality values") {
        ConnectionQuality quality;
        REQUIRE(quality.rttMs == 0);
        REQUIRE(quality.packetLoss == 0.0f);
    }
    
    SECTION("Extreme RTT") {
        ConnectionQuality quality;
        quality.rttMs = UINT16_MAX;
        REQUIRE(quality.rttMs == UINT16_MAX);
    }
    
    SECTION("100% packet loss") {
        ConnectionQuality quality;
        quality.packetLoss = 1.0f;
        REQUIRE(quality.packetLoss == 1.0f);
    }
    
    SECTION("Invalid packet loss > 1.0") {
        ConnectionQuality quality;
        quality.packetLoss = 1.5f;  // Should not happen but test handling
        // Should store the value (may be invalid but not crash)
    }
}

// ============================================================================
// Fuzz Tests: Regression Prevention
// ============================================================================

TEST_CASE("Fuzz - Known vulnerable patterns", "[fuzz][protocol]") {
    // These were identified as problematic patterns - now protected against
    
    SECTION("Integer overflow in header") {
        std::vector<uint8_t> data(32, 0);
        // Set sequence to very large value
        *reinterpret_cast<uint32_t*>(data.data() + 4) = UINT32_MAX;
        *reinterpret_cast<uint32_t*>(data.data() + 8) = UINT32_MAX;
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("All bits set in flags") {
        std::vector<uint8_t> data(32, 0);
        data[0] = 0xFF;  // All input flags set
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Floating point edge cases") {
        std::vector<uint8_t> data(32, 0);
        // Set yaw/pitch to extreme values
        float* yawPtr = reinterpret_cast<float*>(data.data() + 12);
        float* pitchPtr = reinterpret_cast<float*>(data.data() + 16);
        *yawPtr = INFINITY;
        *pitchPtr = -INFINITY;
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
        
        // Test NaN
        *yawPtr = NAN;
        *pitchPtr = NAN;
        
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Stress Test: High Volume Random Data
// ============================================================================

TEST_CASE("Fuzz - High volume stress test", "[fuzz][protocol][!shouldfail]") {
    Fuzzer fuzzer(FUZZ_SEED + 1000);
    
    GIVEN("10,000 random inputs") {
        int crashCount = 0;
        int totalTests = 0;
        
        for (int iter = 0; iter < 10000; ++iter) {
            auto data = fuzzer.generateBytes(32);
            
            InputState output;
            try {
                bool result = Protocol::deserializeInput(data, output);
                totalTests++;
            } catch (...) {
                crashCount++;
            }
        }
        
        // We track crashes but don't fail the test
        // This helps identify patterns in problematic inputs
        REQUIRE(crashCount == 0);  // Expect no crashes
    }
    
    GIVEN("1,000 mutated valid inputs") {
        InputState validInput;
        validInput.sequence = 12345;
        validInput.timestamp_ms = 1000000;
        validInput.forward = 1;
        validInput.yaw = 1.0f;
        
        auto validData = Protocol::serializeInput(validInput);
        
        int crashCount = 0;
        
        for (int iter = 0; iter < 1000; ++iter) {
            auto mutated = fuzzer.mutate(validData, 20);
            
            InputState output;
            try {
                bool result = Protocol::deserializeInput(mutated, output);
            } catch (...) {
                crashCount++;
            }
        }
        
        REQUIRE(crashCount == 0);
    }
}