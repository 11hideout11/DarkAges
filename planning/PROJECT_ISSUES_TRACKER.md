# DarkAges MMO — Project Issues Tracker

**Generated:** 2026-04-29  
**Purpose:** Centralized tracking of all issues, gaps, and problems found in the project

---

## 📊 Current Project Status Summary

| Aspect | Status | Last Verified |
|--------|--------|---------------|
| **Test Suite** | ✅ 1302 cases, 7249 assertions, ALL PASS | 2026-04-29 |
| **Build** | ✅ Compiles with CMake | 2026-04-29 |
| **Server** | ✅ Starts and runs at 60Hz | 2026-04-29 |
| **Demo Readiness** | ⚠️ CONTRADICTORY - See Issue #1 | - |
| **Phase 1-5** | ❓ UNVERIFIED - No docs | - |

---

## 🚨 Critical Issues (Must Resolve)

### Issue #1: Demo Readiness Contradiction

**Priority:** CRITICAL  
**Status:** OPEN  
**Category:** Documentation

**Description:**
- `PROJECT_STATUS.md` (2026-04-28) states: "NOT READY for Demo MVP"
- `AGENTS.md` claims: "NOW FULLY DEMO READY"

**Impact:** Cannot accurately represent project state to stakeholders

**Resolution Required:**
1. Determine actual demo readiness status
2. Update both documents to reflect consistent position

---

### Issue #2: GNS Integration Blocked

**Priority:** CRITICAL  
**Status:** OPEN  
**Category:** Integration / Dependencies

**Description:**
- GameNetworkingSockets (GNS) requires WebRTC submodule
- WebRTC submodule clone fails: `webrtc.googlesource.com` access denied
- WP-8-6 cannot complete

**Current Workaround:**
- Uses stub UDP layer instead of Steam's GNS

**Impact:**
- Production networking may differ from testing
- Cannot validate GNS-specific features

**Resolution Options:**
1. Find alternative WebRTC source
2. Document as permanent limitation
3. Remove GNS entirely from roadmap

---

### Issue #3: Phase 1-5 Implementation Unverified

**Priority:** CRITICAL  
**Status:** OPEN  
**Category:** Documentation / Verification

**Description:**
- No documentation exists for Phases 1-5
- Cannot verify what work was completed
- Only Phase 0, 6, and 8 have summary documents

**Evidence:**
- No `PHASE1_SUMMARY.md` through `PHASE5_SUMMARY.md`
- Code exists but work is unverified

**Resolution Required:**
1. Create phase summary documents OR
2. Document that phases were skipped/merged
3. Verify code matches claimed functionality

---

## ⚠️ High Priority Issues

### Issue #4: Protocol.cpp Excluded

**Priority:** HIGH  
**Status:** OPEN  
**Category:** Build / Networking

**Description:**
- `Protocol.cpp` is excluded when `ENABLE_GNS=OFF`
- Protobuf protocol implementation depends on GNS
- Test builds use stub network layer

**Current State:**
```
ENABLE_GNS=OFF → Protocol.cpp excluded
ENABLE_GNS=ON → Requires WebRTC (fails to build)
```

**Resolution Required:**
1. Implement non-GNS Protobuf path OR
2. Document as build-time limitation

---

### Issue #5: Database Stubs Active

**Priority:** HIGH  
**Status:** OPEN  
**Category:** Infrastructure

**Description:**
- Redis and ScyllaDB are stubbed in most builds
- No real persistence testing in CI
- `ENABLE_REDIS=OFF` and `ENABLE_SCYLLA=OFF` for test builds

**Current State:**
- Default build uses stubs
- Real DB requires separate infrastructure

**Resolution Required:**
1. Add DB integration tests to CI OR
2. Document stub-only status

---

### Issue #6: No Node-Based FSM

**Priority:** HIGH  
**Status:** OPEN  
**Category:** Gameplay / Client

**Description:**
- AGENTS.md claims "FSM integrated with AnimationStateMachine"
- CRITICAL RULES section states "inline state flags (not node-FSM)"
- No formal state machine implementation found

**Impact:**
- Animation system uses basic transitions
- Complex state logic may be difficult

**Resolution Required:**
1. Implement proper AnimationStateMachine node OR
2. Update documentation to reflect current state

---

## 📝 Medium Priority Issues

### Issue #7: Blend Spaces Not Implemented

