# PRD-021: Inventory Equipment System

**Version:** 1.0  
**Date:** 2026-05-02  
**Status:** Proposed  
**Priority:** High  
**Prerequisite:** None - minimal placeholder exists

---

## 1. Problem Statement

The inventory and equipment system has only minimal placeholder implementation:
- ⚠️ No inventory UI
- ⚠️ No equipment slots fully wired
- ⚠️ No item database
- ⚠️ No loot system
- ⚠️ No drag-and-drop

This is critical for gameplay progression and player engagement.

### Current State
- ⚠️ Minimal placeholder only
- ⏸️ Needs full implementation

### Impact
- Players cannot equip items
- No progression depth
- No loot rewards possible
- Missing core RPG loop

---

## 2. Goals

### Primary Goals
1. Implement 7+ equipment slots
2. Create inventory UI with drag-and-drop
3. Build item database (50+ items)
4. Implement loot drops
5. Add tooltips

### Success Criteria
- [ ] Equipment slots work
- [ ] Items can be equipped
- [ ] Inventory UI functional
- [ ] Loot drops work
- [ ] Tooltips display

---

## 3. Technical Specification

### Equipment Slots

```
Equipment Slots:
┌────────────────────────────────────┐
│ HEAD          → Helmet              │
│ CHEST         → Armor/Clothing     │
│ LEGS          → Pants              │
│ FEET          → Boots               │
│ MAIN_HAND     → Weapon             │
│ OFF_HAND      → Shield/Offhand     │
│ ACCESSORY_1   → Ring/Amulet         │
│ ACCESSORY_2   → Ring/Amulet         │
└────────────────────────────────────┘
```

### Data Structures

```csharp
// Item Database Entry
public struct ItemData {
    public uint32_t Id;
    public string Name;
    public string Description;
    public ItemType Type;
    public Rarity Rarity;
    public int Level;
    public int StackSize;
    public int Price;
    public ItemStats Stats;
    public string IconPath;
    public string ModelPath;
}

// Item Stats
public struct ItemStats {
    public int Strength;
    public int Agility;
    public int Vitality;
    public int Intelligence;
    public int Wisdom;
    public int Defense;
    public int Attack;
    public int CriticalRate;
    public int CriticalDamage;
}

// Inventory Slot
public class InventorySlot {
    public uint32_t ItemId;
    public int Quantity;
    public bool IsLocked;
}

// Player Inventory
public class PlayerInventory {
    private const int MAX_SLOTS = 50;
    private InventorySlot[] slots = new InventorySlot[MAX_SLOTS];
    private EquipmentSlot[] equipment = new EquipmentSlot[8];
    
    public bool AddItem(uint32_t itemId, int quantity);
    public bool RemoveItem(uint32_t itemId, int quantity);
    public bool Equip(int slotIndex, int inventoryIndex);
    public bool Unequip(EquipmentSlot slot);
    public InventorySlot GetSlot(int index);
    public EquipmentSlot GetEquipment(EquipmentSlot slot);
}
```

### Inventory UI Layout

```
Inventory UI:
┌────────────────────────────────────────────────────┐
│ [X] Inventory                              [Gold] │
├─────────────────────────────────────┬───────────┤
│  [1]  [2]  [3]  [4]  [5]  [6]     │ WEAPON    │
│  [7]  [8]  [9]  [10] [11] [12]    │ CHEST     │
│  [13] [14] [15] [16] [17] [18]    │ HEAD     │
│  [19] [20] [21] [22] [23] [24]    │ LEGS      │
│  [25] [26] [27] [28] [29] [30]    │ FEET     │
│                                     │ RING     │
│                                     │ AMULET   │
├─────────────────────────────────────┴───────────┤
│ Item: [Name]                    [Equip] [Drop] │
│ Stats: +5 ATK +3 DEF                         │
└──────────────────────────────────────────────┘
```

### Integration Points

| Component | Integration | File |
|-----------|------------|------|
| Player | Add InventoryComponent | Player.cs |
| LootSystem | Spawn loot on death | EnemyDeathSystem.cs |
| ItemTooltip | Show on hover | TooltipUI.cs |
| SaveSystem | Persist inventory | SaveSystem.cs |

---

## 4. Implementation Plan

### Week 1: Core Data

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Define item schema | ItemData structure |
| 3-4 | Create item database (50 items) | items.json |
| 5-6 | Implement InventoryComponent | Component works |
| 7 | Implement EquipmentComponent | Equipment system |

### Week 2: UI & Loot

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Create InventoryPanel UI | UI renders |
| 10-11 | Implement drag-and-drop | Drag works |
| 12 | Add tooltips | Tooltips display |
| 13-14 | Implement loot drops | Loot works |
| 14 | Test full flow | Integration passes |

### Dependencies
- Player.cs (exists)
- prd-inventory-equipment-system.md (exists)

---

## 5. Testing Requirements

### Unit Tests
- Add/remove items
- Equip/unequip
- Stack operations

### Integration Tests
- Full inventory UI flow
- Loot pickup
- Equipment bonuses apply

### Test Metrics
- Item database: 50+ items
- Test coverage: >80%

---

## 6. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 3 weeks |
| LOC | ~500 |
| Skills | C#, UI Design, Godot |

---

## 7. Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| UI complexity | High | Medium | Use existing UI framework |
| Item balance | Medium | Low | Tweak numbers |

---

## 8. Open Questions

1. **Q: Currency system?**
   - A: Gold only for MVP

2. **Q: Auction house?**
   - A: Deferred - trade system first

3. **Q: Crafting?**
   - A: Deferred - see prd-crafting-system.md

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Define item schema