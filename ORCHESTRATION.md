# DarkAges AAA Development Orchestration

## Vision Statement

**"Create a polished, skill-driven MMORPG with satisfying combat, meaningful progression, and a living world that feels alive."**

## AAA Quality Targets

### Performance Benchmarks
- **Server Tick**: Consistent < 16ms (60Hz)
- **Client FPS**: 60fps on mid-range hardware, 30fps minimum
- **Network Latency**: < 100ms feel imperceptible
- **Load Times**: < 3 seconds to zone, < 10 seconds full load

### Player Experience Goals
- **Combat Feel**: Responsive, weighty, skill-expressive
- **Progression**: Meaningful choices, visible growth
- **Social**: Easy to find groups, communicate naturally  
- **World**: Dynamic events, living NPC routines

### Code Quality Gates
- **Test Coverage**: > 80% for new systems
- **Lint**: Zero warnings on new code
- **Review**: 2-agent review before merge
- **Documentation**: API docs for all public interfaces

---

## Phase 1: Foundation (Months 1-2)

### Goals
- Player persistence working
- Core social systems (Guild/Party/Chat)
- Basic progression (Quests/Achievements)

### Tasks

#### T1.1: Player Persistence
- [ ] Implement `src/server/src/memory/PlayerProfileStore.hpp`
- [ ] Wire into ZoneServer save/load
- [ ] Test: Character persists after restart
- [ ] Test: Inventory persists

#### T1.2: Guild System  
- [ ] Create `src/server/src/social/GuildSystem.hpp/.cpp`
- [ ] Create packet types (30-34)
- [ ] Create `src/client/scenes/GuildPanel.tscn`
- [ ] Test: Two players form guild

#### T1.3: Party System
- [ ] Create `src/server/src/social/PartySystem.hpp/.cpp`
- [ ] Create packet types (40-44)
- [ ] Create `src/client/scenes/PartyFrame.tscn`
- [ ] Test: Two players party

#### T1.4: Chat System
- [ ] Extend `src/server/src/social/ChatSystem.hpp`
- [ ] Add channel routing
- [ ] Test: Cross-zone chat

### Quality Gate 1
- [ ] All T1.x tests pass
- [ ] No regression in existing tests
- [ ] Documentation updated

---

## Phase 2: Progression (Months 3-4)

### Goals
- Quest system wired
- Crafting functional
- Achievements tracking
- Economy basics

### Tasks

#### T2.1: Quest Integration
- [ ] Wire `src/server/src/content/QuestSystem.cpp` to data/quests.json
- [ ] Connect CombatSystem kills to quest progress
- [ ] Reward delivery
- [ ] Test: Complete a quest

#### T2.2: Crafting System
- [ ] Create `src/server/src/content/CraftingSystem.hpp/.cpp`
- [ ] Recipe database
- [ ] Create `src/client/scenes/CraftingPanel.tscn`
- [ ] Test: Craft an item

#### T2.3: Achievement System
- [ ] Create `src/server/src/progression/AchievementSystem.hpp/.cpp`
- [ ] Trigger points from Quest, Combat, Crafting
- [ ] Create `src/client/scenes/AchievementPanel.tscn`
- [ ] Test: Unlock an achievement

#### T2.4: Basic Economy  
- [ ] Trade window UI
- [ ] Currency handling
- [ ] Test: Player-to-player trade

### Quality Gate 2
- [ ] All T2.x tests pass
- [ ] No regression in Phase 1
- [ ] Documentation updated

---

## Phase 3: Polish (Months 5-6)

### Goals
- Loading experiences
- Minimap/World Map
- Audio atmosphere
- Settings customization
- Matchmaking ready

### Tasks

#### T3.1: Loading Screen
- [ ] Create `src/client/scenes/LoadingScreen.tscn`
- [ ] Progress callbacks
- [ ] Test: Shows on zone load

#### T3.2: Minimap/World Map
- [ ] Create `src/client/scenes/Minimap.tscn`
- [ ] Create `src/client/scenes/WorldMap.tscn`
- [ ] Test: Shows position accurately

#### T3.3: Audio System
- [ ] Create `src/client/scripts/AudioManager.cs`
- [ ] SFX library
- [ ] Music crossfade
- [ ] Test: Spatial audio works

#### T3.4: Settings UI
- [ ] Create `src/client/scenes/SettingsPanel.tscn`
- [ ] Persist to user://settings.cfg
- [ ] Test: Settings survive restart

#### T3.5: Matchmaking
- [ ] Create `src/server/src/matchmaking/MatchmakingSystem.hpp/.cpp`
- [ ] Test: Queue and match

### Quality Gate 3
- [ ] All T3.x tests pass
- [ ] No regression in Phase 1-2
- [ ] 60fps client on test hardware

---

## Integration Roadmap

```
MVP (Current)
   │
   ├─► Phase 1: Foundation
   │     ├─► Player Persistence ──► Login works
   │     ├─► Guild ──► Social groups
   │     ├─► Party ──► Grouping  
   │     └─► Chat ──► Communication
   │
   ├─► Phase 2: Progression  
   │     ├─► Quest ──► Objectives
   │     ├─► Crafting ──► Item creation
   │     ├─► Achievement ──► Goals
   │     └─► Economy ──► Trading
   │
   └─► Phase 3: Polish
         ├─► Loading ──► Feel
         ├─► Minimap ──► Navigation
         ├─► Audio ──► Atmosphere
         ├─► Settings ──► Customization
         └─► Matchmaking ──► PvP queue
```

---

## Task Execution Guidelines

### For Local Agents

1. **Check PRD_GAP_STATUS.md** for feature priority
2. **Find implementation PRD** (file ends in -IMPLEMENTATION.md)
3. **Read acceptance criteria** at bottom of file
4. **Implement to spec exactly**
5. **Write tests** covering criteria
6. **Update AGENTS.md** when complete

### Task Template
```
## Task: [Feature Name]
**PRD**: prd-[feature]-IMPLEMENTATION.md
**Files to create**:
- src/server/src/[module]/[System].hpp
- src/server/src/[module]/[System].cpp  
- src/client/scenes/[Panel].tscn
- src/client/scripts/[Panel].cs

**Test Criteria**:
- [ ] Criterion 1
- [ ] Criterion 2
```

---

## Success Metrics

| Milestone | Criteria |
|----------|----------|
| Phase 1 | 4 systems working, no regression |
| Phase 2 | 4 systems working, no regression |
| Phase 3 | All systems + 60fps client |
| **Final** | Feature-complete, polished, tested |

---

## Quality Enforcement

### Before Any Commit
- [ ] Tests pass: `cd build && ctest`
- [ ] Build succeeds: `cmake --build build`
- [ ] No new warnings
- [ ] Code formatted

### PR Requirements  
- [ ] 2-agent review
- [ ] Tests included
- [ ] Documentation updated
- [ ] AGENTS.md status updated

---

## Reference Documents

- `AGENTS.md` - Authoritative project state
- `PRD_GAP_STATUS.md` - Feature implementation status
- `prd/prd-*-IMPLEMENTATION.md` - Implementation specifications
- `src/server/` - Server code structure
- `src/client/` - Client code structure