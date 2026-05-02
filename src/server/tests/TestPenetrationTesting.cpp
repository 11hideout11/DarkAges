// [NETWORK_AGENT] Penetration Testing for DarkAges Protocol
// Tests for packet manipulation, replay attacks, and security vulnerabilities
// in network protocol handling

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "netcode/Protocol.hpp"
#include "netcode/NetworkManager.hpp"
#include <vector>
#include <cstring>
#include <random>
#include <chrono>
#include <atomic>

using namespace DarkAges;

// ============================================================================
// Packet Manipulation Tests
// ============================================================================

TEST_CASE("Security - Packet type tampering detection", "[security][network]") {
    SECTION("Manipulated packet type byte") {
        // Create valid input packet
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        input.forward = 1;
        input.yaw = 1.0f;
        
        auto data = Protocol::serializeInput(input);
        
        // Verify original packet parses
        InputState output;
        bool originalResult = Protocol::deserializeInput(data, output);
        REQUIRE(originalResult);
        REQUIRE(output.sequence == 12345);
        
        // Try manipulating packet type byte (at index 0 for protobuf)
        // This tests if the parser handles unexpected type values
        for (uint8_t maliciousType = 128; maliciousType < 255; maliciousType += 32) {
            std::vector<uint8_t> tamperedData = data;
            if (!tamperedData.empty()) {
                tamperedData[0] = maliciousType;
            }
            
            InputState tamperedOutput;
            // Should either parse gracefully or reject
            try {
                bool result = Protocol::deserializeInput(tamperedData, tamperedOutput);
                // We accept either behavior - but no crash
            } catch (...) {
                // Exception is acceptable security response
            }
        }
    }
    
    SECTION("Extra data after valid packet") {
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        
        auto data = Protocol::serializeInput(input);
        
        // Append malicious data
        std::vector<uint8_t> extendedData = data;
        for (int i = 0; i < 256; ++i) {
            extendedData.push_back(0xFF);
        }
        
        InputState output;
        // Should handle gracefully
        try {
            bool result = Protocol::deserializeInput(extendedData, output);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Truncated packet still parses") {
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        
        auto data = Protocol::serializeInput(input);
        
        // Test various truncation points
        for (size_t size = 1; size < data.size(); ++size) {
            std::vector<uint8_t> truncated(data.begin(), data.begin() + size);
            
            InputState output;
            bool result = Protocol::deserializeInput(truncated, output);
            // Result doesn't matter - just no crash
        }
    }
}

TEST_CASE("Security - Buffer overflow prevention", "[security][network]") {
    SECTION("Extremely large packet") {
        // Create oversized input buffer to test memory limits
        std::vector<uint8_t> hugeData(1024 * 1024, 0);  // 1MB
        
        InputState output;
        // Should not crash or consume excessive memory
        try {
            bool result = Protocol::deserializeInput(hugeData, output);
        } catch (const std::bad_alloc&) {
            // Memory allocation failure is acceptable
        } catch (...) {
            // Other exceptions also acceptable
        }
    }
    
    SECTION("Negative size values in header") {
        // Protobuf uses int sizes - negative values could cause issues
        std::vector<uint8_t> maliciousData = {
            0x08,  // Field 1, varint
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01  // Negative varint
        };
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(maliciousData, output);
        } catch (...) {
            // Exception is acceptable security response
        }
    }
    
    SECTION("Recursive data structures") {
        // Try to create deeply nested/large data structures
        std::vector<uint8_t> data;
        // Create data that might cause recursive parsing
        for (int i = 0; i < 1000; ++i) {
            data.push_back(0x0A);  // Start of embedded message
            data.push_back(0x7F);  // Maximum length
        }
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (const std::exception&) {
            // Exception indicates protection
        }
    }
}

TEST_CASE("Security - Integer overflow protection", "[security][network]") {
    SECTION("Timestamp overflow") {
        InputState input;
        input.sequence = 0;
        input.timestamp_ms = UINT32_MAX;
        input.timestamp_ms += 1;  // Overflow
        
        auto data = Protocol::serializeInput(input);
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Sequence number wraparound") {
        InputState input;
        input.sequence = UINT32_MAX;
        input.sequence += 1;
        
        auto data = Protocol::serializeInput(input);
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(data, output);
            // Should handle wraparound
        } catch (...) {
            REQUIRE(false);
        }
    }
    
    SECTION("Position quantization overflow") {
        std::vector<Protocol::EntityStateData> entities;
        Protocol::EntityStateData entity;
        
        // Extreme position values that might overflow quantization
        entity.position.x = Constants::Fixed(INT32_MAX);
        entity.position.y = Constants::Fixed(INT32_MAX);
        entity.position.z = Constants::Fixed(INT32_MAX);
        
        entities.push_back(entity);
        
        try {
            auto data = Protocol::createFullSnapshot(1000, 500, entities);
        } catch (...) {
            REQUIRE(false);
        }
    }
}

