# PRD: Death & Respawn System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Gameplay - Core Loop

---

## 1. Problem Statement

When players die, there's no respawn system. Players remain dead, cannot respawn, or respawn incorrectly. This is fundamental to gameplay - death without respawn breaks the core loop.

### Current State
- ⚠️ No death state tracking
- ⚠️ No respawn logic
- ⚠️ No death penalty
- ⚠️ No corpse removal

### Impact
- Game breaks on death
- Cannot continue playing
- No stakes in combat
- Missing core loop

---

## 2. Goals

### Primary Goals
1. Detect player death (HP <= 0)
2. Implement respawn location logic
3. Add death penalties
4. Create respawn UI

### Success Criteria
- [ ] Death detected correctly
- [ ] Respawn at safe location
- [ ] Death penalty applied
- [ ] Respawn UI displays

---

## 3. Technical Specification

### Death Detection

```csharp
// DeathSystem.cs
public partial class DeathSystem : System {
    private void OnPlayerHPChanged(Entity player, float hp) {
        if (hp <= 0 && player.isAlive) {
            PlayerDied(player);
        }
    }
    
    private void PlayerDied(Entity player) {
        // Mark as dead
        Player.SetDead(player);
        
        // Trigger death effects
        DropLoot(player);  // Optional: dropped on death
        ApplyDeathPenalty(player);
        
        // Show respawn UI
        ShowRespawnUI(player);
        
        // Notify party
        if (player.party) {
            NotifyParty(player, "Player died");
        }
    }
}

// PlayerComponent additions
struct PlayerComponent {
    bool isAlive;
    Entity corpse;  // If we implement corpses
    int deaths;
    timestamp lastDeathTime;
}
```

### Respawn Locations

```cpp
// RespawnPointSystem.hpp
enum class RespawnType {
    SafeZone,      // Nearest safe spawn
    BindPoint,     // Bound location
    Cemetery,     // Ghost area
    Instance       // Instance entry
};

struct RespawnPoint {
    uint32_t zoneId;
    Vector3 position;
    RespawnType type;
    bool isDefault;
};

class RespawnPointSystem {
public:
    RespawnPoint GetRespawnPoint(Entity player);
    void SetBindPoint(Entity player, uint32_t zoneId, Vector3 position);
    
private:
    std::vector<RespawnPoint> GetZoneRespawnPoints(uint32_t zoneId);
    RespawnPoint GetNearestSafeSpawn(Vector3 deathPosition);
    
    std::unordered_map<uint32_t, std::vector<RespawnPoint>> zoneRespawns_;
};
```

### Death Penalties

```cpp
// DeathPenalty system
void ApplyDeathPenalty(Entity player) {
    // XP loss - 10% of current level
    int xpLoss = CalculateXPForLevel(player.level) * 0.10;
    Player.AddXP(player, -xpLoss);
    
    // Chance to lose gold
    if (Rand() < 0.25) {
        int goldLoss = player.gold * 0.05;  // 5%
        player.gold -= goldLoss;
    }
    
    // Equipment durability (if implemented)
    // Equipment.ApplyDamage(player.equipment, 10);
    
    // Stat penalty (temporary)
    ApplyDebuff(player, "death_weakness", 60sec);
}
```

### Respawn UI

```csharp
// RespawnUI.cs
public partial class RespawnUI : Control {
    [Export] private Label deathMessage;
    [Export] private Label respawnTimer;
    [Export] private Button respawnButton;
    [Export] private Button bindButton;
    [Export] private Label penaltyWarning;
    
    private const float RESPAWN_DELAY = 5.0f;
    
    public void Show() {
        // Show countdown
        // Show penalty info
        // Enable respawn button after delay
        
        // Options:
        // "Respawn at Safe Zone"
        // "Respawn at Bind Point" (if unlocked)
    }
    
    private void OnRespawnClicked() {
        var respawn = RespawnSystem.GetRespawnPoint(player);
        Player.Teleport(respawn.position, respawn.zoneId);
    }
}
```

### Flow

```
Attack → HP=0 → Death Animation → UI Shows (5s)
                    → XP Penalty Applied
                    → Player Chooses
                         ↓
              Respawn at Safe Point
                         ↓
              Health Restored (50%)
```

---

## 4. User Stories

### US-001: Death Detection
**Description:** As a player, I want to die when HP reaches zero so that combat has stakes.

**Acceptance Criteria:**
- [ ] Player dies when HP <= 0
- [ ] Death animation plays
- [ ] Controls disabled
- [ ] Party notified

### US-002: Respawn Options
**Description:** As a player, I want to choose my respawn location so that I can return efficiently.

**Acceptance Criteria:**
- [ ] Safe zone respawn available
- [ ] Bind point option (if bound)
- [ ] Timer runs during selection

### US-003: Death Penalty
**Description:** As a player, I want consequences for dying so that I play carefully.

**Acceptance Criteria:**
- [ ] XP penalty applied
- [ ] Potential gold loss
- [ ] Temporary stat reduction
- [ ] Death counter increments

### US-004: Corpse Run
**Description:** As a player, I want to retrieve dropped items so that I can minimize losses.

**Acceptance Criteria:**
- [ ] Corpse spawns at death
- [ ] Can retrieve on return
- [ ] Corpse despawns after timeout

---

## 5. Functional Requirements

- FR-1: Detect HP <= 0 as death
- FR-2: Play death animation
- FR-3: Show respawn UI
- FR-4: Apply XP penalty (10%)
- FR-5: Optional gold loss (25% chance)
- FR-6: Teleport to spawn on respawn
- FR-7: Restore HP to 50%
- FR-8: Clear death state on respawn

---

## 6. Non-Goals

- No corpse runs (deferred)
- No item loss on death (deferred)
- No durability loss (deferred)
- No revive mechanic (deferred)

---

## 7. Implementation Plan

### Week 1: Death Logic

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Death detection | system works |
| 3-4 | Death animation | anim works |
| 5-7 | State management | death state works |

### Week 2: Respawn

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Respawn locations | location works |
| 10-11 | Respawn UI | UI works |
| 12-14 | Teleport logic | teleports work |

### Week 3: Penalties

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | XP penalty | penalty works |
| 18-19 | Gold/soul penalty | penalty works |
| 20-21 | Test and polish | all works |

---

## 8. Testing Requirements

### Unit Tests
- Death detection threshold
- XP penalty calculation
- Respawn location selection

### Integration Tests
- HP=0 → death state → respawn → continue

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Low |
| Time | 1-2 weeks |
| LOC | ~300 |
| Skills | C#, Godot |

---

## 10. Open Questions

1. **Q: Corpse runs required?**
   - A: No, MVP can skip

2. **Q: Respawn timer length?**
   - A: Instant or 5s countdown

---

**Status:** ✅ Complete — DeathRespawnUI.cs exists with styled respawn system (UITheme.cs integration)
**Author:** OpenHands Analysis  
**Next Step:** Death detection logic