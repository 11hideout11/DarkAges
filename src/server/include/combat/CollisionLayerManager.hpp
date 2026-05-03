#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace DarkAges::combat {

class CollisionLayerManager {
public:
    void registerLayer(const std::string& name, uint32_t bitmask);
    void setCollisionRule(uint32_t layerA, uint32_t layerB, bool canCollide);
    bool canCollide(uint32_t layerA, uint32_t layerB) const;

private:
    std::unordered_map<std::string, uint32_t> layerNames_;
    std::unordered_set<uint64_t> allowedPairs_;

    static uint64_t makePair(uint32_t a, uint32_t b);
};

// Global accessor
CollisionLayerManager& getGlobalCollisionLayerManager();

} // namespace DarkAges::combat

#endif // DARKAGES_COMBAT_COLLISIONLAYERMANAGER_HPP
