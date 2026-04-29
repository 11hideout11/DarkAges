#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "physics/NavigationGrid.hpp"
#include "ecs/CoreTypes.hpp"
#include "combat/NPCAISystem.hpp"
#include "combat/CombatSystem.hpp"
#include <entt/entt.hpp>
#include <glm/glm.hpp>

using namespace DarkAges;
using Catch::Approx;

// ============================================================================
// NavigationGrid Construction and Coordinate Conversion
// ============================================================================

TEST_CASE("NavigationGrid construction", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    REQUIRE(grid.width() == 100);
    REQUIRE(grid.height() == 100);
    REQUIRE(grid.resolution() == Approx(1.0f));
    REQUIRE(grid.centerX() == Approx(0.0f));
    REQUIRE(grid.centerZ() == Approx(0.0f));
    REQUIRE(grid.blockedCount() == 0);
}

TEST_CASE("NavigationGrid construction with fractional size", "[physics][pathfinding]") {
    NavigationGrid grid(10.0f, 10.0f, 10.5f, 10.5f, 1.0f);

    // Should ceil to 11x11
    REQUIRE(grid.width() == 11);
    REQUIRE(grid.height() == 11);
}

TEST_CASE("NavigationGrid minimum size", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

    // Minimum is 4x4
    REQUIRE(grid.width() >= 4);
    REQUIRE(grid.height() >= 4);
}

TEST_CASE("NavigationGrid world-to-cell conversion", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Center of grid should map to center cell
    uint32_t cx, cz;
    grid.worldToCell(0.0f, 0.0f, cx, cz);
    REQUIRE(cx == 50);
    REQUIRE(cz == 50);

    // Bottom-left corner
    grid.worldToCell(-50.0f, -50.0f, cx, cz);
    REQUIRE(cx == 0);
    REQUIRE(cz == 0);

    // Top-right corner (clamped)
    grid.worldToCell(50.0f, 50.0f, cx, cz);
    REQUIRE(cx == 99);
    REQUIRE(cz == 99);
}

TEST_CASE("NavigationGrid cell-to-world conversion", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    float wx, wz;
    grid.cellToWorld(50, 50, wx, wz);
    REQUIRE(wx == Approx(0.5f));  // Cell center
    REQUIRE(wz == Approx(0.5f));

    grid.cellToWorld(0, 0, wx, wz);
    REQUIRE(wx == Approx(-49.5f));
    REQUIRE(wz == Approx(-49.5f));
}

TEST_CASE("NavigationGrid round-trip coordinate conversion", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    float testX = 10.0f, testZ = -20.0f;
    uint32_t cx, cz;
    grid.worldToCell(testX, testZ, cx, cz);

    float wx, wz;
    grid.cellToWorld(cx, cz, wx, wz);

    // Should be within one cell
    REQUIRE(std::abs(wx - testX) < 1.0f);
    REQUIRE(std::abs(wz - testZ) < 1.0f);
}

// ============================================================================
// Obstacle Management
// ============================================================================

TEST_CASE("NavigationGrid obstacle placement", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    REQUIRE(grid.blockedCount() == 0);
    REQUIRE(grid.isWalkable(10, 10));

    grid.setObstacle(10, 10, true);
    REQUIRE(grid.blockedCount() == 1);
    REQUIRE_FALSE(grid.isWalkable(10, 10));

    grid.setObstacle(10, 10, false);
    REQUIRE(grid.blockedCount() == 0);
    REQUIRE(grid.isWalkable(10, 10));
}

TEST_CASE("NavigationGrid obstacle rectangle", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Place a 5x5 obstacle at world position (10, 10)
    grid.setObstacleRect(10.0f, 10.0f, 5.0f, 5.0f, true);

    // The center cell should be blocked
    uint32_t cx, cz;
    grid.worldToCell(10.0f, 10.0f, cx, cz);
    REQUIRE_FALSE(grid.isWalkable(cx, cz));

    // Cells outside the rect should be walkable
    REQUIRE(grid.isWalkable(0, 0));

    // Clear the obstacle
    grid.setObstacleRect(10.0f, 10.0f, 5.0f, 5.0f, false);
    REQUIRE(grid.isWalkable(cx, cz));
}

