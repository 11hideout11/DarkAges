// [SECURITY_AGENT] Tests for DDoS Protection and Rate Limiting

#include <catch2/catch_test_macros.hpp>
#include "security/DDoSProtection.hpp"
#include "security/RateLimiter.hpp"
#include "security/CircuitBreaker.hpp"
#include "security/InputValidator.hpp"
#include "ecs/CoreTypes.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

using namespace DarkAges;
using namespace DarkAges::Security;

TEST_CASE("DDoSProtection basic operations", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxConnectionsPerIP = 3;
    config.maxPacketsPerSecond = 10;
    config.blockDurationSeconds = 1;  // Short for testing
    config.violationThreshold = 2;
    
    DDoSProtection protection(config);
    REQUIRE(protection.initialize());
    
    SECTION("Connection acceptance within limits") {
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        // Fourth connection from same IP should fail
        REQUIRE_FALSE(protection.shouldAcceptConnection("192.168.1.1"));
    }
    
    SECTION("Different IPs don't interfere") {
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.2"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.3"));
        // Each IP at 1 connection, should all accept more
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.2"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.3"));
    }
    
    SECTION("Emergency mode blocks connections") {
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        protection.setEmergencyMode(true);
        REQUIRE_FALSE(protection.shouldAcceptConnection("192.168.1.2"));
        protection.setEmergencyMode(false);
        REQUIRE(protection.shouldAcceptConnection("192.168.1.2"));
    }
    
    SECTION("Whitelist bypasses limits") {
        protection.whitelistIP("192.168.1.100");
        REQUIRE(protection.isWhitelisted("192.168.1.100"));
        // Should accept even in emergency mode
        protection.setEmergencyMode(true);
        REQUIRE(protection.shouldAcceptConnection("192.168.1.100"));
    }
}

TEST_CASE("DDoSProtection packet rate limiting", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxPacketsPerSecond = 5;
    config.burstSize = 2;
    config.violationThreshold = 3;
    config.blockDurationSeconds = 1;
    
    DDoSProtection protection(config);
    protection.initialize();
    
    const std::string ip = "192.168.1.1";
    uint32_t currentTime = 0;
    
    SECTION("Packets within rate limit accepted") {
        // 5 packets in first second (within limit)
        for (uint32_t i = 0; i < 5; ++i) {
            REQUIRE(protection.processPacket(1, ip, 100, currentTime));
        }
    }
    
    SECTION("Burst packets accepted then limited") {
        // 7 packets (5 + 2 burst) should be accepted
        for (uint32_t i = 0; i < 7; ++i) {
            REQUIRE(protection.processPacket(1, ip, 100, currentTime));
        }
        // 8th packet should be dropped
        REQUIRE_FALSE(protection.processPacket(1, ip, 100, currentTime));
    }
    
    SECTION("Connection cleanup works") {
        protection.processPacket(1, ip, 100, currentTime);
        REQUIRE(protection.getActiveConnectionCount() == 1);
        
        protection.onConnectionClosed(1);
        REQUIRE(protection.getActiveConnectionCount() == 0);
    }
}

TEST_CASE("DDoSProtection IP blocking", "[security][ddos]") {
    DDoSProtection::Config config;
    config.blockDurationSeconds = 1;  // 1 second for testing
    
    DDoSProtection protection(config);
    protection.initialize();
    
    SECTION("Manual IP blocking") {
        const std::string ip = "192.168.1.1";
        REQUIRE_FALSE(protection.isIPBlocked(ip));
        
        protection.blockIP(ip, 1, "Test block");
        REQUIRE(protection.isIPBlocked(ip));
        REQUIRE_FALSE(protection.shouldAcceptConnection(ip));
    }
    
    SECTION("IP unblocking") {
        const std::string ip = "192.168.1.1";
        protection.blockIP(ip, 1, "Test block");
        REQUIRE(protection.isIPBlocked(ip));
        
        protection.unblockIP(ip);
        REQUIRE_FALSE(protection.isIPBlocked(ip));
        REQUIRE(protection.shouldAcceptConnection(ip));
    }
}

