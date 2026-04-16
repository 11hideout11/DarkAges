#pragma once

#include "monitoring/MetricsExporter.hpp"
#include <cstdint>
#include <string>

namespace DarkAges {

class ZoneServer;
class NetworkManager;
class ReplicationOptimizer;

// Handles performance budget monitoring, QoS degradation, and network metrics.
// Extracted from ZoneServer to reduce monolithic file size.
class PerformanceHandler {
public:
    explicit PerformanceHandler(ZoneServer& server);
    ~PerformanceHandler() = default;

    // Set subsystem references (called during ZoneServer::initialize)
    void setNetwork(NetworkManager* network) { network_ = network; }
    void setReplicationOptimizer(ReplicationOptimizer* opt) { replicationOptimizer_ = opt; }

    // Check if tick exceeded performance budget, trigger QoS degradation if needed
    void checkPerformanceBudgets(uint64_t tickTimeUs);

    // Update network-related Prometheus metrics (packet loss, bandwidth, etc.)
    void updateNetworkMetrics(Monitoring::MetricsExporter& metrics, const std::string& zoneIdStr);

    // Activate QoS degradation (reduce update rates, bandwidth)
    void activateQoSDegradation();

private:
    ZoneServer& server_;
    NetworkManager* network_{nullptr};
    ReplicationOptimizer* replicationOptimizer_{nullptr};
};

} // namespace DarkAges