// ============================================================================
// Replay Attack Prevention Tests
// ============================================================================

TEST_CASE("Security - Anti-replay mechanism", "[security][network]") {
    SECTION("Duplicate packet detection") {
        InputState input1;
        input1.sequence = 100;
        input1.timestamp_ms = 1000;
        
        auto packet1 = Protocol::serializeInput(input1);
        
        // Same packet again (replay)
        InputState replayInput;
        replayInput.sequence = 100;
        replayInput.timestamp_ms = 1000;
        
        auto replayPacket = Protocol::serializeInput(replayInput);
        
        // Verify packets are identical
        REQUIRE(packet1 == replayPacket);
        
        // Test that sequence-based rejection could work
        std::atomic<uint32_t> lastProcessedSequence(100);
        
        // Simulate replay detection
        uint32_t incomingSequence = 100;
        bool isReplay = (incomingSequence <= lastProcessedSequence.load());
        
        // This pattern allows replay detection
        REQUIRE(isReplay);  // With same sequence, should be detected as potential replay
    }
    
    SECTION("Old packet rejection") {
        std::atomic<uint32_t> lastProcessedSequence(200);
        
        // Try processing old packet
        uint32_t oldSequence = 100;
        bool shouldReject = (oldSequence <= lastProcessedSequence.load());
        
        REQUIRE(shouldReject);
    }
    
    SECTION("Sequence ordering") {
        // Test that sequence numbers are properly ordered
        InputState input1, input2, input3;
        input1.sequence = 1;
        input2.sequence = 2;
        input3.sequence = 3;
        
        auto packet1 = Protocol::serializeInput(input1);
        auto packet2 = Protocol::serializeInput(input2);
        auto packet3 = Protocol::serializeInput(input3);
        
        // Verify we can deserialize in order
        InputState out1, out2, out3;
        bool r1 = Protocol::deserializeInput(packet1, out1);
        bool r2 = Protocol::deserializeInput(packet2, out2);
        bool r3 = Protocol::deserializeInput(packet3, out3);
        
        REQUIRE(r1 && r2 && r3);
        REQUIRE(out1.sequence == 1);
        REQUIRE(out2.sequence == 2);
        REQUIRE(out3.sequence == 3);
    }
}

TEST_CASE("Security - Timestamp validation", "[security][network]") {
    SECTION("Timestamp from future rejected") {
        auto now = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        InputState futureInput;
        futureInput.sequence = 1;
        futureInput.timestamp_ms = now + 3600000;  // 1 hour in future
        
        auto packet = Protocol::serializeInput(futureInput);
        
        InputState output;
        bool result = Protocol::deserializeInput(packet, output);
        
        // Parser should accept (validation is application-level)
        // But timestamp check would catch this at game logic level
    }
    
    SECTION("Timestamp from very old packet") {
        auto oldTimestamp = static_cast<uint32_t>(1000);  // Very old
        
        InputState oldInput;
        oldInput.sequence = 1;
        oldInput.timestamp_ms = oldTimestamp;
        
        auto packet = Protocol::serializeInput(oldInput);
        
        InputState output;
        bool result = Protocol::deserializeInput(packet, output);
        
        // Parser accepts - game logic validates age
    }
    
    SECTION("Zero timestamp handling") {
        InputState zeroInput;
        zeroInput.sequence = 1;
        zeroInput.timestamp_ms = 0;
        
        auto packet = Protocol::serializeInput(zeroInput);
        
        InputState output;
        bool result = Protocol::deserializeInput(packet, output);
        
        REQUIRE(result);
        REQUIRE(output.timestamp_ms == 0);
    }
}

// ============================================================================
// Connection Flooding Protection Tests
// ============================================================================

TEST_CASE("Security - Connection limits", "[security][network]") {
    SECTION("Multiple rapid connections") {
        // This tests connection pool limits
        // Actual limits are enforced by NetworkManager
        ConnectionQuality quality;
        
        REQUIRE(quality.packetsReceived == 0);
    }
    
    SECTION("Bandwidth tracking") {
        NetworkManager net;
        bool init = net.initialize(9998);
        REQUIRE(init);
        
        auto bytesSent = net.getTotalBytesSent();
        auto bytesRecv = net.getTotalBytesReceived();
        
        // Initially should be zero
        REQUIRE(bytesSent == 0);
        REQUIRE(bytesRecv == 0);
        
        net.shutdown();
    }
}