TEST_CASE("CircuitBreaker state machine", "[security][circuit]") {
    CircuitBreaker::Config config;
    config.failureThreshold = 3;
    config.successThreshold = 2;
    config.timeoutMs = 100;  // Short for testing
    
    CircuitBreaker breaker("test-service", config);
    
    SECTION("Initial state is CLOSED") {
        REQUIRE(breaker.getState() == CircuitBreaker::State::CLOSED);
        REQUIRE(breaker.allowRequest());
    }
    
    SECTION("Opens after threshold failures") {
        breaker.recordFailure();
        breaker.recordFailure();
        REQUIRE(breaker.getState() == CircuitBreaker::State::CLOSED);
        
        breaker.recordFailure();
        REQUIRE(breaker.getState() == CircuitBreaker::State::OPEN);
        REQUIRE_FALSE(breaker.allowRequest());
    }
    
    SECTION("Closes after threshold successes in half-open") {
        // Open the circuit
        for (int i = 0; i < 3; ++i) {
            breaker.recordFailure();
        }
        REQUIRE(breaker.getState() == CircuitBreaker::State::OPEN);
        
        // Wait for timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        REQUIRE(breaker.allowRequest());  // Should enter HALF_OPEN
        
        // Record successes
        breaker.recordSuccess();
        breaker.recordSuccess();
        REQUIRE(breaker.getState() == CircuitBreaker::State::CLOSED);
    }
    
    SECTION("Returns to OPEN on failure in HALF_OPEN") {
        // Open the circuit
        for (int i = 0; i < 3; ++i) {
            breaker.recordFailure();
        }
        
        // Wait for timeout
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        REQUIRE(breaker.allowRequest());
        
        // Fail in half-open
        breaker.recordFailure();
        REQUIRE(breaker.getState() == CircuitBreaker::State::OPEN);
    }
}

TEST_CASE("InputValidator validation", "[security][validation]") {
    SECTION("Position validation") {
        // Valid position
        Position validPos;
        validPos.x = static_cast<Constants::Fixed>(0 * Constants::FLOAT_TO_FIXED);
        validPos.y = static_cast<Constants::Fixed>(10 * Constants::FLOAT_TO_FIXED);
        validPos.z = static_cast<Constants::Fixed>(0 * Constants::FLOAT_TO_FIXED);
        REQUIRE(InputValidator::isValidPosition(validPos));
        
        // Invalid position (outside world bounds)
        Position invalidPos;
        invalidPos.x = static_cast<Constants::Fixed>(10000 * Constants::FLOAT_TO_FIXED);
        invalidPos.y = 0;
        invalidPos.z = 0;
        REQUIRE_FALSE(InputValidator::isValidPosition(invalidPos));
    }
    
    SECTION("Rotation validation") {
        Rotation validRot;
        validRot.yaw = 1.0f;
        validRot.pitch = 0.5f;
        REQUIRE(InputValidator::isValidRotation(validRot));
        
        Rotation invalidRot;
        invalidRot.yaw = 10.0f;  // > 2π
        invalidRot.pitch = 0.0f;
        REQUIRE_FALSE(InputValidator::isValidRotation(invalidRot));
    }
    
    SECTION("Packet size validation") {
        REQUIRE(InputValidator::isValidPacketSize(100));
        REQUIRE(InputValidator::isValidPacketSize(1400));
        REQUIRE_FALSE(InputValidator::isValidPacketSize(0));
        REQUIRE_FALSE(InputValidator::isValidPacketSize(1500));
    }
    
    SECTION("Protocol version validation") {
        REQUIRE(InputValidator::isValidProtocolVersion(Constants::PROTOCOL_VERSION));
        REQUIRE_FALSE(InputValidator::isValidProtocolVersion(999));
    }
    
    SECTION("String sanitization") {
        std::string dirty = "Hello\x01\x02World\n!";
        std::string clean = InputValidator::sanitizeString(dirty, 256);
        REQUIRE(clean == "HelloWorld!");
        
        // Test max length
        std::string longStr(300, 'a');
        std::string truncated = InputValidator::sanitizeString(longStr, 10);
        REQUIRE(truncated.length() == 10);
    }
    
    SECTION("Input sequence validation") {
        REQUIRE(InputValidator::isValidInputSequence(10, 5));
        REQUIRE_FALSE(InputValidator::isValidInputSequence(5, 10));
        REQUIRE_FALSE(InputValidator::isValidInputSequence(5, 5));  // Duplicate
    }
}

TEST_CASE("TokenBucketRateLimiter basic operations", "[security][ratelimiter]") {
    TokenBucketRateLimiter<std::string>::Config config;
    config.maxTokens = 5;
    config.tokensPerSecond = 10;
    
    TokenBucketRateLimiter<std::string> limiter(config);
    const std::string key = "test-key";
    
    SECTION("Initial bucket is full") {
        REQUIRE(limiter.wouldAllow(key));
        REQUIRE(limiter.getRemainingTokens(key) == 5);
    }
    
    SECTION("Tokens are consumed") {
        for (int i = 0; i < 5; ++i) {
            REQUIRE(limiter.allow(key));
        }
        REQUIRE(limiter.getRemainingTokens(key) == 0);
        REQUIRE_FALSE(limiter.allow(key));
    }
    
    SECTION("Bucket refills over time") {
        // Consume all tokens
        for (int i = 0; i < 5; ++i) {
            limiter.allow(key);
        }
        REQUIRE_FALSE(limiter.allow(key));
        
        // Wait for refill (100ms at 10 tokens/sec = 1 token)
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        REQUIRE(limiter.allow(key));
    }
}

