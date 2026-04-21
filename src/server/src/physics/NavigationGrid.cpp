#include "physics/NavigationGrid.hpp"
#include <cmath>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <limits>

namespace DarkAges {

// ============================================================================
// NavigationGrid Implementation
// ============================================================================

NavigationGrid::NavigationGrid(float centerX, float centerZ,
                               float widthMeters, float heightMeters,
                               float resolution)
    : resolution_(resolution)
    , centerX_(centerX)
    , centerZ_(centerZ)
{
    resize(centerX, centerZ, widthMeters, heightMeters);
}

void NavigationGrid::resize(float centerX, float centerZ,
                            float widthMeters, float heightMeters) {
    centerX_ = centerX;
    centerZ_ = centerZ;
    width_ = static_cast<uint32_t>(std::ceil(widthMeters / resolution_));
    height_ = static_cast<uint32_t>(std::ceil(heightMeters / resolution_));

    // Ensure minimum grid size
    if (width_ < 4) width_ = 4;
    if (height_ < 4) height_ = 4;

    originX_ = centerX_ - (width_ * resolution_) / 2.0f;
    originZ_ = centerZ_ - (height_ * resolution_) / 2.0f;

    grid_.assign(width_ * height_, false);
}

void NavigationGrid::clear() {
    grid_.assign(width_ * height_, false);
}

void NavigationGrid::setObstacle(uint32_t cellX, uint32_t cellZ, bool blocked) {
    if (inBounds(cellX, cellZ)) {
        grid_[cellIndex(cellX, cellZ)] = blocked;
    }
}

void NavigationGrid::setObstacleRect(float worldX, float worldZ,
                                     float width, float height, bool blocked) {
    uint32_t x1, z1, x2, z2;
    worldToCell(worldX - width / 2.0f, worldZ - height / 2.0f, x1, z1);
    worldToCell(worldX + width / 2.0f, worldZ + height / 2.0f, x2, z2);

    for (uint32_t z = z1; z <= z2; ++z) {
        for (uint32_t x = x1; x <= x2; ++x) {
            setObstacle(x, z, blocked);
        }
    }
}

bool NavigationGrid::isWalkable(uint32_t cellX, uint32_t cellZ) const {
    if (!inBounds(cellX, cellZ)) return false;
    return !grid_[cellIndex(cellX, cellZ)];
}

void NavigationGrid::worldToCell(float worldX, float worldZ,
                                 uint32_t& cellX, uint32_t& cellZ) const {
    int32_t cx = static_cast<int32_t>((worldX - originX_) / resolution_);
    int32_t cz = static_cast<int32_t>((worldZ - originZ_) / resolution_);

    cellX = static_cast<uint32_t>(std::max(0, std::min(static_cast<int32_t>(width_) - 1, cx)));
    cellZ = static_cast<uint32_t>(std::max(0, std::min(static_cast<int32_t>(height_) - 1, cz)));
}

void NavigationGrid::cellToWorld(uint32_t cellX, uint32_t cellZ,
                                 float& worldX, float& worldZ) const {
    worldX = originX_ + (static_cast<float>(cellX) + 0.5f) * resolution_;
    worldZ = originZ_ + (static_cast<float>(cellZ) + 0.5f) * resolution_;
}

uint32_t NavigationGrid::blockedCount() const {
    uint32_t count = 0;
    for (bool blocked : grid_) {
        if (blocked) ++count;
    }
    return count;
}

// ============================================================================
// Line of Sight — Bresenham's line algorithm
// ============================================================================

bool NavigationGrid::hasLineOfSight(float x1, float z1, float x2, float z2) const {
    uint32_t cx1, cz1, cx2, cz2;
    worldToCell(x1, z1, cx1, cz1);
    worldToCell(x2, z2, cx2, cz2);

    // Bresenham's line algorithm
    int32_t dx = static_cast<int32_t>(cx2) - static_cast<int32_t>(cx1);
    int32_t dz = static_cast<int32_t>(cz2) - static_cast<int32_t>(cz1);

    int32_t stepX = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
    int32_t stepZ = (dz > 0) ? 1 : (dz < 0) ? -1 : 0;

    dx = std::abs(dx);
    dz = std::abs(dz);

    int32_t err = dx - dz;
    int32_t x = static_cast<int32_t>(cx1);
    int32_t z = static_cast<int32_t>(cz1);

    while (true) {
        // Check current cell
        if (!isWalkable(static_cast<uint32_t>(x), static_cast<uint32_t>(z))) {
            return false;
        }

        // Reached the end?
        if (static_cast<uint32_t>(x) == cx2 && static_cast<uint32_t>(z) == cz2) {
            return true;
        }

        int32_t e2 = 2 * err;
        if (e2 > -dz) {
            err -= dz;
            x += stepX;
        }
        if (e2 < dx) {
            err += dx;
            z += stepZ;
        }
    }
}

// ============================================================================
// A* Pathfinding
// ============================================================================

Path NavigationGrid::findPath(float startX, float startZ,
                              float endX, float endZ) const {
    Path result;

    // Convert to grid coordinates
    uint32_t sx, sz, ex, ez;
    worldToCell(startX, startZ, sx, sz);
    worldToCell(endX, endZ, ex, ez);

    // Early exit: start or end is blocked
    if (!isWalkable(sx, sz) || !isWalkable(ex, ez)) {
        return result;
    }

    // Early exit: start == end
    if (sx == ex && sz == ez) {
        result.valid = true;
        float wx, wz;
        cellToWorld(ex, ez, wx, wz);
        result.waypoints.push_back({wx, wz});
        return result;
    }

    // Early exit: line of sight is clear (no pathfinding needed)
    if (hasLineOfSight(startX, startZ, endX, endZ)) {
        result.valid = true;
        result.waypoints.push_back({endX, endZ});
        return result;
    }

    // A* search
    // Key for hash map: pack (x, z) into uint64_t
    auto makeKey = [](uint32_t x, uint32_t z) -> uint64_t {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(z);
    };

    // Open set (min-heap by fCost)
    struct CompareNode {
        bool operator()(const Node& a, const Node& b) const {
            return a.fCost > b.fCost; // Min-heap
        }
    };
    std::priority_queue<Node, std::vector<Node>, CompareNode> openSet;

    // Closed set
    std::unordered_map<uint64_t, bool> closedSet;

    // Track best g-cost for each node
    std::unordered_map<uint64_t, float> gCosts;

    // Track parent for path reconstruction
    std::unordered_map<uint64_t, uint64_t> parents;

    // 8-directional movement costs
    static const int32_t DIRS[][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},    // Cardinal
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}   // Diagonal
    };
    static const float CARDINAL_COST = 1.0f;
    static const float DIAGONAL_COST = 1.414f;

    // Add start node
    Node startNode;
    startNode.x = sx;
    startNode.z = sz;
    startNode.gCost = 0.0f;
    startNode.hCost = heuristic(sx, sz, ex, ez);
    startNode.fCost = startNode.hCost;
    startNode.parentX = -1;
    startNode.parentZ = -1;

    openSet.push(startNode);
    gCosts[makeKey(sx, sz)] = 0.0f;

    const uint32_t MAX_ITERATIONS = width_ * height_; // Safety limit
    uint32_t iterations = 0;

    while (!openSet.empty() && iterations < MAX_ITERATIONS) {
        ++iterations;

        Node current = openSet.top();
        openSet.pop();

        uint64_t currentKey = makeKey(current.x, current.z);

        // Skip if already processed with better cost
        if (closedSet.find(currentKey) != closedSet.end()) {
            continue;
        }
        closedSet[currentKey] = true;

        // Check if we reached the goal
        if (current.x == ex && current.z == ez) {
            // Reconstruct path
            result.valid = true;
            result.targetCellX = ex;
            result.targetCellZ = ez;

            // Build path from end to start using parent map
            std::vector<Waypoint> reversedPath;
            uint64_t pathKey = makeKey(ex, ez);

            while (pathKey != makeKey(sx, sz)) {
                float wx, wz;
                cellToWorld(
                    static_cast<uint32_t>(pathKey >> 32),
                    static_cast<uint32_t>(pathKey & 0xFFFFFFFF),
                    wx, wz
                );
                reversedPath.push_back({wx, wz});

                auto parentIt = parents.find(pathKey);
                if (parentIt == parents.end()) break;
                pathKey = parentIt->second;
            }

            // Add start position
            {
                float wx, wz;
                cellToWorld(sx, sz, wx, wz);
                reversedPath.push_back({wx, wz});
            }

            // Reverse to get start->end order, skip first (start position)
            result.waypoints.reserve(reversedPath.size());
            for (int32_t i = static_cast<int32_t>(reversedPath.size()) - 2; i >= 0; --i) {
                result.waypoints.push_back(reversedPath[i]);
            }

            return result;
        }

        // Expand neighbors
        for (const auto& dir : DIRS) {
            int32_t nx = static_cast<int32_t>(current.x) + dir[0];
            int32_t nz = static_cast<int32_t>(current.z) + dir[1];

            // Bounds check
            if (nx < 0 || nz < 0 ||
                nx >= static_cast<int32_t>(width_) ||
                nz >= static_cast<int32_t>(height_)) {
                continue;
            }

            uint32_t nux = static_cast<uint32_t>(nx);
            uint32_t nuz = static_cast<uint32_t>(nz);

            // Walkability check
            if (!isWalkable(nux, nuz)) continue;

            // For diagonal movement, check that both cardinal neighbors are walkable
            // (prevents cutting through corners)
            if (dir[0] != 0 && dir[1] != 0) {
                if (!isWalkable(current.x, nuz) || !isWalkable(nux, current.z)) {
                    continue;
                }
            }

            uint64_t neighborKey = makeKey(nux, nuz);

            // Skip if in closed set
            if (closedSet.find(neighborKey) != closedSet.end()) {
                continue;
            }

            // Calculate movement cost
            float moveCost = (dir[0] != 0 && dir[1] != 0) ? DIAGONAL_COST : CARDINAL_COST;
            float tentativeG = current.gCost + moveCost;

            // Skip if we already found a better path to this node
            auto gIt = gCosts.find(neighborKey);
            if (gIt != gCosts.end() && tentativeG >= gIt->second) {
                continue;
            }

            // This is the best path to this node so far
            gCosts[neighborKey] = tentativeG;
            parents[neighborKey] = currentKey;

            Node neighbor;
            neighbor.x = nux;
            neighbor.z = nuz;
            neighbor.gCost = tentativeG;
            neighbor.hCost = heuristic(nux, nuz, ex, ez);
            neighbor.fCost = neighbor.gCost + neighbor.hCost;
            neighbor.parentX = static_cast<int32_t>(current.x);
            neighbor.parentZ = static_cast<int32_t>(current.z);

            openSet.push(neighbor);
        }
    }

    // No path found
    return result;
}

} // namespace DarkAges
