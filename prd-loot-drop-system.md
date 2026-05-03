# PRD: Loot Drop System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Gameplay - Player Progression

---

## 1. Problem Statement

When enemies die, players should receive loot (items, gold, equipment) as rewards. Currently, there's no loot system - enemies die but drop nothing. This is critical for player progression and engagement.

### Current State
- ⚠️ Enemy death events exist
- ⚠️ No loot drop logic
- ⚠️ Item definitions exist
- ⚠️ No pickup UI

### Impact
- No rewards for defeating enemies
- No progression loop
- Players outgrow content quickly
- Missing core RPG loop

---

## 2. Goals

### Primary Goals
1. Create loot tables for enemies
2. Implement drop logic (RNG, rarity)
3. Create loot drop entities (world items)
4. Build pickup interaction

### Success Criteria
- [ ] Enemies drop items on death
- [ ] Gold drops work
- [ ] Loot appears as ground items
- [ ] Player can pickup loot

---

## 3. Technical Specification

### Loot Table Structure

```json
{
  "loot_tables": {
    "goblin": {
      "gold": {"min": 5, "max": 25, "chance": 100},
      "items": [
        {"itemId": 101, "dropChance": 10, "minQty": 1, "maxQty": 2},
        {"itemId": 201, "dropChance": 5, "minQty": 1, "maxQty": 1}
      ]
    },
    "boss_treasure": {
      "gold": {"min": 100, "max": 500, "chance": 100},
      "items": [
        {"itemId": 301, "dropChance": 100, "minQty": 1, "maxQty": 1},
        {"itemId": 401, "dropChance": 50, "minQty": 1, "maxQty": 1}
      ]
    }
  }
}
```

### Drop Types

| Type | Behavior |
|------|---------|
| Gold | Instant pickup, adds to player gold |
| Item | Drops as ground entity, click to pickup |
| Equipment | Same as item, rare drop |
| Quest Item | Same as item, unique per quest |

### Server Components

```cpp
// LootSystem.hpp
class LootSystem {
public:
    void Initialize();
    void OnEnemyKilled(Entity enemy, Entity killer);
    
private:
    LootDrop RollLoot(const LootTable& table);
    void SpawnLootDrop(Entity killer, const LootDrop& drop);
    bool ShouldDropGold(const LootTable& table);
    int RollGold(const GoldDrop& config);
    
    std::unordered_map<std::string, LootTable> tables_;
};

// LootDropComponent
struct LootDropComponent {
    uint32_t itemId;
    int quantity;
    float lifetime;  // Despawn time
    Entity owner;   # Who can pickup (party shared)
};
```

### Client Components

```csharp
// WorldItem.cs
public partial class WorldItem : RigidBody3D {
    [Export] private uint32_t itemId;
    [Export] private int quantity;
    [Export] private float despawnTime;
    
    public void SetItem(uint32_t id, int qty);
    private void OnBodyEntered(Node body) {
        if (body is Player) {
            AttemptPickup(body as Player);
        }
    }
}

// LootPickupPrompt.cs  
public partial class LootPickupPrompt : Control {
    [Export] private Label itemName;
    [Export] private Label quantity;
    [Export] private KeyLabel pickupKey;
    
    public void Show(WorldItem item);
    public void Hide();
}
```

### Drop Rules

```cpp
// Rarity distribution
enum Rarity { Common=70, Uncommon=25, Rare=4, Epic=0.9, Legendary=0.1 };

// Player level bonus (+1 item per 5 levels over enemy)
int GetDropBonus(Entity player, Entity enemy) {
    int diff = player.level - enemy.level;
    return std::max(0, diff / 5);
}

// Party sharing
void DistributeLoot(Entity party, const LootDrop& drop) {
    for (auto member : party.members) {
        // Round-robin or need/greed roll
    }
}
```

---

## 4. User Stories

### US-001: Enemy Loot Drop
**Description:** As a player, I want enemies to drop items so that I can get rewards.

**Acceptance Criteria:**
- [ ] Common enemies drop gold (100% chance)
- [ ] Common enemies drop items (10-30% chance)
- [ ] Bosses drop guaranteed rewards
- [ ] Drop quantity varies

### US-002: Ground Item Display
**Description:** As a player, I want to see dropped items on the ground so that I know what's available.

**Acceptance Criteria:**
- [ ] Items appear at enemy death location
- [ ] Items have 3D representation
- [ ] Items glow or shimmer
- [ ] Items despawn after timeout

### US-003: Loot Pickup
**Description:** As a player, I want to pick up dropped items so that I can add them to my inventory.

**Acceptance Criteria:**
- [ ] Click item to pickup
- [ ] Auto-pickup gold on contact
- [ ] Prompt shows "Press E to pickup"
- [ ] Item added to inventory

### US-004: Loot Auto-Pickup
**Description:** As a player, I want gold to auto-collect so that I don't have to pickup each coin.

**Acceptance Criteria:**
- [ ] Gold auto-pickups within radius
- [ ] Radius configurable
- [ ] Animation on pickup

---

## 5. Functional Requirements

- FR-1: Load loot tables from JSON
- FR-2: Roll RNG for each drop entry
- FR-3: Spawn world item entities on drop
- FR-4: Apply player level bonus
- FR-5: Despawn drops after 60 seconds
- FR-6: Pickup via E key or click
- FR-7: Gold auto-pickups within 3 units
- FR-8: Party loot sharing rules

---

## 6. Non-Goals

- No loot trading between players
- No loot auction house
- No rare spawn mechanics
- No treasure map system

---

## 7. Implementation Plan

### Week 1: Server Logic

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Define loot table JSON | format defined |
| 3-4 | Implement LootSystem | system works |
| 5-7 | Wire to death events | drops fire |

### Week 2: Client Display

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Create WorldItem scene | item displays |
| 10-11 | Create ground item visuals | items visible |
| 12-14 | Create pickup prompt | UI works |

### Week 3: Integration

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Implement pickup logic | pickup works |
| 18-19 | Gold auto-pickup | gold works |
| 20-21 | Test full flow | integration works |

---

## 8. Testing Requirements

### Unit Tests
- Loot table RNG rolls
- Level bonus calculation
- Despawn timing

### Integration Tests
- Enemy dies → loot spawns
- Pickup item → added to inventory
- Gold pickup → gold increases

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| LOC | ~400 (server) + ~300 (client) |
| Skills | C++, C#, Godot |

---

## 10. Open Questions

1. **Q: How many loot tables?**
   - A: 15 for MVP (5 enemy types, 5 zones, 5 bosses)

2. **Q: Drop rate for testing?**
   - A: 100% for demo, tune later

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Define loot table JSON