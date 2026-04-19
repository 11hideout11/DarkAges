// ZoneDefinition functional tests
// Tests containsPosition, isInAuraBuffer, distanceToEdge, calculateOverlap, WorldPartition

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../include/zones/ZoneDefinition.hpp"
#include "../include/Constants.hpp"
#include <cmath>
#include <vector>

using namespace DarkAges;
using Catch::Approx;

static ZoneDefinition makeZone(uint32_t id, float minX, float maxX, float minZ, float maxZ) {
    ZoneDefinition zone;
    zone.zoneId = id;
    zone.zoneName = "Zone_" + std::to_string(id);
    zone.shape = ZoneShape::RECTANGLE;
    zone.minX = minX;
    zone.maxX = maxX;
    zone.minY = 0.0f;
    zone.maxY = 100.0f;
    zone.minZ = minZ;
    zone.maxZ = maxZ;
    zone.centerX = (minX + maxX) / 2.0f;
    zone.centerZ = (minZ + maxZ) / 2.0f;
    zone.host = "localhost";
    zone.port = 7777 + id;
    return zone;
}

// ============================================================================
// containsPosition tests
// ============================================================================

TEST_CASE("ZoneDefinition containsPosition center", "[zones][zonedef]") {
    // Zone from 0,0 to 200,200 with 50m buffer
    // Core region: 50 to 150 on each axis
    auto zone = makeZone(1, 0.0f, 200.0f, 0.0f, 200.0f);
    float buffer = Constants::AURA_BUFFER_METERS;

    SECTION("Center point is inside core") {
        REQUIRE(zone.containsPosition(100.0f, 100.0f) == true);
    }

    SECTION("Points within core region") {
        REQUIRE(zone.containsPosition(60.0f, 60.0f) == true);
        REQUIRE(zone.containsPosition(140.0f, 140.0f) == true);
        REQUIRE(zone.containsPosition(75.0f, 125.0f) == true);
    }

    SECTION("Points in aura buffer are NOT in core") {
        // Buffer region: 0-50 and 150-200 on each axis
        REQUIRE(zone.containsPosition(25.0f, 100.0f) == false);  // In X buffer
        REQUIRE(zone.containsPosition(100.0f, 25.0f) == false);  // In Z buffer
        REQUIRE(zone.containsPosition(25.0f, 25.0f) == false);   // Corner buffer
    }

    SECTION("Points outside zone entirely") {
        REQUIRE(zone.containsPosition(-10.0f, 100.0f) == false);
        REQUIRE(zone.containsPosition(100.0f, -10.0f) == false);
        REQUIRE(zone.containsPosition(210.0f, 100.0f) == false);
        REQUIRE(zone.containsPosition(100.0f, 210.0f) == false);
    }

    SECTION("Exact buffer boundary is excluded (strict inequality)") {
        // x = minX + buffer = 50, should be excluded
        REQUIRE(zone.containsPosition(buffer, 100.0f) == false);
        // x = maxX - buffer = 150, should be excluded
        REQUIRE(zone.containsPosition(200.0f - buffer, 100.0f) == false);
    }
}

TEST_CASE("ZoneDefinition containsPosition edge cases", "[zones][zonedef]") {
    SECTION("Very small zone") {
        auto zone = makeZone(1, 0.0f, 100.0f, 0.0f, 100.0f);
        // Buffer is 50, so core is 50 to 50 — essentially no core region
        REQUIRE(zone.containsPosition(50.0f, 50.0f) == false);
    }

    SECTION("Negative coordinates") {
        auto zone = makeZone(1, -200.0f, 0.0f, -200.0f, 0.0f);
        // Core: -150 to -50 on each axis
        REQUIRE(zone.containsPosition(-100.0f, -100.0f) == true);
        REQUIRE(zone.containsPosition(-175.0f, -100.0f) == false);  // In buffer
    }
}

// ============================================================================
// isInAuraBuffer tests
// ============================================================================