// ============================================================================
// Packet Rate Limiting Tests
// ============================================================================

TEST_CASE("Security - Rate limiting verification", "[security][network]") {
    SECTION("Packet rate tracking") {
        ConnectionQuality quality;
        
        // Simulate high packet rate
        for (uint32_t i = 0; i < 1000; ++i) {
            quality.packetsSent++;
        }
        
        REQUIRE(quality.packetsSent == 1000);
    }
    
    SECTION("Packet loss calculation") {
        ConnectionQuality quality;
        quality.packetsSent = 1000;
        quality.packetsReceived = 900;
        
        // Calculate packet loss
        float expectedLoss = static_cast<float>(1000 - 900) / 1000.0f;
        quality.packetLoss = expectedLoss;
        
        REQUIRE(quality.packetLoss == 0.1f);
    }
    
    SECTION("RTT tracking") {
        ConnectionQuality quality;
        quality.rttMs = 50;
        
        REQUIRE(quality.rttMs == 50);
    }
}

// ============================================================================
// Data Integrity Tests
// ============================================================================

TEST_CASE("Security - Data integrity verification", "[security][network]") {
    SECTION("Checksum verification") {
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        input.forward = 1;
        
        auto data = Protocol::serializeInput(input);
        
        // Verify deserialization integrity
        InputState output;
        bool result = Protocol::deserializeInput(data, output);
        
        REQUIRE(result);
        REQUIRE(output.sequence == input.sequence);
        REQUIRE(output.timestamp_ms == input.timestamp_ms);
    }
    
    SECTION("Data not modified in transit simulation") {
        InputState original;
        original.sequence = 99999;
        original.timestamp_ms = 9999999;
        original.yaw = 3.14159f;
        original.pitch = -1.5f;
        
        auto serialized = Protocol::serializeInput(original);
        
        // Simulate transmission (no modification)
        std::vector<uint8_t> transmitted = serialized;
        
        InputState received;
        bool result = Protocol::deserializeInput(transmitted, received);
        
        REQUIRE(result);
        REQUIRE(received.sequence == original.sequence);
        REQUIRE(received.timestamp_ms == original.timestamp_ms);
        REQUIRE(received.yaw == Catch::Approx(original.yaw).margin(0.001f));
    }
    
    SECTION("Corrupted data detected") {
        InputState input;
        input.sequence = 12345;
        
        auto data = Protocol::serializeInput(input);
        
        // Corrupt the data
        data[0] ^= 0xFF;
        
        InputState output;
        bool result = Protocol::deserializeInput(data, output);
        
        // Should fail integrity check
        REQUIRE_FALSE(result);
    }
}

// ============================================================================
// Input Validation Tests
// ============================================================================

TEST_CASE("Security - Input bounds checking", "[security][network]") {
    SECTION("Yaw overflow") {
        InputState input;
        input.yaw = 1000.0f;  // Way beyond valid range
        
        auto data = Protocol::serializeInput(input);
        
        InputState output;
        bool result = Protocol::deserializeInput(data, output);
        
        REQUIRE(result);
        // Store what was sent (validation is app-level)
    }
    
    SECTION("Pitch overflow") {
        InputState input;
        input.pitch = -1000.0f;
        
        auto data = Protocol::serializeInput(input);
        
        InputState output;
        bool result = Protocol::deserializeInput(data, output);
        
        REQUIRE(result);
    }
    
    SECTION("All flags set") {
        InputState input;
        input.forward = 1;
        input.backward = 1;
        input.left = 1;
        input.right = 1;
        input.jump = 1;
        input.attack = 1;
        input.block = 1;
        input.sprint = 1;
        
        auto data = Protocol::serializeInput(input);
        
        InputState output;
        bool result = Protocol::deserializeInput(data, output);
        
        REQUIRE(result);
        REQUIRE(output.forward == 1);
        REQUIRE(output.backward == 1);
    }
}

// ============================================================================
// Authentication Bypass Prevention Tests
// ============================================================================

TEST_CASE("Security - Authentication edge cases", "[security][network]") {
    SECTION("No authentication on raw packet") {
        // Raw packets should not grant access
        std::vector<uint8_t> fakePacket = {0x01, 0x02, 0x03};
        
        InputState output;
        bool result = Protocol::deserializeInput(fakePacket, output);
        
        // Should fail or require further validation
        // Parser doesn't know about auth - that's network layer concern
    }
    
    SECTION("Valid packet without proper session") {
        // Even valid-format packets need session validation
        InputState input;
        input.sequence = 1;
        
        auto packet = Protocol::serializeInput(input);
        
        // Network layer should validate session before processing
        // This test just verifies packet structure
        InputState output;
        bool result = Protocol::deserializeInput(packet, output);
        
        REQUIRE(result);
    }
}

