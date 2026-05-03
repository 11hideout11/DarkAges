# PRD: Save/Load System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Infrastructure - Persistence

---

## 1. Problem Statement

Players cannot save their progress or load previous sessions. For a game with progression systems (XP, inventory, quests), saves are essential. Without save/load, every session starts from zero.

### Current State
- ⚠️ No save system implementation
- ⚠️ PlayerComponent exists but no serialization
- ⚠️ No file-based storage
- ⚠️ No cloud save option

### Impact
- No progression persistence
- Cannot save settings
- Demo resets each session
- Missing core feature

---

## 2. Goals

### Primary Goals
1. Implement player data serialization
2. Create save file format
3. Build save/load UI
4. Add settings persistence

### Success Criteria
- [ ] Player progress saves
- [ ] Player progress loads
- [ ] Settings persist
- [ ] Multiple save slots

---

## 3. Technical Specification

### Save Data Structure

```json
{
  "save_version": 1,
  "player": {
    "player_id": "uuid",
    "name": "PlayerName",
    "level": 10,
    "experience": 5000,
    "gold": 1500,
    "position": {"x": 10.5, "y": 0, "z": 20.3},
    "zone_id": 1
  },
  "inventory": [
    {"slot": 0, "itemId": 101, "quantity": 1},
    {"slot": 1, "itemId": 201, "quantity": 5},
  ],
  "equipment": {
    "main_hand": 301,
    "chest": 201,
    "head": null
  },
  "abilities": [1, 2, 3, 5],
  "quests": [
    {"id": 1, "status": "active", "progress": {"killed": 5, "total": 10}},
    {"id": 2, "status": "completed"}
  ],
  "stats": {
    "strength": 10,
    "agility": 8,
    "vitality": 12,
    "intelligence": 5
  },
  "saved_at": "2026-05-03T12:00:00Z"
}
```

### Save System

```csharp
// SaveManager.cs
public partial class SaveManager : Node {
    private const int MAX_SAVE_SLOTS = 3;
    private const string SAVE_FOLDER = "saves/";
    private const string SETTINGS_FILE = "user://settings.json";
    
    public bool SaveGame(int slotIndex = 0);
    public bool LoadGame(int slotIndex = 0);
    public void DeleteSave(int slotIndex = 0);
    public SaveSlotInfo[] GetSaveSlots();
    public bool HasSaveData();
    
    private string GetSavePath(int slot);
    private string SerializePlayer();
    private PlayerSaveData DeserializePlayer(string json);
}

// SaveSlotInfo.cs
public partial class SaveSlotInfo {
    public int slotIndex;
    public string playerName;
    public int level;
    public DateTime savedAt;
    public TimeSpan playTime;
    public string zoneName;
    public Texture2D screenshot;
}
```

### Settings Format

```json
{
  "graphics": {
    "quality": "medium",
    "resolution": {"width": 1920, "height": 1080},
    "fullscreen": true,
    "vsync": true
  },
  "audio": {
    "master_volume": 0.8,
    "sfx_volume": 0.7,
    "music_volume": 0.5,
    "mute": false
  },
  "controls": {
    "mouse_sensitivity": 1.0,
    "invert_y": false
  }
}
```

---

## 4. User Stories

### US-001: Manual Save
**Description:** As a player, I want to manually save my progress so that I don't lose advancement.

**Acceptance Criteria:**
- [ ] Save button in menu
- [ ] Save to selected slot
- [ ] Confirmation shown
- [ ] Auto-increment slot if full

### US-002: Load Game
**Description:** As a player, I want to load a previous save so that I can resume playing.

**Acceptance Criteria:**
- [ ] Load menu shows save slots
- [ ] Preview shows level/save time
- [ ] Load restores position/items
- [ ] Warning for overwriting current

### US-003: Auto-Save
**Description:** As a player, I want the game to save automatically so that I don't lose progress on crash.

**Acceptance Criteria:**
- [ ] Auto-save every 10 minutes
- [ ] Auto-save on level up
- [ ] Auto-save before exiting
- [ ] Keep last 3 autosaves

### US-004: Settings Save
**Description:** As a player, I want my settings to persist so that I don't reconfigure each session.

**Acceptance Criteria:**
- [ ] Graphics settings save
- [ ] Audio settings save
- [ ] Controls save
- [ ] Settings load on start

---

## 5. Functional Requirements

- FR-1: Serialize all player data to JSON
- FR-2: Write save file to disk
- FR-3: Read save file from disk
- FR-4: Handle deserialization errors
- FR-5: Support multiple save slots
- FR-6: Settings save to user://
- FR-7: Settings load on startup

---

## 6. Non-Goals

- No cloud save (deferred)
- No cross-platform save
- No save editor
- No save transfer

---

## 7. Implementation Plan

### Week 1: Serialization

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Define save format | format defined |
| 3-4 | Serialization code | serialization works |
| 5-7 | File I/O | writes to disk |

### Week 2: UI & Slots

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-10 | Save menu UI | menu works |
| 11-12 | Load menu UI | selection works |
| 13-14 | Multiple slots | 3 slots work |

### Week 3: Auto-Save

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Auto-save triggers | autosave works |
| 18-19 | Settings persistence | settings work |
| 20-21 | Test and polish | all works |

---

## 8. Testing Requirements

### Unit Tests
- Serialization round-trip
- File I/O
- Settings persistence

### Integration Tests
- Save → load → verify data matches
- Multiple slot persistence

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Low |
| Time | 2 weeks |
| LOC | ~300 |
| Skills | C#, Godot file I/O |

---

## 10. Open Questions

1. **Q: Compress saves?**
   - A: No for MVP

2. **Q: Encryption?**
   - A: No for MVP (local only)

---

**Status:** ✅ Complete — SaveManager.cs exists with save/load capability in client codebase
**Author:** OpenHands Analysis  
**Next Step:** Define save format