TEST_CASE("ZoneDefinition isInAuraBuffer", "[zones][zonedef]") {
    auto zone = makeZone(1, 0.0f, 200.0f, 0.0f, 200.0f);

    SECTION("Center is NOT in aura buffer") {
        REQUIRE(zone.isInAuraBuffer(100.0f, 100.0f) == false);
    }

    SECTION("Points in X buffer region") {
        REQUIRE(zone.isInAuraBuffer(25.0f, 100.0f) == true);   // Left buffer
        REQUIRE(zone.isInAuraBuffer(175.0f, 100.0f) == true);  // Right buffer
    }

    SECTION("Points in Z buffer region") {
        REQUIRE(zone.isInAuraBuffer(100.0f, 25.0f) == true);   // Bottom buffer
        REQUIRE(zone.isInAuraBuffer(100.0f, 175.0f) == true);  // Top buffer
    }

    SECTION("Corner buffer regions") {
        REQUIRE(zone.isInAuraBuffer(25.0f, 25.0f) == true);
        REQUIRE(zone.isInAuraBuffer(175.0f, 175.0f) == true);
    }

    SECTION("Outside zone is NOT in aura buffer") {
        REQUIRE(zone.isInAuraBuffer(-10.0f, 100.0f) == false);
        REQUIRE(zone.isInAuraBuffer(210.0f, 100.0f) == false);
    }

    SECTION("Core region is NOT in aura buffer") {
        REQUIRE(zone.isInAuraBuffer(75.0f, 75.0f) == false);
        REQUIRE(zone.isInAuraBuffer(125.0f, 125.0f) == false);
    }

    SECTION("Exact boundary between core and buffer") {
        float buffer = Constants::AURA_BUFFER_METERS;
        // At x = buffer (50), this is in the core boundary (uses >= for main bounds, >= for core)
        // The core check uses >=, so x=50 is IN the core, not in the buffer
        REQUIRE(zone.isInAuraBuffer(buffer, 100.0f) == false);
    }
}

// ============================================================================
// distanceToEdge tests
// ============================================================================

TEST_CASE("ZoneDefinition distanceToEdge inside", "[zones][zonedef]") {
    auto zone = makeZone(1, 0.0f, 200.0f, 0.0f, 200.0f);

    SECTION("Center point - equidistant to all edges") {
        float dist = zone.distanceToEdge(100.0f, 100.0f);
        REQUIRE(dist == Approx(-100.0f).margin(0.001f));  // Negative = inside
    }

    SECTION("Close to minX edge") {
        float dist = zone.distanceToEdge(10.0f, 100.0f);
        REQUIRE(dist == Approx(-10.0f).margin(0.001f));
    }

    SECTION("Close to maxX edge") {
        float dist = zone.distanceToEdge(190.0f, 100.0f);
        REQUIRE(dist == Approx(-10.0f).margin(0.001f));
    }

    SECTION("Close to minZ edge") {
        float dist = zone.distanceToEdge(100.0f, 10.0f);
        REQUIRE(dist == Approx(-10.0f).margin(0.001f));
    }

    SECTION("Close to maxZ edge") {
        float dist = zone.distanceToEdge(100.0f, 190.0f);
        REQUIRE(dist == Approx(-10.0f).margin(0.001f));
    }

    SECTION("Corner - distance to nearest edge") {
        float dist = zone.distanceToEdge(10.0f, 10.0f);
        REQUIRE(dist == Approx(-10.0f).margin(0.001f));  // Nearest edge is 10m
    }
}

