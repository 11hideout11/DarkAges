#pragma once

#include "ecs/CoreTypes.hpp"
#include <vector>
#include <cstdint>
#include <functional>

// NavigationGrid — 2D grid-based A* pathfinding for NPC movement
// Provides obstacle avoidance, line-of-sight checks, and path caching
// Each zone can have its own NavigationGrid covering the local area

namespace DarkAges {

// A single waypoint in a path
struct Waypoint {
    float x{0.0f};
    float z{0.0f};
};

// A computed path (list of waypoints)
struct Path {
    std::vector<Waypoint> waypoints;
    uint32_t targetCellX{0};
    uint32_t targetCellZ{0};
    bool valid{false};

    bool empty() const { return waypoints.empty(); }
    size_t size() const { return waypoints.size(); }

    // Get next waypoint and advance the path
    bool nextWaypoint(Waypoint& out) {
        if (waypoints.empty()) return false;
        out = waypoints.front();
        waypoints.erase(waypoints.begin());
        return true;
    }

    void clear() {
        waypoints.clear();
        valid = false;
        targetCellX = 0;
        targetCellZ = 0;
    }
};

// 2D navigation grid with A* pathfinding
class NavigationGrid {
public:
    // Default constructor - creates empty grid
    NavigationGrid() = default;

    // Create a grid centered at (centerX, centerZ) with given size in meters
    // resolution = meters per cell (default 1.0m)
    NavigationGrid(float centerX, float centerZ, float widthMeters, float heightMeters,
                   float resolution = 1.0f);

    // Mark a cell as blocked or walkable
    void setObstacle(uint32_t cellX, uint32_t cellZ, bool blocked);

    // Mark a rectangular area as blocked
    void setObstacleRect(float worldX, float worldZ, float width, float height, bool blocked);

    // Check if a cell is walkable
    bool isWalkable(uint32_t cellX, uint32_t cellZ) const;

    // Check line of sight between two world positions (true = clear path, no obstacles)
    bool hasLineOfSight(float x1, float z1, float x2, float z2) const;

    // Find path between two world positions
    // Returns empty/invalid path if no path exists
    Path findPath(float startX, float startZ, float endX, float endZ) const;

    // Convert world coordinates to grid cell
    void worldToCell(float worldX, float worldZ, uint32_t& cellX, uint32_t& cellZ) const;

    // Convert grid cell to world coordinates (cell center)
    void cellToWorld(uint32_t cellX, uint32_t cellZ, float& worldX, float& worldZ) const;

    // Grid dimensions
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    float resolution() const { return resolution_; }
    float centerX() const { return centerX_; }
    float centerZ() const { return centerZ_; }

    // Clear all obstacles
    void clear();

    // Resize the grid (clears all obstacles)
    void resize(float centerX, float centerZ, float widthMeters, float heightMeters);

    // Get number of blocked cells (for diagnostics)
    uint32_t blockedCount() const;

private:
    // A* node
    struct Node {
        uint32_t x{0};
        uint32_t z{0};
        float gCost{0.0f};     // Cost from start
        float hCost{0.0f};     // Heuristic to goal
        float fCost{0.0f};     // g + h
        int32_t parentX{-1};   // Parent node position (-1 = none)
        int32_t parentZ{-1};
    };

    // Get cell index in the flat grid array
    uint32_t cellIndex(uint32_t x, uint32_t z) const {
        return z * width_ + x;
    }

    // Check if cell coordinates are in bounds
    bool inBounds(uint32_t x, uint32_t z) const {
        return x < width_ && z < height_;
    }

    // Heuristic function (octile distance for 8-directional movement)
    static float heuristic(uint32_t x1, uint32_t z1, uint32_t x2, uint32_t z2) {
        uint32_t dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);
        uint32_t dz = (z1 > z2) ? (z1 - z2) : (z2 - z1);
        // Octile distance: max(dx,dz) + (sqrt(2)-1) * min(dx,dz)
        if (dx > dz) {
            return static_cast<float>(dx) + 0.414f * static_cast<float>(dz);
        }
        return static_cast<float>(dz) + 0.414f * static_cast<float>(dx);
    }

    // Grid data (true = blocked)
    std::vector<bool> grid_;
    uint32_t width_{0};
    uint32_t height_{0};
    float resolution_{1.0f};

    // World-space origin (center of grid)
    float centerX_{0.0f};
    float centerZ_{0.0f};

    // World-space origin (bottom-left corner)
    float originX_{0.0f};
    float originZ_{0.0f};
};

} // namespace DarkAges
