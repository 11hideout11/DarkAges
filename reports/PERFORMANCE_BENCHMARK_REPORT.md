# PRD-041: Performance Baseline Report

**Date:** 2026-05-03
**Test suite:** 1322 cases, 7310 assertions, 100% pass
**CPU:** x86_64 (WSL2)
**Build:** ENABLE_GNS=OFF, ENABLE_REDIS=OFF, ENABLE_SCYLLA=OFF

## Summary

All benchmarks pass well within the **16ms tick budget** for a 60Hz server tick rate.
The heaviest individual system benchmark (spatial hash insert: 134us) accounts for <1% of a full tick.
A combined full tick simulation (50 NPCs + 5 players with combat + movement + AI) runs at **3.9us** — negligible overhead.

## Benchmark Results

### Core ECS Operations
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| entt_create_1000 | 103 us | 8.8 us | 0.6% |
| entt_iterate_1000 | 12.7 ns | 3.3 ns | <0.01% |
| entt_destroy_1000 | 121 us | 9.9 us | 0.8% |

**Analysis:** EnTT registry operations are near-zero cost. Iteration is effectively
free at 12.7ns per 1000 entities. Creation/destruction remain under 130us.

### Spatial Grid (AOI)
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| spatial_insert_1000 | 134 us | 15.3 us | 0.8% |
| spatial_query_1000_radius50 | 1.82 us | 177 ns | 0.01% |
| aoi_query_200_radius200 | 41.7 us | 4.3 us | 0.3% |

**Analysis:** Spatial queries are extremely fast (sub-2us). Insert is the bottleneck
at ~134us for 1000 entities, but this is a setup cost, not per-tick.

### Movement & Combat
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| movement_500 | 52.3 us | 5.6 us | 0.3% |
| combat_damage_100 | 2.73 us | 356 ns | 0.02% |

**Analysis:** Movement for 500 entities is ~52us. Combat damage for 100 entities
is ~2.7us. Both well within budget.

### NPC AI (New — PRD-041)
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| npca_update_50 | 2.42 us | 102 ns | 0.02% |
| npca_update_200 | 9.53 us | 247 ns | 0.06% |

**Analysis:** Even 200 NPCs update in <10us. NPC AI is not a performance concern
at current scale. Scales linearly (200 NPCs is ~4x 50 NPCs).

### Zone Events (New — PRD-041)
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| zonevent_register_20 | 3.61 us | 318 ns | 0.02% |

### Netcode (New — PRD-041)
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| snapshot_serialize_100 | 1.74 us | 48 ns | 0.01% |
| delta_serialize_100_unchanged | 4.01 us | 773 ns | 0.03% |
| delta_serialize_100_half_changed | 8.56 us | 1.37 us | 0.05% |

**Analysis:** Snapshot serialization is extremely fast. Delta compression
adds ~2-4us overhead vs full snapshots. Half-changed delta is the most
expensive netcode path at 8.6us, still <0.1% of tick budget.

### Full Tick Simulation (New — PRD-041)
| Benchmark | Mean | Std Dev | vs Budget |
|-----------|------|---------|-----------|
| full_tick_50_npcs_5_players | 3.92 us | 233 ns | 0.02% |

**Analysis:** A combined tick with movement + NPC AI + combat calculations
for 50 NPCs and 5 players runs in **3.9us**. The server can handle this
load thousands of times over and remain under 16ms.

## Performance Budgets

| Metric | Current | Budget | Status |
|--------|---------|--------|--------|
| 60Hz tick time | < 1% | 16,000 us | ✅ PASS |
| NPC AI (200 NPCs) | 9.5 us | 1,000 us | ✅ PASS |
| Movement (500 entities) | 52 us | 1,000 us | ✅ PASS |
| Combat (100 entities) | 2.7 us | 1,000 us | ✅ PASS |
| Netcode (100 entities) | 8.6 us | 1,000 us | ✅ PASS |
| Full tick (50 NPCs) | 3.9 us | 1,000 us | ✅ PASS |
| Memory (per 100 entities) | TBD | 512 KB | ⚠️ NOT MEASURED |

## Recommendations

1. **No optimization needed** at current scale. All systems operate at <1% of budget.
2. **Add memory profiling** — entity memory usage per 100 entities should be measured
   to catch leaks and set a per-zone limit.
3. **Scale test** — the microbenchmarks show individual system costs, but a
   stress test with 1000+ concurrent entities and 10+ zones would validate
   linear scaling assumptions.
4. **CI benchmark regression check** — add cmake target to run benchmarks
   and compare against saved baselines in CI.

## How to Re-run

```bash
# All benchmarks
./build/darkages_tests "[.benchmark]" -r console --benchmark-samples 10 -v high

# Category-specific
./build/darkages_tests "[npca][performance]" -r console
./build/darkages_tests "[netcode][performance]" -r console
./build/darkages_tests "[tick][performance]" -r console
```