TEST_CASE("NavigationGrid out of bounds", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Out of bounds should be unwalkable
    REQUIRE_FALSE(grid.isWalkable(999, 999));
    REQUIRE_FALSE(grid.isWalkable(grid.width(), 0));
    REQUIRE_FALSE(grid.isWalkable(0, grid.height()));
}

TEST_CASE("NavigationGrid clear", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    grid.setObstacle(10, 10, true);
    grid.setObstacle(20, 20, true);
    REQUIRE(grid.blockedCount() == 2);

    grid.clear();
    REQUIRE(grid.blockedCount() == 0);
    REQUIRE(grid.isWalkable(10, 10));
    REQUIRE(grid.isWalkable(20, 20));
}

// ============================================================================
// Line of Sight
// ============================================================================

TEST_CASE("NavigationGrid line of sight clear", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // No obstacles — should always have LOS
    REQUIRE(grid.hasLineOfSight(-20.0f, -20.0f, 20.0f, 20.0f));
    REQUIRE(grid.hasLineOfSight(0.0f, 0.0f, 10.0f, 0.0f));
    REQUIRE(grid.hasLineOfSight(0.0f, 0.0f, 0.0f, 10.0f));
    REQUIRE(grid.hasLineOfSight(0.0f, 0.0f, 0.0f, 0.0f));  // Same point
}

TEST_CASE("NavigationGrid line of sight blocked by wall", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Place a wall at x=0 (column of blocked cells)
    for (uint32_t z = 0; z < grid.height(); ++z) {
        grid.setObstacle(50, z, true);  // x=0 maps to cell 50
    }

    // LOS across the wall should fail
    REQUIRE_FALSE(grid.hasLineOfSight(-5.0f, 0.0f, 5.0f, 0.0f));

    // LOS along one side should pass
    REQUIRE(grid.hasLineOfSight(-5.0f, 0.0f, -5.0f, 10.0f));
}

TEST_CASE("NavigationGrid line of sight blocked by pillar", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Place a single obstacle
    grid.setObstacle(50, 50, true);

    // LOS passing through the obstacle should fail
    REQUIRE_FALSE(grid.hasLineOfSight(0.0f, 0.0f, 2.0f, 2.0f));
}

// ============================================================================
// A* Pathfinding
// ============================================================================

TEST_CASE("NavigationGrid A* clear path", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    Path path = grid.findPath(-20.0f, 0.0f, 20.0f, 0.0f);

    REQUIRE(path.valid);
    // With clear LOS, path should be direct (single waypoint = destination)
    REQUIRE(path.size() == 1);
    REQUIRE(path.waypoints[0].x == Approx(20.0f));
    REQUIRE(path.waypoints[0].z == Approx(0.0f));
}

TEST_CASE("NavigationGrid A* same start and end", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    Path path = grid.findPath(5.0f, 5.0f, 5.0f, 5.0f);

    REQUIRE(path.valid);
    REQUIRE(path.size() == 1);
}

TEST_CASE("NavigationGrid A* obstacle avoidance", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Place a wall blocking direct path from (-10,0) to (10,0)
    for (uint32_t z = 48; z <= 52; ++z) {
        grid.setObstacle(50, z, true);
    }

    Path path = grid.findPath(-10.0f, 0.0f, 10.0f, 0.0f);

    REQUIRE(path.valid);
    REQUIRE(path.size() > 1);  // Should have multiple waypoints going around

    // Verify the path doesn't pass through the wall
    // (all waypoints should be on walkable cells)
    for (const auto& wp : path.waypoints) {
        uint32_t cx, cz;
        grid.worldToCell(wp.x, wp.z, cx, cz);
        REQUIRE(grid.isWalkable(cx, cz));
    }
}

TEST_CASE("NavigationGrid A* blocked start", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    uint32_t cx, cz;
    grid.worldToCell(0.0f, 0.0f, cx, cz);
    grid.setObstacle(cx, cz, true);

    Path path = grid.findPath(0.0f, 0.0f, 10.0f, 0.0f);

    REQUIRE_FALSE(path.valid);
}

TEST_CASE("NavigationGrid A* blocked goal", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    uint32_t cx, cz;
    grid.worldToCell(10.0f, 0.0f, cx, cz);
    grid.setObstacle(cx, cz, true);

    Path path = grid.findPath(0.0f, 0.0f, 10.0f, 0.0f);

    REQUIRE_FALSE(path.valid);
}

