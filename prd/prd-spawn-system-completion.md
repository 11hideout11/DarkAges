# PRD: Spawn System Completion

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Gameplay - World Population

---

## 1. Problem Statement

The server has SpawnSystem.hpp but NPCs don't spawn in the world. The client shows empty zones. Players encounter no enemies, NPCs, or life - the world feels empty.

### Current State
- ⚠️ SpawnSystem.hpp stub may exist
- ⚠️ No spawn points configured
- ⚠️ No NPC entities in world
- ⚠️ No enemy spawn on entity death

### Impact
- Zones have no content
- No enemies to fight
- No NPCs to interact with
- World feels empty

---

## 2. Goals

### Primary Goals
1. Define NPC/enemy spawn configurations
2. Implement spawn point components
3. Build wave spawning for combat
4. Create respawn logic

### Success Criteria
- [ ] NPCs spawn at configured points
- [ ] Enemies respawn after death
- [ ] Wave spawning works
- [ ] Spawn/despawn animations

---

## 3. Technical Specification

### Spawn Configuration

```json
{
  "spawn_zones": [
    {
      "zoneId": 1,
      "name": "Goblin Camp",
      "spawn_points": [
        {
          "id": "goblin_captain_1",
          "entity_type": "npc_goblin",
          "position": {"x": 10, "y": 0, "z": 20},
          "respawn_time": 300,
          "spawn_script": "patrol_camp"
        },
        {
          "id": "goblin_guard_1",
          "entity_type": "enemy_goblin",
          "position": {"x": 12, "y": 0, "z": 18},
          "count": 3,
          "respawn_time": 60
        }
      ]
    }
  ],
  "wave_configs": [
    {
      "waveId": "goblin_camp_wave",
      "trigger": "enter_zone",
      "spawns": [
        {"type": "enemy_goblin", "count": 5, "delay": 0},
        {"type": "enemy_goblin_archer", "count": 2, "delay": 5},
        {"type": "npc_goblin_chief", "count": 1, "delay": 15}
      ]
    }
  ]
}
```

### Entity Definitions

```cpp
// EntityFactory.hpp
class EntityFactory {
public:
    Entity CreateNPC(uint32_t npcTemplateId);
    Entity CreateEnemy(uint32_t enemyTemplateId);
    Entity CreateWorldItem(uint32_t itemId, Vector3 position);
    
private:
    std::unordered_map<uint32_t, EntityTemplate> templates_;
};

// EntityTemplate
struct EntityTemplate {
    std::string name;
    uint32_t modelId;
    std::string aiBehavior;
    Stats baseStats;
    std::vector<uint32_t> lootTable;
};
```

### Spawn System

```cpp
// SpawnSystem.hpp
class SpawnSystem {
public:
    void Initialize();
    void OnZoneLoaded(Entity zone);
    void OnEntityKilled(Entity entity);
    void Update(float deltaTime);
    
private:
    void SpawnAtPoint(SpawnPoint& point);
    void ProcessWave(WaveConfig& wave);
    void StartRespawnTimer(Entity entity, float delay);
    
    std::vector<SpawnPoint> spawnPoints_;
    std::queue<RespawnEntry> respawnQueue_;
};

// RespawnComponent
struct RespawnComponent {
    uint32_t spawnPointId;
    float respawnTime;
    bool isDead;
};
```

### AI Behaviors

| Behavior | Description |
|----------|-------------|
| idle_standing | Stand in place, look around |
| patrol_route | Follow predefined path |
| wander_radius | Random movement within radius |
| aggression_radius | Attack player within range |
| fleeing | Run away when low HP |

---

## 4. User Stories

### US-001: Zone Spawning
**Description:** As a player, I want to see NPCs in zones so that the world feels alive.

**Acceptance Criteria:**
- [ ] 10+ NPCs per zone spawn on load
- [ ] NPCs in correct positions
- [ ] NPCs perform idle animations
- [ ] Spawn animations play

### US-002: Enemy Respawn
**Description:** As a player, I want defeated enemies to respawn so that I can keep farming.

**Acceptance Criteria:**
- [ ] Enemies respawn after configurable delay
- [ ] Respawn at same location
- [ ] No stacking if player still there

### US-003: Wave Spawning
**Description:** As a player, I want to encounter waves of enemies so that combat feels dynamic.

**Acceptance Criteria:**
- [ ] Wave triggers on zone entry
- [ ] Enemies spawn sequentially with delay
- [ ] Next wave after clear
- [ ] Wave clear triggers rewards

### US-004: NPC Interactions
**Description:** As a player, I want to interact with spawned NPCs so that I can complete quests.

**Acceptance Criteria:**
- [ ] NPCs have interaction prompt
- [ ] Proximity triggers dialogue
- [ ] Shop NPCs open shop

---

## 5. Functional Requirements

- FR-1: Load spawn configs from JSON
- FR-2: Spawn initial NPCs on zone load
- FR-3: Spawn enemies on entity death (respawn timer)
- FR-4: Wave spawning triggers
- FR-5: Despawn on distance exceeded
- FR-6: Spawn point persistence
- FR-7: AI behavior integration

---

## 6. Non-Goals

- No dynamic spawn scaling
- No spawn events (time of day)
- No spawn animations (basic only)
- No spawn debugging UI

---

## 7. Implementation Plan

### Week 1: Data & Core

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Define spawn JSON schema | format defined |
| 3-4 | Entity template system | templates work |
| 5-7 | SpawnSystem core | system works |

### Week 2: Respawn & Waves

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-10 | Respawn logic | respawn works |
| 11-12 | Wave spawning | waves work |
| 13-14 | AI integration | behaviors work |

### Week 3: Content

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Create spawn configs | zone configured |
| 18-19 | Test in editor | spawns work |
| 20-21 | Polish | all working |

---

## 8. Testing Requirements

### Unit Tests
- Spawn point positioning
- Respawn timing
- Wave sequencing

### Integration Tests
- Zone loads → NPCs spawn
- Kill enemy → respawn after delay
- Enter zone → wave triggers

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| LOC | ~500 |
| Skills | C++, C#, JSON |

---

## 10. Open Questions

1. **Q: How many spawn configs?**
   - A: 10 NPC spawns, 20 enemy spawns across 3 zones

2. **Q: AI complexity?**
   - A: Basic patrol + aggression only for MVP

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Define spawn JSON