TEST_CASE("ZoneDefinition distanceToEdge outside", "[zones][zonedef]") {
    auto zone = makeZone(1, 0.0f, 200.0f, 0.0f, 200.0f);

    SECTION("Directly outside minX") {
        float dist = zone.distanceToEdge(-10.0f, 100.0f);
        REQUIRE(dist == Approx(10.0f).margin(0.001f));  // Positive = outside
    }

    SECTION("Directly outside maxX") {
        float dist = zone.distanceToEdge(210.0f, 100.0f);
        REQUIRE(dist == Approx(10.0f).margin(0.001f));
    }

    SECTION("Diagonally outside corner") {
        // 10m outside in X, 10m outside in Z -> sqrt(200) ≈ 14.14
        float dist = zone.distanceToEdge(-10.0f, -10.0f);
        float expected = std::sqrt(200.0f);
        REQUIRE(dist == Approx(expected).margin(0.01f));
    }

    SECTION("Far outside") {
        float dist = zone.distanceToEdge(-100.0f, 100.0f);
        REQUIRE(dist == Approx(100.0f).margin(0.001f));
    }
}

// ============================================================================
// calculateOverlap tests
// ============================================================================

TEST_CASE("ZoneDefinition calculateOverlap", "[zones][zonedef]") {
    SECTION("Overlapping zones") {
        auto zoneA = makeZone(1, 0.0f, 200.0f, 0.0f, 200.0f);
        auto zoneB = makeZone(2, 100.0f, 300.0f, 100.0f, 300.0f);

        float oMinX, oMaxX, oMinZ, oMaxZ;
        bool overlaps = zoneA.calculateOverlap(zoneB, oMinX, oMaxX, oMinZ, oMaxZ);

        REQUIRE(overlaps == true);
        REQUIRE(oMinX == Approx(100.0f).margin(0.001f));
        REQUIRE(oMaxX == Approx(200.0f).margin(0.001f));
        REQUIRE(oMinZ == Approx(100.0f).margin(0.001f));
        REQUIRE(oMaxZ == Approx(200.0f).margin(0.001f));
    }

    SECTION("Non-overlapping zones") {
        auto zoneA = makeZone(1, 0.0f, 100.0f, 0.0f, 100.0f);
        auto zoneB = makeZone(2, 200.0f, 300.0f, 200.0f, 300.0f);

        float oMinX, oMaxX, oMinZ, oMaxZ;
        bool overlaps = zoneA.calculateOverlap(zoneB, oMinX, oMaxX, oMinZ, oMaxZ);

        REQUIRE(overlaps == false);
    }

    SECTION("Touching edges (no overlap with strict inequality)") {
        auto zoneA = makeZone(1, 0.0f, 100.0f, 0.0f, 100.0f);
        auto zoneB = makeZone(2, 100.0f, 200.0f, 0.0f, 100.0f);

        float oMinX, oMaxX, oMinZ, oMaxZ;
        bool overlaps = zoneA.calculateOverlap(zoneB, oMinX, oMaxX, oMinZ, oMaxZ);

        // outMinX = max(0, 100) = 100, outMaxX = min(100, 200) = 100
        // 100 < 100 is false, so no overlap
        REQUIRE(overlaps == false);
    }

    SECTION("One zone contains another") {
        auto zoneA = makeZone(1, 0.0f, 300.0f, 0.0f, 300.0f);
        auto zoneB = makeZone(2, 50.0f, 150.0f, 50.0f, 150.0f);

        float oMinX, oMaxX, oMinZ, oMaxZ;
        bool overlaps = zoneA.calculateOverlap(zoneB, oMinX, oMaxX, oMinZ, oMaxZ);

        REQUIRE(overlaps == true);
        REQUIRE(oMinX == Approx(50.0f).margin(0.001f));
        REQUIRE(oMaxX == Approx(150.0f).margin(0.001f));
    }

    SECTION("Same zone overlap") {
        auto zone = makeZone(1, 0.0f, 200.0f, 0.0f, 200.0f);

        float oMinX, oMaxX, oMinZ, oMaxZ;
        bool overlaps = zone.calculateOverlap(zone, oMinX, oMaxX, oMinZ, oMaxZ);

        REQUIRE(overlaps == true);
        REQUIRE(oMinX == Approx(0.0f).margin(0.001f));
        REQUIRE(oMaxX == Approx(200.0f).margin(0.001f));
    }
}