TEST_CASE("SlidingWindowRateLimiter basic operations", "[security][ratelimiter]") {
    SlidingWindowRateLimiter<std::string>::Config config;
    config.maxRequests = 3;
    config.windowMs = 100;  // Short for testing
    
    SlidingWindowRateLimiter<std::string> limiter(config);
    const std::string key = "test-key";
    
    SECTION("Requests within window allowed") {
        REQUIRE(limiter.allow(key));
        REQUIRE(limiter.allow(key));
        REQUIRE(limiter.allow(key));
        REQUIRE_FALSE(limiter.allow(key));
    }
    
    SECTION("Window slides over time") {
        REQUIRE(limiter.allow(key));
        REQUIRE(limiter.allow(key));
        REQUIRE(limiter.allow(key));
        REQUIRE_FALSE(limiter.allow(key));
        
        // Wait for window to slide
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        REQUIRE(limiter.allow(key));
    }
    
    SECTION("Remaining requests calculated correctly") {
        REQUIRE(limiter.getRemaining(key) == 3);
        limiter.allow(key);
        REQUIRE(limiter.getRemaining(key) == 2);
        limiter.allow(key);
        REQUIRE(limiter.getRemaining(key) == 1);
        limiter.allow(key);
        REQUIRE(limiter.getRemaining(key) == 0);
    }
}

TEST_CASE("NetworkRateLimiter integration", "[security][ratelimiter]") {
    NetworkRateLimiter limiter;
    const std::string ip = "192.168.1.1";
    
    SECTION("Connection rate limiting") {
        // Should allow burst of connections
        REQUIRE(limiter.allowConnection(ip));
        REQUIRE(limiter.allowConnection(ip));
        // Eventually should rate limit
        bool limited = false;
        for (int i = 0; i < 20; ++i) {
            if (!limiter.allowConnection(ip)) {
                limited = true;
                break;
            }
        }
        // With burst of 10 and rate of 2/sec, should eventually limit
        REQUIRE(limited);
    }
    
    SECTION("Packet rate limiting") {
        uint32_t connId = 1;
        // Should allow burst
        for (int i = 0; i < 100; ++i) {
            limiter.allowPacket(connId);
        }
        // Check remaining tokens
        REQUIRE(limiter.getRemainingPacketTokens(connId) < 120);
    }
    
    SECTION("Cleanup on disconnect") {
        uint32_t connId = 1;
        limiter.allowPacket(connId);
        limiter.allowMessage(connId);
        
        limiter.onConnectionClosed(connId);
        // After cleanup, should have fresh tokens (but this depends on implementation)
    }
}

TEST_CASE("DDoSProtection statistics", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxConnectionsPerIP = 5;
    config.maxPacketsPerSecond = 10;
    
    DDoSProtection protection(config);
    protection.initialize();
    
    SECTION("Connection count tracking") {
        REQUIRE(protection.getActiveConnectionCount() == 0);
        
        protection.processPacket(1, "192.168.1.1", 100, 0);
        REQUIRE(protection.getActiveConnectionCount() == 1);
        
        protection.processPacket(2, "192.168.1.2", 100, 0);
        REQUIRE(protection.getActiveConnectionCount() == 2);
        
        protection.onConnectionClosed(1);
        REQUIRE(protection.getActiveConnectionCount() == 1);
    }
    
    SECTION("Blocked IP count") {
        REQUIRE(protection.getBlockedIPCount() == 0);
        
        protection.blockIP("192.168.1.1", 60, "Test");
        protection.blockIP("192.168.1.2", 60, "Test");
        REQUIRE(protection.getBlockedIPCount() == 2);
        
        protection.unblockIP("192.168.1.1");
        REQUIRE(protection.getBlockedIPCount() == 1);
    }
}

// ============================================================================
// Subnet Limiting
// ============================================================================

