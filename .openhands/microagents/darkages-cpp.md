---
name: darkages-cpp
type: knowledge
version: 1.0.0
agent:CodeActAgent
triggers:
  - c++
  - cpp
  - server code
  - entt
---
# DarkAges C++ Server Development

## Build Flags (CMake)
- BUILD_TESTS=ON (always)
- FETCH_DEPENDENCIES=ON (FetchContent)
- ENABLE_GNS=OFF | ENABLE_REDIS=OFF | ENABLE_SCYLLA=OFF (CI/staging)

## EnTT Patterns
- View iteration: `registry.view<Comp1, Comp2>()` — prefer `.each()` loops
- Component get: `registry.get<T>(entity)` — requires component exists; use `contains` check if unsure
- After `emplace/patch/remove`: re-fetch pointers/refs (they become invalid)
- DO NOT use `view.size()` — instead count with loop or `std::distance(view.begin(), view.end())`

## Namespace Discipline
Everything in `DarkAges::` namespace ONLY. No unqualified globals. No `using namespace` in headers.

## Networking Stubs
When `ENABLE_GNS=OFF`:
- `GNSNetworkManager` excluded from build (guarded by `#ifdef`)
- `NetworkManager` test stub uses loopback + packet capture fixture
- Do not test real UDP behavior in CI

## Database Stubs
RedisManager / ScyllaManager are stubs when disabled — they return canned responses. No integration tests touch real DB.

## Test Writing (Catch2 v2)
- `TEST_CASE("BEHAVIORY when CONDITION returns RESULT", "[section]")`
- Sections: `[given][when][then]`
- Arrange-Act-Assert; at least 3 distinctive checks per test
- Use `SECTION()` for branching scenarios

## Anti-Patterns to Avoid
- Manual entity destruction — use `registry.destroy(entity)`
- Long-running work in tick systems — keep each system < 2 ms
- Network packet allocation in hot path — use object pools
