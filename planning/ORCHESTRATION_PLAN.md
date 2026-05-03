# DarkAges MMO: Orchestration Execution Plan

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Active - Phase 2 In Progress

---

## Implementation Status (2026-05-03)

### PRD-036: Player Progression Loop - IN PROGRESS ✓
- [x] XP formula updated (PRD-036: Level*100 + Level²*10)
- [x] TalentPoints field added to PlayerProgression
- [x] Talent points unlock every 2 levels
- [x] CombatState added: baseDamage, finalDamage, finalArmor, STR, DEX, VIT
- [x] CombatState::recalculateStats() method added
- [x] Level-up stat bonuses applied (STR+1, DEX+1, VIT+1)
- [x] ProgressionCalculator created
- [x] Tests added for PRD-036 features

### PRD-037: World Progression - IN PROGRESS ✓
- [x] Zone unlock fields added to PlayerProgression
- [x] WorldProgressionSystem created
- [x] Tutorial → Arena → Boss progression enforced
- [x] ZoneCompletionHandler integrates with ZoneObjectiveSystem
- [x] Tests added for World Progression

### PRD-043: Quest Integration - EXISTING
- Quest system exists with rewards, quests provide XP

### PRD-038: Production Monitoring - COMPLETE ✓
**Existing:** MetricsExporter already implements all PRD-038 requirements

- [x] /metrics endpoint (Prometheus format)
- [x] /health endpoint  
- [x] Tick metrics (duration, count)
- [x] Player count/capacity gauges
- [x] Memory usage tracking
- [x] Packet loss tracking

### PRD-039: Account System - COMPLETE ✓
**Implemented:** Account creation, authentication, sessions, bans

- [x] AccountSystem created
- [x] createAccount()
- [x] authenticate() with failed login lockout
- [x] Session tokens (7 day expiry)
- [x] banAccount() / unbanAccount()
- [x] Account statistics
- [x] Tests added

### PRD-040: Endgame Systems - IN PROGRESS
**Existing:** LeaderboardSystem.cpp, AchievementSystem.cpp

- [x] Leaderboards (existing)
- [x] Achievements (existing)
- [x] DailyChallengeSystem new file
- [x] 6 challenge types
- [x] Daily reset logic

### PRD-042: Client Polish - DOCUMENTED
- Loading transitions
- Toast notifications
- Tutorial overlays
- Client-only work required

---

This document orchestrates development toward a cohesive AAA MMO by enabling local agents to execute well-defined tasks while maintaining rigorous big-picture oversight through quality gates and integration checkpoints.

---

## Vision Alignment

### North Star
> **"A polished, replayable multiplayer action RPG where players journey through a dangerous world, leveling up, defeating bosses, and competing on leaderboards - with responsive combat, meaningful progression, and production-grade reliability."**

### Technical Benchmarks
- **Tick Rate:** 60Hz fixed, <16ms at 100 players
- **Latency:** <100ms input responsiveness
- **Load Time:** <3 seconds per zone
- **Uptime:** 99.9% production availability
- **Player Cap:** 1000 concurrent per zone

### Player Experience Goals
- Player feels " powerful" when leveling up
- Combat is responsive and satisfying
- Progression loop is clear and rewarding
- World map shows meaningful journey
- End-game provides replayability
- Polish creates emotional connection

---

## Phase Overview

| Phase | Focus | Weeks | Key PRDs | Ship Criteria |
|-------|-------|-------|---------|-------------|
| **Phase 1** | Core Gameplay | 1-4 | PRD-036, PRD-037, PRD-043 | Playable journey: Tutorial → Arena → Boss |
| **Phase 2** | Production Ready | 5-8 | PRD-038, PRD-039, PRD-041 | 24/7 operations with visibility |
| **Phase 3** | AAA Polish | 9-12 | PRD-040, PRD-042 | Polished end-game + UX excellence |

---

## Phase 1: Core Gameplay (Weeks 1-4)

### Objective
Create a complete, playable character journey from account creation through end-game content, with meaningful progression and quest integration.

### Deliverables by Week

#### Week 1-2: Player Progression System
**Target:** PRD-036 - Player Progression Loop

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-1.1 | XP Integration | Server + Game Design | XP on kill, XP on quest complete | XP awarded correctly per enemy/quest |
| WP-1.2 | Level Up Logic | Server | Level thresholds, stat bonuses | Stats increase visibly |
| WP-1.3 | Talent Points | Server + UI | Point unlock, UI display | Points available every 2 levels |
| WP-1.4 | Equipment Scaling | Server | Scale formula implemented | Tooltip shows scaled values |

**Acceptance Criteria:**
- [ ] Player can earn XP from combat
- [ ] Level up triggers at correct thresholds
- [ ] Stat increases visible (+5 HP, +1 per stat)
- [ ] Talent points unlock at levels 2, 4, 6, 8, 10
- [ ] Equipment stats scale with player level

**Integration Test:**
```
1. Create new character
2. Kill 10 enemies → Should reach ~Level 2
3. Check stats increased from baseline
4. Verify talent point available
```

---