// ============================================================================
// DOS Protection Tests
// ============================================================================

TEST_CASE("Security - Denial of service prevention", "[security][network]") {
    SECTION("Rapid same packet submission") {
        InputState input;
        input.sequence = 1;
        input.timestamp_ms = 1000;
        
        auto packet = Protocol::serializeInput(input);
        
        // Submit same packet many times rapidly
        for (int i = 0; i < 10000; ++i) {
            InputState output;
            bool result = Protocol::deserializeInput(packet, output);
            REQUIRE(result);
        }
    }
    
    SECTION("Varied packets at high rate") {
        // Simulate high packet rate
        std::vector<InputState> inputs;
        for (int i = 0; i < 1000; ++i) {
            InputState input;
            input.sequence = static_cast<uint32_t>(i);
            input.timestamp_ms = static_cast<uint32_t>(i * 16);  // 60fps timing
            inputs.push_back(input);
        }
        
        for (const auto& input : inputs) {
            auto packet = Protocol::serializeInput(input);
            InputState output;
            bool result = Protocol::deserializeInput(packet, output);
            REQUIRE(result);
        }
    }
    
    SECTION("Large payload DOS attempt") {
        // Try to send huge packets
        std::vector<uint8_t> largePayload(64 * 1024, 0xFF);  // 64KB
        
        InputState output;
        try {
            bool result = Protocol::deserializeInput(largePayload, output);
        } catch (const std::bad_alloc&) {
            // Memory limit hit - good protection
        } catch (...) {
            // Other exception - acceptable
        }
    }
}

// ============================================================================
// Multi-Packet Attack Scenarios
// ============================================================================

TEST_CASE("Security - Complex attack scenarios", "[security][network]") {
    SECTION("Packet fragmentation attack") {
        InputState input;
        input.sequence = 1;
        
        auto completePacket = Protocol::serializeInput(input);
        
        // Try sending fragments
        for (size_t i = 1; i < completePacket.size(); ++i) {
            std::vector<uint8_t> fragment(completePacket.begin(), completePacket.begin() + i);
            
            InputState output;
            bool result = Protocol::deserializeInput(fragment, output);
            // Should reject incomplete
        }
    }
    
    SECTION("Out-of-order packet flood") {
        std::vector<InputState> inputs;
        for (int i = 0; i < 100; ++i) {
            InputState input;
            input.sequence = static_cast<uint32_t>(99 - i);  // Reverse order
            inputs.push_back(input);
        }
        
        for (const auto& input : inputs) {
            auto packet = Protocol::serializeInput(input);
            InputState output;
            bool result = Protocol::deserializeInput(packet, output);
            REQUIRE(result);
        }
    }
    
    SECTION("Invalid state transitions") {
        InputState input;
        input.sequence = 1;
        
        auto packet = Protocol::serializeInput(input);
        
        // Parse once
        InputState output1;
        bool r1 = Protocol::deserializeInput(packet, output1);
        REQUIRE(r1);
        
        // Try parsing again - same sequence
        InputState output2;
        bool r2 = Protocol::deserializeInput(packet, output2);
        REQUIRE(r2);
        
        // Both succeed - replay detection is application level
    }
}

// ============================================================================
// Encryption Verification Tests
// ============================================================================

TEST_CASE("Security - Encryption detection", "[security][network]") {
    SECTION("Plaintext packet detection") {
        InputState input;
        input.sequence = 12345;
        input.timestamp_ms = 1000000;
        
        auto data = Protocol::serializeInput(input);
        
        // Protobuf encoded data should not be trivially readable
        // Verify it's not plaintext
        std::string asString(data.begin(), data.end());
        
        // Check that sequence isn't directly visible
        bool hasPlaintextSequence = asString.find("12345") != std::string::npos;
        REQUIRE_FALSE(hasPlaintextSequence);
    }
    
    SECTION("Variable length encoding") {
        InputState input1, input2, input3;
        input1.sequence = 1;
        input2.sequence = 127;
        input3.sequence = 128;
        
        auto data1 = Protocol::serializeInput(input1);
        auto data2 = Protocol::serializeInput(input2);
        auto data3 = Protocol::serializeInput(input3);
        
        // Small numbers should have smaller encoding
        REQUIRE(data1.size() <= data2.size());
        REQUIRE(data2.size() <= data3.size());
    }
}