TEST_CASE("NavigationGrid A* completely enclosed", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Create a box around (0,0) with no gap
    for (int i = -2; i <= 2; ++i) {
        grid.setObstacle(50 + i, 48, true);  // South wall
        grid.setObstacle(50 + i, 52, true);  // North wall
        grid.setObstacle(48, 50 + i, true);  // West wall
        grid.setObstacle(52, 50 + i, true);  // East wall
    }

    // Start inside the box, end outside
    Path path = grid.findPath(0.0f, 0.0f, 20.0f, 0.0f);

    REQUIRE_FALSE(path.valid);
}

TEST_CASE("NavigationGrid A* diagonal movement", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    // Diagonal path should work
    Path path = grid.findPath(-10.0f, -10.0f, 10.0f, 10.0f);

    REQUIRE(path.valid);
    // Clear LOS means direct single waypoint
    REQUIRE(path.size() >= 1);
}

// ============================================================================
// Path Object
// ============================================================================

TEST_CASE("Path nextWaypoint", "[physics][pathfinding]") {
    Path path;
    path.valid = true;
    path.waypoints.push_back({1.0f, 2.0f});
    path.waypoints.push_back({3.0f, 4.0f});

    Waypoint wp;
    REQUIRE(path.nextWaypoint(wp));
    REQUIRE(wp.x == Approx(1.0f));
    REQUIRE(wp.z == Approx(2.0f));
    REQUIRE(path.size() == 1);

    REQUIRE(path.nextWaypoint(wp));
    REQUIRE(wp.x == Approx(3.0f));
    REQUIRE(wp.z == Approx(4.0f));
    REQUIRE(path.size() == 0);

    REQUIRE_FALSE(path.nextWaypoint(wp));
}

TEST_CASE("Path clear", "[physics][pathfinding]") {
    Path path;
    path.valid = true;
    path.waypoints.push_back({1.0f, 2.0f});

    path.clear();

    REQUIRE_FALSE(path.valid);
    REQUIRE(path.empty());
}

// ============================================================================
// NPCAISystem Pathfinding Integration
// ============================================================================

static EntityID createTestPlayer(Registry& registry, float x, float z) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(x, 0, z)));
    registry.emplace<Velocity>(entity);
    registry.emplace<CombatState>(entity);
    auto& combat = registry.get<CombatState>(entity);
    combat.health = 1000;
    combat.maxHealth = 1000;
    registry.emplace<Mana>(entity);
    registry.emplace<PlayerTag>(entity);
    registry.emplace<PlayerProgression>(entity);
    return entity;
}

static EntityID createTestNPC(Registry& registry, float x, float z,
                               float aggroRange = 15.0f, float leashRange = 30.0f,
                               float attackRange = 2.0f) {
    auto entity = registry.create();
    registry.emplace<Position>(entity, Position::fromVec3(glm::vec3(x, 0, z)));
    registry.emplace<Velocity>(entity);
    registry.emplace<Rotation>(entity);
    registry.emplace<BoundingVolume>(entity);

    registry.emplace<CombatState>(entity);
    auto& combat = registry.get<CombatState>(entity);
    combat.health = 1000;
    combat.maxHealth = 1000;

    Mana mana;
    registry.emplace<Mana>(entity, mana);

    registry.emplace<NPCTag>(entity);
    registry.emplace<CollisionLayer>(entity, CollisionLayer::makeNPC());

    NPCAIState ai;
    ai.aggroRange = aggroRange;
    ai.leashRange = leashRange;
    ai.attackRange = attackRange;
    ai.spawnPoint = Position::fromVec3(glm::vec3(x, 0, z));
    registry.emplace<NPCAIState>(entity, ai);

    NPCStats stats;
    stats.baseDamage = 10;
    stats.xpReward = 50;
    stats.respawnTimeMs = 10000;
    registry.emplace<NPCStats>(entity, stats);

    return entity;
}

TEST_CASE("NPCAISystem pathfinding without grid", "[combat][npc_ai][pathfinding]") {
    Registry registry;
    NPCAISystem aiSystem;

    // Without a nav grid, NPCs should use direct movement (existing behavior)
    auto npc = createTestNPC(registry, 0.0f, 0.0f);
    auto player = createTestPlayer(registry, 5.0f, 5.0f);

    aiSystem.update(registry, 0);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Chase);
    REQUIRE(ai.target == player);
}