#### Week 3: World Progression
**Target:** PRD-037 - World Progression System

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-1.5 | Tutorial Entry Gate | Server | New chars spawn in Tutorial | Player starts in Tutorial zone |
| WP-1.6 | Zone Unlock System | Server | Tutorial→Arena→Boss locks | Cannot access locked zones |
| WP-1.7 | World Map UI | Client | Zone status display | Accessible/locked visible |

**Acceptance Criteria:**
- [ ] New characters spawn in Tutorial (zone 98)
- [ ] Cannot leave Tutorial until objectives complete
- [ ] Arena unlocks after Tutorial
- [ ] Boss unlocks after Arena
- [ ] World map shows zone status

**Integration Test:**
```
1. New character → Starts in Tutorial
2. Complete Tutorial objectives → Arena unlocks
3. Complete Arena → Boss unlocks
4. Verify zone locks in world map
```

---

#### Week 4: Quest Integration
**Target:** PRD-043 - Quest System Complete

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-1.8 | Quest Database | Game Design | 20 quests in JSON | Quests parse without error |
| WP-1.9 | Quest Acceptance | Server | Accept/track/complete flow | Quest log shows active |
| WP-1.10 | Quest Rewards | Server + Progression | XP/gold/item rewards | Rewards grant correctly |
| WP-1.11 | Quest Log UI | Client | Quest display panel | Objectives visible |

**Acceptance Criteria:**
- [ ] At least 20 quests in database
- [ ] Quests available from NPCs
- [ ] Progress tracks correctly
- [ ] Rewards grant on completion
- [ ] Quest chain解锁 works

**Integration Test:**
```
1. Accept quest from NPC
2. Complete objectives (kill, collect, talk)
3. Return to NPC → Receive reward
4. Next quest in chain unlocks
```

---

### Phase 1 Quality Gates

| Gate | Criteria | Sign-Off |
|------|----------|----------|
| G-1.1 | XP loop functional | Player gains XP from combat, quest completion |
| G-1.2 | Level progression works | Level up at thresholds, stats increase |
| G-1.3 | World map locked/unlocked | Cannot skip Tutorial, ordered progression |
| G-1.4 | Quest acceptance + rewards | 20 quests functional with rewards |
| G-1.5 | **INTEGRATION:** Full journey playable | Tutorial → Arena → Boss end-to-end |

---

## Phase 2: Production Ready (Weeks 5-8)

### Objective
Enable 24/7 production operations with monitoring, account management, and performance benchmarks.

### Deliverables by Week

#### Week 5-6: Production Monitoring
**Target:** PRD-038 - Production Monitoring

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-2.1 | Dashboard HTTP | Server | /health, /metrics endpoints | Dashboard loads in <1s |
| WP-2.2 | Player Monitoring | Server | Active player list, zone counts | Player list shows correct |
| WP-2.3 | Performance Metrics | Server | Tick time, FPS, memory | Metrics refresh 1s |
| WP-2.4 | Alert System | Server | CPU, player thresholds | Alerts trigger correctly |

**Acceptance Criteria:**
- [ ] Health endpoint returns 200
- [ ] Player list accurate within 5s
- [ ] Tick time logged per frame
- [ ] Alerts fire on threshold breach

---

#### Week 6-7: Account System
**Target:** PRD-039 - Account System Complete

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-2.5 | Account Creation | Server | Create account flow | Account created successfully |
| WP-2.6 | Authentication | Server | Login with session token | Login succeeds/fails |
| WP-2.7 | Character Binding | Server | Character→account linking | Characters persist correctly |
| WP-2.8 | Ban System | Server | Account-level bans | Banned cannot login |

**Acceptance Criteria:**
- [ ] Account creation works
- [ ] Login authenticates correctly
- [ ] Characters linked to account
- [ ] Bans enforced immediately

---

#### Week 7-8: Performance Optimization
**Target:** PRD-041 - Server Performance

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-2.9 | Tick Benchmark | Server | Tick time at 100 players | <16ms P50 |
| WP-2.10 | Memory Verification | Server | Memory usage stable | <10MB drift over 1h |
| WP-2.11 | Network Optimization | Server | Bandwidth per player | <60KB/s |
| WP-2.12 | Load Test | Server + QA | 100 player test | Stable at target |

**Acceptance Criteria:**
- [ ] Tick <16ms at 100 players
- [ ] Memory stable over 1 hour
- [ ] Bandwidth within limits
- [ ] Load test passes

---

### Phase 2 Quality Gates

| Gate | Criteria | Sign-Off |
|------|----------|----------|
| G-2.1 | Dashboard functional | All metrics visible, alerts fire |
| G-2.2 | Account system works | Create, login, ban all functional |
| G-2.3 | Performance targets met | Tick <16ms @100, memory stable |
| G-2.4 | **INTEGRATION:** 24/7 ops capable | Full monitoring + accounts + performance |

---

## Phase 3: AAA Polish (Weeks 9-12)

### Objective
Deliver polished end-game content and refined user experience that creates emotional connection.

### Deliverables by Week

