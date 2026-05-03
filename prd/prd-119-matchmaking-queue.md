# PRD-119: Matchmaking & Queue System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** PartySystem (exists)

---

## Introduction

Players need a way to quickly find matches or groups without manually building parties. A matchmaking system is crucial for player retention and reducing friction to gameplay.

## Goals

- Add player to matchmaking queue
- Match players by skill/level
- Create match when sufficient players ready
- Handle queue timeouts
- Support multiple game modes

## User Stories

### US-001: Quick Join
**Description:** As a solo player, I want to join a queue so that I can find a match quickly.

**Acceptance Criteria:**
- [ ] "Find Match" button in UI
- [ ] Player enters queue on click
- [ ] Queue position displayed
- [ ] Can cancel queue

### US-002: Match Found
**Description:** As a queued player, I want to be notified when a match is found.

**Acceptance Criteria:**
- [ ] Notification when match ready
- [ ] Accept/decline option
- [ ] Match starts if all accept
- [ ] Timeout if no response

### US-003: Skill Matching
**Description:** As a player, I want to face players of similar skill.

**Acceptance Criteria:**
- [ ] Skill rating per player
- [ ] Matchmaking considers skill
- [ ] Skill bracket widening over time

### US-004: Queue Status
**Description:** As a queued player, I want to see queue status.

**Acceptance Criteria:**
- [ ] Estimated wait time
- [ ] Players in queue
- [ ] Mode selected

## Functional Requirements

- FR-1: Queue system handles 1000+ players
- FR-2: Match found within 2 minutes (average)
- FR-3: Skill rating persisted
- FR-4: Multiple concurrent queues (different modes)

## Non-Goals

- Ranked mode (deferred)
- Tournament brackets (deferred)
-spectator mode (deferred)

---

**PRD Status:** Proposed  
**Author:** OpenHands Gap Analysis  
**Next Step:** Create MatchmakingSystem.hpp