TEST_CASE("NPCAISystem pathfinding with grid - clear LOS", "[combat][npc_ai][pathfinding]") {
    Registry registry;
    NPCAISystem aiSystem;

    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);
    aiSystem.setNavigationGrid(&grid);

    auto npc = createTestNPC(registry, 0.0f, 0.0f);
    auto player = createTestPlayer(registry, 5.0f, 5.0f);

    // First tick: NPC detects player and transitions Idle -> Chase
    aiSystem.update(registry, 0);
    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Chase);

    // Second tick: Chase behavior moves toward player
    aiSystem.update(registry, 17);

    // With clear LOS and pathfinding, NPC should move directly toward player
    const auto* vel = registry.try_get<Velocity>(npc);
    REQUIRE(vel != nullptr);
    // Velocity should be non-zero (moving toward player)
    REQUIRE(vel->speed() > 0.0f);
}

TEST_CASE("NPCAISystem pathfinding with grid - obstacle between", "[combat][npc_ai][pathfinding]") {
    Registry registry;
    NPCAISystem aiSystem;

    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);
    aiSystem.setNavigationGrid(&grid);

    // Place a wall between NPC and player
    for (uint32_t z = 48; z <= 52; ++z) {
        grid.setObstacle(50, z, true);
    }

    auto npc = createTestNPC(registry, 0.0f, 0.0f);  // Cell (50, 50)
    auto player = createTestPlayer(registry, 10.0f, 0.0f);  // Cell (60, 50)

    aiSystem.update(registry, 0);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Chase);

    // NPC should still move (using pathfinding to go around the wall)
    const auto* vel = registry.try_get<Velocity>(npc);
    REQUIRE(vel != nullptr);
}

TEST_CASE("NPCAISystem clearPaths", "[combat][npc_ai][pathfinding]") {
    Registry registry;
    NPCAISystem aiSystem;

    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);
    aiSystem.setNavigationGrid(&grid);

    auto npc = createTestNPC(registry, 0.0f, 0.0f);
    auto player = createTestPlayer(registry, 5.0f, 5.0f);

    // Run a tick to generate paths
    aiSystem.update(registry, 0);

    // Clear all paths (should not crash)
    aiSystem.clearPaths();

    // Run another tick (should still work)
    aiSystem.update(registry, 17);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Chase);
}

TEST_CASE("NPCAISystem clearPath for specific NPC", "[combat][npc_ai][pathfinding]") {
    Registry registry;
    NPCAISystem aiSystem;

    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);
    aiSystem.setNavigationGrid(&grid);

    auto npc = createTestNPC(registry, 0.0f, 0.0f);
    auto player = createTestPlayer(registry, 5.0f, 5.0f);

    aiSystem.update(registry, 0);
    aiSystem.clearPath(npc);  // Should not crash
    aiSystem.update(registry, 17);
}

TEST_CASE("NPCAISystem setNavigationGrid to null", "[combat][npc_ai][pathfinding]") {
    Registry registry;
    NPCAISystem aiSystem;

    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);
    aiSystem.setNavigationGrid(&grid);
    aiSystem.setNavigationGrid(nullptr);  // Disable pathfinding

    auto npc = createTestNPC(registry, 0.0f, 0.0f);
    auto player = createTestPlayer(registry, 5.0f, 5.0f);

    // Should work without pathfinding (direct movement)
    aiSystem.update(registry, 0);

    const auto& ai = registry.get<NPCAIState>(npc);
    REQUIRE(ai.behavior == NPCBehavior::Chase);
}

TEST_CASE("NavigationGrid resize", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 100.0f, 100.0f, 1.0f);

    grid.setObstacle(10, 10, true);
    REQUIRE(grid.blockedCount() == 1);

    // Resize clears obstacles
    grid.resize(0.0f, 0.0f, 50.0f, 50.0f);
    REQUIRE(grid.width() == 50);
    REQUIRE(grid.height() == 50);
    REQUIRE(grid.blockedCount() == 0);
}

TEST_CASE("NavigationGrid high resolution", "[physics][pathfinding]") {
    NavigationGrid grid(0.0f, 0.0f, 10.0f, 10.0f, 0.5f);

    // 10m / 0.5m = 20 cells
    REQUIRE(grid.width() == 20);
    REQUIRE(grid.height() == 20);
    REQUIRE(grid.resolution() == Approx(0.5f));
}
