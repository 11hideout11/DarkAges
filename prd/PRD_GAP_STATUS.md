# PRD Gap Status & Implementation Guide

## Overview

This document tracks the gap between PRD specifications and actual implementation.

**Critical Finding**: Many PRD files exist for features that have **NO** server/client implementation in `src/server/` or `src/client/`.

## Implementation Status Legend

| Status | Meaning |
|--------|---------|
| ✅ IMPLEMENTED | Server and/or client code exists and working |
| 📋 PRD_ONLY | PRD exists but NO implementation code |
| ⚠️ PARTIAL | Some code exists but incomplete |
| 🔧 BLOCKED | Implementation exists but blocked by external dependency |

---

## Core Systems

### Server Core (PRD-001)
- **Status**: ✅ IMPLEMENTED
- **Location**: `src/server/src/` - ECS, zones, components
- **Notes**: Working, 32K LOC

### Networking (PRD-002)  
- **Status**: ✅ IMPLEMENTED
- **Location**: `src/server/src/netcode/`, `src/shared/Protocol.cpp`
- **Notes**: UDP snapshots, custom protocol working

### Combat (PRD-003)
- **Status**: ✅ IMPLEMENTED
- **Location**: `src/server/src/combat/`
- **Notes**: FSM, damage calc, hit detection working

### Sharding (PRD-004)
- **Status**: ✅ IMPLEMENTED
- **Location**: `src/server/src/zones/`
- **Notes**: Zone handoff working

---

## Social Systems (NOT IMPLEMENTED)

### Guild System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-guild-system.md`, `prd/prd-guild-ui-system.md`
- **Implementation**: NONE in src/server/ or src/client/
- **Priority**: P1 (Multiplayer)
- **Files Needed**: 
  - `src/server/src/social/GuildSystem.hpp/.cpp`
  - `src/server/include/darkages/components/Guild.hpp`
  - Client: GuildPanel.tscn, GuildChat.cs

### Party System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-party-system.md`, `prd/prd-party-ui-system.md`
- **Implementation**: NONE
- **Priority**: P1 (Multiplayer)
- **Files Needed**:
  - `src/server/src/social/PartySystem.hpp/.cpp`
  - Client: PartyFrame.tscn

### Chat/Social System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-chat-social-system.md`
- **Implementation**: NONE (basic only in NetworkManager)
- **Priority**: P1 (Multiplayer)
- **Files Needed**:
  - `src/server/src/social/ChatSystem.hpp/.cpp`
  - PACKET_CHAT_* packet types
  - Client: ChatBox.tscn

### Friend System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-friend-system.md`, `prd/prd-friend-system-ui.md`
- **Implementation**: NONE
- **Priority**: P1 (Multiplayer)
- **Files Needed**:
  - `src/server/src/social/FriendSystem.hpp/.cpp`
  - Client: FriendsList.tscn

### Mail System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-mail-system.md`
- **Implementation**: NONE
- **Priority**: P2 (Gameplay)
- **Files Needed**:
  - `src/server/src/social/MailSystem.hpp/.cpp`
  - Client: MailUI.tscn

---

## Player Progression (NOT IMPLEMENTED)

### Crafting System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-crafting-system.md`
- **Implementation**: NONE
- **Priority**: P2 (Gameplay)
- **Data**: `data/items.json` has crafting ingredients

### Achievement System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-achievement-system.md`, `prd/prd-achievement-ui-system.md`
- **Implementation**: NONE
- **Priority**: P2 (Gameplay)
- **Files Needed**:
  - `src/server/src/progression/AchievementSystem.hpp/.cpp`
  - Client: AchievementPanel.tscn

### Experience/Leveling
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-experience-leveling-system.md`
- **Implementation**: PARTIAL (XP on kill exists in CombatSystem)
- **Priority**: P2 (Gameplay)
- **Files Needed**:
  - Level advancement logic server-side
  - Client: LevelUpNotification.tscn

---

## Trading & Economy (NOT IMPLEMENTED)

### Trade System
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-trade-system.md`
- **Implementation**: NONE
- **Priority**: P2 (Gameplay)
- **Files Needed**:
  - `src/server/src/economy/TradeSystem.hpp/.cpp`
  - Trade window UI

### Economy
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-economy-trading-system.md`
- **Implementation**: NONE
- **Priority**: P2 (Gameplay)
- **Notes**: Depends on Trade and Crafting systems

---

## Matchmaking & Leaderboard (NOT IMPLEMENTED)

### Matchmaking Queue
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-matchmaking-queue.md`
- **Implementation**: NONE
- **Priority**: P2 (Gameplay)
- **Files Needed**:
  - `src/server/src/matchmaking/MatchmakingSystem.hpp/.cpp`
  - Queue management for PvP/PvE