**Priority:** MEDIUM  
**Status:** OPEN  
**Category:** Gameplay / Animation

**Description:**
- Marked as MEDIUM priority in AGENTS.md
- Animation blending uses crossfade only
- No procedural blend spaces

**Impact:**
- Character animations less fluid
- Limited animation quality

**Resolution:** Deferred to post-MVP

---

### Issue #8: Godot Client Headless Artifacts

**Priority:** MEDIUM  
**Status:** OPEN  
**Category:** Client / Testing

**Description:**
- `add_child()` failures during `_Ready()`
- `!is_inside_tree()` warnings
- Quaternion normalization issues

**Impact:** Non-fatal, seen in headless testing only

**Resolution:** Document as known artifacts

---

### Issue #9: Validator Connection Exhaustion

**Priority:** MEDIUM  
**Status:** OPEN  
**Category:** Testing Tools

**Description:**
- `live_client_validator.py` aggressive UDP retries
- Can exhaust server connection slots
- Does not affect validation results

**Resolution:** Add connection pooling or rate limiting

---

### Issue #10: Documentation Drift

**Priority:** MEDIUM  
**Status:** OPEN  
**Category:** Documentation

**Description:**
- Multiple markdown files reference pre-April state
- Inconsistent numbers across docs (test counts, etc.)
- AGENTS.md updated but other docs stale

**Resolution:** Audit and update all docs

---

## ✅ Resolved Issues

### Issue #11: Test Pass Rate Inconsistency

**Status:** RESOLVED  
**Resolution Date:** 2026-04-29

**Description:** 
- Old docs showed 77% pass rate
- Current: 1302 test cases, 7249 assertions, ALL PASS

**Resolution:** Verified current tests pass

---

### Issue #12: Server Startup Crash

**Status:** RESOLVED  
**Resolution Date:** 2026-04-29

**Description:**
- PERFORMANCE_BENCHMARK_REPORT.md (Jan 30) documented crash
- Current build runs successfully

**Resolution:** Issue appears resolved in current build

---

### Issue #13: Memory Leaks in Tests

**Status:** RESOLVED (WONT FIX)  
**Resolution Date:** 2026-04-29

**Description:**
- LeakDetector tests intentionally detect leaks
- These are tests of the leak detector, not real leaks

**Resolution:** Documented as intentional behavior

---

## 📋 Phase Completion Status

| Phase | Status | Evidence | Notes |
|-------|--------|----------|-------|
| Phase 0 | ✅ COMPLETE | PHASE0_SUMMARY.md | Foundation |
| Phase 1 | ❓ UNVERIFIED | No docs | Prediction & Reconciliation |
| Phase 2 | ❓ UNVERIFIED | No docs | Multi-Player Sync |
| Phase 3 | ❓ UNVERIFIED | No docs | Combat & Lag Compensation |
| Phase 4 | ❓ UNVERIFIED | No docs | Spatial Sharding |
| Phase 5 | ❓ UNVERIFIED | No docs | Optimization & Security |
| Phase 6 | ✅ COMPLETE | PHASE_6_COMPLETION_SUMMARY.md | Build System |
| Phase 7 | ✅ COMPLETE | 1302 tests passing | Unit Testing |
| Phase 8 | ⚠️ PARTIAL | Multiple docs | Production Hardening |
| Phase 9 | ✅ COMPLETE | Performance validated | Performance |

---

## 🔧 Build Configuration

Current validation build uses:
```bash
cmake -S . -B build_validate \
  -DBUILD_TESTS=ON \
  -DFETCH_DEPENDENCIES=ON \
  -DENABLE_GNS=OFF \
  -DENABLE_REDIS=OFF \
  -DENABLE_SCYLLA=OFF
```

---

## 📝 Action Items

### Immediate (This Session)
- [ ] Resolve Demo Readiness contradiction (Issue #1)
- [ ] Document Phase 1-5 status or create docs (Issue #3)

### Short Term (Next Sprint)
- [ ] Address GNS integration path (Issue #2)
- [ ] Fix Protocol.cpp exclusion (Issue #4)
- [ ] Update documentation consistency (Issue #10)

### Medium Term
- [ ] Implement node-based FSM (Issue #6)
- [ ] Add blend spaces (Issue #7)
- [ ] Fix validator connection handling (Issue #9)

---

*Last Updated: 2026-04-29*
