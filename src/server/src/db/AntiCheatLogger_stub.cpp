// Stub implementation for AntiCheatLogger when cassandra-cpp-driver is not available
#include "db/AntiCheatLogger.hpp"
#include "db/ScyllaManager.hpp"  // For AntiCheatEvent struct

namespace DarkAges {

AntiCheatLogger::AntiCheatLogger() = default;
AntiCheatLogger::~AntiCheatLogger() = default;

void AntiCheatLogger::logAntiCheatEvent(CassSession*, const AntiCheatEvent&, WriteCallback) {}
void AntiCheatLogger::logAntiCheatEventsBatch(CassSession*, const std::vector<AntiCheatEvent>&, WriteCallback) {}

} // namespace DarkAges