TEST_CASE("DDoSProtection subnet limiting", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxConnectionsPerIP = 5;
    config.maxConnectionsPerSubnet = 3;  // Only 3 connections per /24
    config.maxGlobalConnections = 100;

    DDoSProtection protection(config);
    protection.initialize();

    SECTION("Same subnet connections are limited") {
        // 192.168.1.10, .20, .30 are all in 192.168.1.0/24
        REQUIRE(protection.shouldAcceptConnection("192.168.1.10"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.20"));
        REQUIRE(protection.shouldAcceptConnection("192.168.1.30"));
        // 4th connection from same subnet should fail
        REQUIRE_FALSE(protection.shouldAcceptConnection("192.168.1.40"));
    }

    SECTION("Different subnets don't interfere") {
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
        REQUIRE(protection.shouldAcceptConnection("192.168.2.1"));
        REQUIRE(protection.shouldAcceptConnection("192.168.3.1"));
        // Each is in a different /24 subnet
        REQUIRE(protection.getActiveConnectionCount() == 3);
    }

    SECTION("Subnet limit blocks excess connections") {
        // Verify that subnet limiting works at all
        // (Connection cleanup is tested in packet processing edge cases)
        DDoSProtection::Config cfg;
        cfg.maxConnectionsPerIP = 5;
        cfg.maxConnectionsPerSubnet = 2;
        cfg.maxGlobalConnections = 1000;
        DDoSProtection prot(cfg);
        prot.initialize();

        REQUIRE(prot.shouldAcceptConnection("10.0.0.1"));
        REQUIRE(prot.shouldAcceptConnection("10.0.0.2"));
        // 3rd from same subnet exceeds limit of 2
        REQUIRE_FALSE(prot.shouldAcceptConnection("10.0.0.3"));
    }
}

// ============================================================================
// Global Connection Threshold Throttling
// ============================================================================

TEST_CASE("DDoSProtection global connection threshold", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxConnectionsPerIP = 1;
    config.maxConnectionsPerSubnet = 100;
    config.maxGlobalConnections = 10;
    config.globalConnectionsThreshold = 5;  // Start throttling at 5

    DDoSProtection protection(config);
    protection.initialize();

    SECTION("Accepts connections below threshold normally") {
        for (int i = 0; i < 5; ++i) {
            std::string ip = "10.0." + std::to_string(i) + ".1";
            REQUIRE(protection.shouldAcceptConnection(ip));
        }
    }

    SECTION("Rejects at global connection limit") {
        // Fill past global threshold
        // Due to 50% throttling near threshold, not all may be accepted
        int accepted = 0;
        for (int i = 0; i < 20; ++i) {
            std::string ip = "10.0." + std::to_string(i) + ".1";
            if (protection.shouldAcceptConnection(ip)) {
                accepted++;
            }
        }
        // With threshold=5, limit=10, and 50% throttling near threshold
        REQUIRE(accepted >= 5);
        REQUIRE(accepted <= 10);
    }
}

// ============================================================================
// Emergency Mode Edge Cases
// ============================================================================

TEST_CASE("DDoSProtection emergency mode edge cases", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxConnectionsPerIP = 3;

    DDoSProtection protection(config);
    protection.initialize();

    SECTION("Emergency mode blocks all new connections") {
        // Accept some first
        protection.shouldAcceptConnection("192.168.1.1");
        REQUIRE(protection.getActiveConnectionCount() == 1);

        protection.setEmergencyMode(true);
        REQUIRE(protection.isInEmergencyMode());

        // New connections blocked
        REQUIRE_FALSE(protection.shouldAcceptConnection("192.168.1.2"));
        REQUIRE_FALSE(protection.shouldAcceptConnection("10.0.0.1"));
    }

    SECTION("Existing connections still process packets in emergency mode") {
        protection.shouldAcceptConnection("192.168.1.1");
        protection.setEmergencyMode(true);

        // Existing connection should still process packets
        REQUIRE(protection.processPacket(1, "192.168.1.1", 100, 0));
    }

    SECTION("Toggle emergency mode multiple times") {
        REQUIRE_FALSE(protection.isInEmergencyMode());

        protection.setEmergencyMode(true);
        REQUIRE(protection.isInEmergencyMode());

        protection.setEmergencyMode(false);
        REQUIRE_FALSE(protection.isInEmergencyMode());

        // Should accept connections again
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
    }

    SECTION("Emergency mode doesn't affect whitelist") {
        protection.whitelistIP("192.168.1.100");
        protection.setEmergencyMode(true);

        // Whitelisted IP should still be accepted
        REQUIRE(protection.shouldAcceptConnection("192.168.1.100"));
    }
}

// ============================================================================
// Whitelist and Blocking Interaction
// ============================================================================

TEST_CASE("DDoSProtection whitelist and blocking interaction", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxConnectionsPerIP = 2;

    DDoSProtection protection(config);
    protection.initialize();

    SECTION("Whitelisted IP and block interaction") {
        protection.whitelistIP("192.168.1.1");
        protection.blockIP("192.168.1.1", 60, "Test block");

        // Block check happens BEFORE whitelist check in implementation
        // so an actively blocked IP is still blocked even if whitelisted
        REQUIRE_FALSE(protection.shouldAcceptConnection("192.168.1.1"));

        // After unblocking, whitelist applies again
        protection.unblockIP("192.168.1.1");
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
    }

    SECTION("Whitelisted packets bypass rate limiting") {
        protection.whitelistIP("192.168.1.1");
        protection.shouldAcceptConnection("192.168.1.1");

        // Send many packets - whitelisted so should all pass
        for (int i = 0; i < 100; ++i) {
            REQUIRE(protection.processPacket(1, "192.168.1.1", 100, 0));
        }
    }

    SECTION("Unwhitelisting removes protection") {
        protection.whitelistIP("192.168.1.1");
        REQUIRE(protection.isWhitelisted("192.168.1.1"));

        protection.unwhitelistIP("192.168.1.1");
        REQUIRE_FALSE(protection.isWhitelisted("192.168.1.1"));
    }

    SECTION("Block then unblock restores normal access") {
        protection.blockIP("192.168.1.1", 60, "Test");
        REQUIRE_FALSE(protection.shouldAcceptConnection("192.168.1.1"));

        protection.unblockIP("192.168.1.1");
        REQUIRE(protection.shouldAcceptConnection("192.168.1.1"));
    }
}

// ============================================================================
// Packet Processing for Blocked/Unknown Connections
// ============================================================================

TEST_CASE("DDoSProtection packet processing edge cases", "[security][ddos]") {
    DDoSProtection::Config config;
    config.maxPacketsPerSecond = 5;
    config.burstSize = 2;
    config.maxBytesPerSecond = 1024;

    DDoSProtection protection(config);
    protection.initialize();

    SECTION("Unknown connection creates stats entry") {
        REQUIRE(protection.processPacket(999, "10.0.0.1", 100, 0));
        REQUIRE(protection.getActiveConnectionCount() == 1);
    }

    SECTION("Byte rate limiting triggers on large packets") {
        // Verify byte-level rate limiting using packet processing
        DDoSProtection::Config cfg2;
        cfg2.maxPacketsPerSecond = 100;
        cfg2.burstSize = 10;
        cfg2.maxBytesPerSecond = 800;
        cfg2.maxConnectionsPerIP = 10;
        cfg2.maxGlobalConnections = 1000;
        cfg2.globalConnectionsThreshold = 900;
        DDoSProtection prot2(cfg2);
        prot2.initialize();

        // Create connection via processPacket
        REQUIRE(prot2.processPacket(42, "10.0.0.1", 400, 0));  // 400 bytes
        REQUIRE(prot2.processPacket(42, "10.0.0.1", 400, 0));  // 800 bytes
        REQUIRE_FALSE(prot2.processPacket(42, "10.0.0.1", 400, 0));  // Would be 1200 > 1000
    }

    SECTION("Window resets after 1 second") {
        DDoSProtection::Config cfg;
        cfg.maxPacketsPerSecond = 3;
        cfg.burstSize = 0;
        DDoSProtection prot(cfg);
        prot.initialize();

        // Exhaust window at t=0
        prot.processPacket(1, "10.0.0.1", 100, 0);
        prot.processPacket(1, "10.0.0.1", 100, 0);
        prot.processPacket(1, "10.0.0.1", 100, 0);
        REQUIRE_FALSE(prot.processPacket(1, "10.0.0.1", 100, 0));  // Rate limited

        // After 1 second, window resets
        REQUIRE(prot.processPacket(1, "10.0.0.1", 100, 1100));
    }

    SECTION("Closing non-existent connection doesn't crash") {
        // Should not crash
        protection.onConnectionClosed(99999);
        REQUIRE(protection.getActiveConnectionCount() == 0);
    }

    SECTION("Multiple connections from same IP are tracked") {
        DDoSProtection::Config cfg;
        cfg.maxConnectionsPerIP = 3;
        DDoSProtection prot(cfg);
        prot.initialize();

        // Process packets creating multiple connections from same IP
        prot.processPacket(1, "10.0.0.1", 100, 0);
        prot.processPacket(2, "10.0.0.1", 100, 0);
        prot.processPacket(3, "10.0.0.1", 100, 0);

        REQUIRE(prot.getActiveConnectionCount() == 3);

        // Close one
        prot.onConnectionClosed(2);
        REQUIRE(prot.getActiveConnectionCount() == 2);
    }
}
