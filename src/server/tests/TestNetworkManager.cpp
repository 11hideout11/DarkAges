// Auto-generated tests for NetworkManager
#include <catch2/catch_test_macros.hpp>
#include "../include/netcode/NetworkManager.hpp"

using namespace DarkAges;

namespace darkages {
namespace test {

    TEST_CASE("NetworkManager - header compiles", "[networkmanager]") {
        REQUIRE(true);
    }

    TEST_CASE("NetworkManager - PacketType enum defined", "[networkmanager]") {
        REQUIRE(sizeof(PacketType) > 0);
    }

    TEST_CASE("NetworkManager - ConnectionQuality is complete", "[networkmanager]") {
        REQUIRE(sizeof(ConnectionQuality) > 0);
    }

    TEST_CASE("NetworkManager - ClientInputPacket is complete", "[networkmanager]") {
        REQUIRE(sizeof(ClientInputPacket) > 0);
    }

    TEST_CASE("NetworkManager - SnapshotPacket is complete", "[networkmanager]") {
        REQUIRE(sizeof(SnapshotPacket) > 0);
    }

} // namespace test
} // namespace darkages