### Leaderboard
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-leaderboard-system.md`, `prd/prd-leaderboard-ui.md`
- **Implementation**: NONE
- **Priority**: P3 (Polish)
- **Files Needed**:
  - `src/server/src/progression/Leaderboard.hpp/.cpp`
  - Client: LeaderboardPanel.tscn

---

## Client UI (NOT IMPLEMENTED)

### Minimap/World Map
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-minimap-system.md`, `prd/prd-minimap-world-map.md`
- **Implementation**: NONE
- **Priority**: P3 (Polish)
- **Files Needed**:
  - Client: Minimap.tscn, WorldMap.tscn

### Loading Screen
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-loading-screen.md`, `prd/prd-loading-screen-system.md`
- **Implementation**: NONE
- **Priority**: P3 (Polish)
- **Files Needed**:
  - Client: LoadingScreen.tscn

### Audio/Sound
- **Status**: 📋 PRD_ONLY
- **PRD File**: `prd/prd-audio-system.md`, `prd/prd-audio-system-client.md`
- **Implementation**: NONE
- **Priority**: P3 (Polish)
- **Files Needed**:
  - Client: AudioManager.cs, SoundPlayer.tscn

### Settings UI
- **Status**: ⚠️ PARTIAL
- **Implementation**: Basic only
- **Priority**: P3 (Polish)
- **Files Needed**:
  - Client: SettingsPanel.tscn expansion

### Character Creation
- **Status**: ⚠️ PARTIAL
- **Implementation**: Minimal
- **Priority**: P2 (Gameplay)
- **Files Needed**:
  - Client: CharacterCreator.tscn

---

## Persistence (NEEDS IMPLEMENTATION)

### Player Persistence
- **Status**: ⚠️ PARTIAL
- **Implementation**: Session only, no disk persistence
- **Notes**: Data lost on disconnect
- **Priority**: P0 (MVP Critical)
- **Files Needed**:
  - Redis profile cache
  - Scylla persist (when DB enabled)
  - Character selection UI

---

## Integration Work Needed

### NPC Dialogue
- **Status**: 📋 DATA_EXISTS
- **Data File**: `data/dialogues.json` (exists)
- **Implementation**: NONE (needs wiring)
- **Priority**: P2
- **Files Needed**:
  - `src/server/src/dialogue/DialogueSystem.hpp/.cpp`
  - Client: DialogueBox.tscn

### Quest System
- **Status**: 📋 DATA_EXISTS
- **Data File**: `data/quests.json` (exists)
- **Implementation**: PARTIAL (needs full integration)
- **Priority**: P2
- **Files Needed**:
  - Server: QuestSystem full integration
  - Client: QuestTracker.tscn expansion

---

## External Blockers (Not Implementation Gaps)

### GNS Runtime (PRD-012)
- **Status**: 🔧 BLOCKED
- **Issue**: WebRTC submodule auth
- **Current**: Custom UDP stub working

### Production Database (PRD-018)
- **Status**: 🔧 BLOCKED  
- **Issue**: Docker not available
- **Current**: Stub implementations work

---

## Implementation Priority Order

### Phase 1: P0 - MVP Critical
1. Player Persistence (persistence across restarts)

### Phase 2: P1 - Multiplayer Core
2. Guild System
3. Party System  
4. Chat System
5. Friend System

### Phase 3: P2 - Gameplay
6. Quest System (integration)
7. NPC Dialogue (wiring)
8. Crafting System
9. Achievement System
10. Mail System

### Phase 4: P3 - Polish
11. Matchmaking Queue
12. Leaderboard
13. Minimap/World Map
14. Loading Screen
15. Audio System
16. Settings UI (full)

---

## How to Use This Document

When starting work on any feature:
1. Check this document for status
2. If 📋 PRD_ONLY → implementation is needed (create code in src/)
3. Look for **-IMPLEMENTATION.md** version of the PRD
4. Verify with: `ls src/server/src/` or `ls src/client/scripts/`
5. Reference implementation PRD in `prd/` for exact code specs

**CRITICAL**: Implementation PRDs now exist with explicit code paths!

### Implementation PRD Files Created

| Feature | Implementation PRD | Status |
|---------|------------------|--------|
| Guild System | `prd-guild-system-IMPLEMENTATION.md` | Use this for implementation |
| Party System | `prd-party-system-IMPLEMENTATION.md` | Use this for implementation |
| Chat System | `prd-chat-system-IMPLEMENTATION.md` | Use this for implementation |
| Friend System | `prd-friend-system-IMPLEMENTATION.md` | Use this for implementation |
| Quest System | `prd-quest-system-INTEGRATION.md` | Data exists, wire it |
| Crafting | `prd-crafting-system-IMPLEMENTATION.md` | Use this for implementation |
| Achievement | `prd-achievement-system-IMPLEMENTATION.md` | Use this for implementation |
| Minimap | `prd-minimap-IMPLEMENTATION.md` | Use this for implementation |
| Loading Screen | `prd-loading-screen-IMPLEMENTATION.md` | Use this for implementation |