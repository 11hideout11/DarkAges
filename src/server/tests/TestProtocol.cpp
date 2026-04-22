// [NETWORK_AGENT] Comprehensive unit tests for Protocol and Network Protocol Layer

#include <catch2/catch_test_macros.hpp>
#include "../include/netcode/Protocol.hpp"
#include <unordered_set>
#include <type_traits>

using namespace DarkAges;

namespace {

// Helper: check that enum values are correctly scoped and integral
template<typename E>
constexpr auto to_underlying(E e) noexcept -> std::underlying_type_t<E> {
    return static_cast<std::underlying_type_t<E>>(e);
}

} // namespace

TEST_CASE("Protocol - MessageType enum is well-formed", "[protocol]") {
    // Basic type properties
    STATIC_REQUIRE(std::is_enum_v<Protocol::MessageType>);
    STATIC_REQUIRE(std::is_same_v<std::underlying_type_t<Protocol::MessageType>, uint8_t>);
    
    // Can take address
    Protocol::MessageType msg = Protocol::MessageType::Unknown;
    REQUIRE(sizeof(msg) == 1);
}

TEST_CASE("Protocol - MessageType values are as expected", "[protocol]") {
    REQUIRE(to_underlying(Protocol::MessageType::Unknown) == 0);
    REQUIRE(to_underlying(Protocol::MessageType::PlayerMove) == 1);
    REQUIRE(to_underlying(Protocol::MessageType::PlayerUpdate) == 2);
    REQUIRE(to_underlying(Protocol::MessageType::CombatHit) == 3);
    REQUIRE(to_underlying(Protocol::MessageType::ZoneTransfer) == 4);
}

TEST_CASE("Protocol - MessageType values are unique", "[protocol]") {
    // Create set of underlying values
    std::unordered_set<uint8_t> values;
    values.insert(to_underlying(Protocol::MessageType::Unknown));
    values.insert(to_underlying(Protocol::MessageType::PlayerMove));
    values.insert(to_underlying(Protocol::MessageType::PlayerUpdate));
    values.insert(to_underlying(Protocol::MessageType::CombatHit));
    values.insert(to_underlying(Protocol::MessageType::ZoneTransfer));
    
    REQUIRE(values.size() == 5);
}

TEST_CASE("Protocol - MessageType zero is Unknown", "[protocol]") {
    // Zero-initialized MessageType should equal Unknown (per enum default)
    Protocol::MessageType defaultMsg{};
    REQUIRE(defaultMsg == Protocol::MessageType::Unknown);
}

TEST_CASE("Protocol - MessageType can be used in switch statements", "[protocol]") {
    Protocol::MessageType msg = Protocol::MessageType::PlayerMove;
    bool handled = false;
    
    switch (msg) {
        case Protocol::MessageType::PlayerMove:
            handled = true;
            break;
        default:
            break;
    }
    
    REQUIRE(handled);
}

TEST_CASE("Protocol - MessageType string representation exists", "[protocol]") {
    // Not testing actual string output, just that we can reference types
    // This test primarily ensures the enum is available in header
    Protocol::MessageType msgs[] = {
        Protocol::MessageType::Unknown,
        Protocol::MessageType::PlayerMove,
        Protocol::MessageType::PlayerUpdate,
        Protocol::MessageType::CombatHit,
        Protocol::MessageType::ZoneTransfer
    };
    
    REQUIRE(std::size(msgs) == 5);
    
    // Verify we can read from array
    for (auto msg : msgs) {
        REQUIRE(to_underlying(msg) >= 0);
        REQUIRE(to_underlying(msg) <= 4);
    }
}

TEST_CASE("Protocol - MessageType supports comparison operators", "[protocol]") {
    Protocol::MessageType a = Protocol::MessageType::PlayerMove;
    Protocol::MessageType b = Protocol::MessageType::PlayerMove;
    Protocol::MessageType c = Protocol::MessageType::CombatHit;

    REQUIRE(a == b);
    REQUIRE_FALSE(a == c);
    REQUIRE(a != c);
    REQUIRE(a < c);  // enum ordering by underlying value
    REQUIRE(c > a);
}

TEST_CASE("Protocol - MessageType range is uint8_t compatible", "[protocol]") {
    // Ensure all defined values fit in uint8_t (0-255)
    for (uint8_t i = 0; i <= 4; ++i) {
        Protocol::MessageType msg = static_cast<Protocol::MessageType>(i);
        REQUIRE(to_underlying(msg) == i);
    }
    
    // Check unknown values still construct
    Protocol::MessageType unknown = static_cast<Protocol::MessageType>(255);
    REQUIRE(to_underlying(unknown) == 255);
}

TEST_CASE("Protocol - compile-time constness of MessageType", "[protocol]") {
    // Enum values are constexpr
    constexpr uint8_t v1 = to_underlying(Protocol::MessageType::PlayerMove);
    STATIC_REQUIRE(v1 == 1);
    
    constexpr auto v2 = Protocol::MessageType::CombatHit;
    STATIC_REQUIRE(to_underlying(v2) == 3);
}

TEST_CASE("Protocol - MessageType fits in network packet type field", "[protocol]") {
    // PacketType in NetworkManager.hpp is uint8_t, MessageType is uint8_t — compatible
    Protocol::MessageType msg = Protocol::MessageType::ZoneTransfer;
    uint8_t raw = static_cast<uint8_t>(msg);
    REQUIRE(raw == 4);
    
    // Round-trip
    Protocol::MessageType roundtrip = static_cast<Protocol::MessageType>(raw);
    REQUIRE(roundtrip == msg);
}

TEST_CASE("Protocol - enum class prevents implicit int conversion", "[protocol]") {
    // Should require explicit cast
    // Uncommenting next line would fail compilation:
    // int x = Protocol::MessageType::PlayerMove; // ERROR
    // So we verify via cast
    int x = static_cast<int>(Protocol::MessageType::PlayerMove);
    REQUIRE(x == 1);
}

TEST_CASE("Protocol - MessageType is POD-like", "[protocol]") {
    STATIC_REQUIRE(std::is_trivial_v<Protocol::MessageType>);
    STATIC_REQUIRE(std::is_standard_layout_v<Protocol::MessageType>);
}