#### Week 9-10: End-Game Systems
**Target:** PRD-040 - End-Game Systems

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-3.1 | Hard Mode | Server | Hard versions of zones | 50% difficulty increase |
| WP-3.2 | Daily Challenges | Server + UI | 3 daily challenges | Reset at midnight |
| WP-3.3 | New Game Plus | Server | NG+ restart feature | Scales correctly |
| WP-3.4 | Leaderboards | Server + UI | Kill/damage leaderboards | Rankings accurate |

**Acceptance Criteria:**
- [ ] Arena/Boss Hard mode accessible
- [ ] Daily challenges reset correctly
- [ ] NG+ scales properly (enemy level × 1.1 per NG+)
- [ ] Leaderboards populate correctly

---

#### Week 10-11: Client Polish
**Target:** PRD-042 - Client Polish

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-3.5 | Loading Transitions | Client | Smooth loading UI | <3s load, fade effects |
| WP-3.6 | Toast Notifications | Client | Level up, achievement toasts | Queue and auto-dismiss |
| WP-3.7 | Error Handling | Client | Consistent error UI | No stack traces shown |
| WP-3.8 | Celebration Effects | Client | Particle bursts, sounds | Level up celebrations |

**Acceptance Criteria:**
- [ ] Loading has progress + tips
- [ ] Toasts display and auto-dismiss
- [ ] Errors clear, no crashes
- [ ] Level up feels celebratory

---

#### Week 11-12: Integration & Polish Pass
**Target:** End-to-End Polish

**Work Packages:**

| WP | Task | Agent Type | Deliverable | Quality Gate |
|----|------|-----------|------------|--------------|
| WP-3.9 | Full Journey QA | QA | Tutorial → End-game test | All features work |
| WP-3.10 | Performance QA | QA | 60 FPS, no lag | Stable under load |
| WP-3.11 | UX QA | QA | No jarring transitions | Feel polished |

---

### Phase 3 Quality Gates

| Gate | Criteria | Sign-Off |
|------|----------|----------|
| G-3.1 | End-game content complete | Hard mode, daily, NG+ working |
| G-3.2 | UX polish verified | Toasts, loading, errors all polished |
| G-3.3 | Full integration test | End-to-end journey works |
| G-3.4 | **SHIP:** Ready for production | All phases pass + polish verified |

---

## Oversight Framework

### Continuous Alignment Checks

Every sprint (weekly):

1. **Vision Check:** Does work align with "North Star" goals?
2. **Technical Check:** Are benchmarks being met?
3. **Integration Check:** Do new features connect to existing systems?
4. **Quality Check:** Does work pass quality gates?

### Integration Points Map

```
Core Progression (PRD-036)
├── XP from Combat → CombatSystem
├── XP from Quests → QuestSystem  
├── Level → Stat Bonuses → CombatSystem
├── Talents → Combat Abilities → CombatSystem
└── Save ← SaveSystem

World Progression (PRD-037)
├── Tutorial Completeness → Zone Objectives
├── World Map UI ← All zones
├── Zone Locks → ZoneServer
└── Save ← SaveSystem

Quest (PRD-043)
├── Dialogue → NPC System
├── Rewards → Progression
├── Objectives → CombatSystem
└── Save ← SaveSystem
```

### Quality Gate Process

1. **Pre-Gate:** Agent completes work package
2. **Self-Check:** Agent verifies against acceptance criteria
3. **Gate Review:** Peer verifies all criteria pass
4. **Integration Test:** Combined systems tested
5. **Sign-Off:** Phase lead approves

---

## Progress Tracking

### Weekly Milestone Format

| Week | Target | Status | Blocker? | Notes |
|------|--------|--------|---------|-------|
| 1 | WP-1.1, 1.2 | | | |
| 2 | WP-1.3, 1.4 | | | |
| 3 | WP-1.5, 1.6, 1.7 | | | |
| 4 | WP-1.8-1.11 | | | |
| ... | ... | | | |

### Sign-Off Requirements

Each phase requires:
- [ ] All quality gates pass
- [ ] Integration tests pass
- [ ] Peer review complete
- [ ] Documentation updated
- [ ] Phase lead sign-off

---

## Quick Reference

### Agent Task Selection

| Agent Type | Available Work Packages |
|------------|------------------------|
| Server Dev | WP-1.1, 1.2, 1.3, 1.5, 1.6, 1.9, 2.1-2.4, 2.5-2.8, 2.9-2.12, 3.1, 3.2, 3.3 |
| Client Dev | WP-1.4, 1.7, 1.11, 2.2, 3.2, 3.5-3.8 |
| Game Design | WP-1.1, 1.8, 3.1, 3.2, 3.3 |
| QA Test | All integration tests |

### Critical Paths

**Path 1 (Core Loop):** WP-1.1 → 1.2 → 1.3 → 1.4 → 1.5 → 1.6 → 1.7 → 1.8 → 1.9 → 1.10  
**Path 2 (Production):** WP-2.1 → 2.2 → 2.5 → 2.6 → 2.9 → 2.12  
**Path 3 (Polish):** WP-3.1 → 3.3 → 3.5 → 3.6 → 3.7 → 3.8 → 3.9

---

**Document Owner:** Orchestration Lead  
**Review Cadence:** Weekly  
**Last Updated:** 2026-05-03