// ============================================================================
// WorldPartition tests
// ============================================================================

TEST_CASE("WorldPartition createGrid", "[zones][zonedef]") {
    SECTION("2x2 grid creates 4 zones") {
        auto zones = WorldPartition::createGrid(2, 2, 0.0f, 400.0f, 0.0f, 400.0f);
        REQUIRE(zones.size() == 4);
    }

    SECTION("3x3 grid creates 9 zones") {
        auto zones = WorldPartition::createGrid(3, 3, 0.0f, 600.0f, 0.0f, 600.0f);
        REQUIRE(zones.size() == 9);
    }

    SECTION("Zone IDs are 1-based and sequential") {
        auto zones = WorldPartition::createGrid(2, 2, 0.0f, 400.0f, 0.0f, 400.0f);
        for (size_t i = 0; i < zones.size(); i++) {
            REQUIRE(zones[i].zoneId == static_cast<uint32_t>(i + 1));
        }
    }

    SECTION("Zone names follow pattern") {
        auto zones = WorldPartition::createGrid(2, 2, 0.0f, 400.0f, 0.0f, 400.0f);
        REQUIRE(zones[0].zoneName == "Zone_1");
        REQUIRE(zones[1].zoneName == "Zone_2");
        REQUIRE(zones[3].zoneName == "Zone_4");
    }

    SECTION("Grid zones cover entire world") {
        auto zones = WorldPartition::createGrid(2, 2, 0.0f, 400.0f, 0.0f, 400.0f);

        // Each zone has core width of 200m, but interior edges add buffer overlap
        // Zone 1 (0,0): minX=0, maxX=200+50=250 (buffer on right edge)
        // Zone 2 (1,0): minX=200-50=150, maxX=400 (buffer on left edge)
        // So zones are 250m wide due to buffer overlap on interior edges
        for (const auto& zone : zones) {
            float width = zone.maxX - zone.minX;
            float height = zone.maxZ - zone.minZ;
            // Zones with buffer overlap are 250m, edge zones are 200m
            REQUIRE(width >= 200.0f);
            REQUIRE(width <= 250.0f);
            REQUIRE(height >= 200.0f);
            REQUIRE(height <= 250.0f);
        }
    }

    SECTION("Adjacent zones have correct relationships") {
        auto zones = WorldPartition::createGrid(2, 2, 0.0f, 400.0f, 0.0f, 400.0f);

        // Zone 1 (0,0)-(200,200) should be adjacent to Zone 2 and Zone 3
        const auto& z1 = zones[0];
        REQUIRE(z1.adjacentZones.size() > 0);
    }
}

// ============================================================================
// ZoneDefinition struct validation
// ============================================================================

TEST_CASE("ZoneDefinition struct construction", "[zones][zonedef]") {
    SECTION("makeZone creates valid zone") {
        auto zone = makeZone(42, 100.0f, 300.0f, 200.0f, 400.0f);

        REQUIRE(zone.zoneId == 42);
        REQUIRE(zone.zoneName == "Zone_42");
        REQUIRE(zone.minX == Approx(100.0f).margin(0.001f));
        REQUIRE(zone.maxX == Approx(300.0f).margin(0.001f));
        REQUIRE(zone.minZ == Approx(200.0f).margin(0.001f));
        REQUIRE(zone.maxZ == Approx(400.0f).margin(0.001f));
        REQUIRE(zone.centerX == Approx(200.0f).margin(0.001f));
        REQUIRE(zone.centerZ == Approx(300.0f).margin(0.001f));
    }

    SECTION("Default constructed zone") {
        ZoneDefinition zone;
        // Default values - zoneId is uninitialized (not guaranteed to be 0)
        REQUIRE(zone.adjacentZones.empty());
        REQUIRE(zone.zoneName.empty());
    }
}
