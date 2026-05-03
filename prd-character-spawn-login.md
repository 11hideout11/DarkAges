# PRD: Character Spawn & Login Flow

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical  
**Category:** Gameplay - Core Loop

---

## 1. Problem Statement

No character creation, spawn, or login flow is implemented. When players launch the game, there's no path to create a character, spawn in the world, or resume playing. The client loads but players can't actually play.

### Current State
- ⚠️ No character creation UI
- ⚠️ No spawn point system
- ⚠️ No login flow
- ⚠️ No server connection UI

### Impact
- Cannot play game
- First-time experience broken
- Missing core onboarding
- Demo unreachable

---

## 2. Goals

### Primary Goals
1. Create character selection/creation UI
2. Implement spawn point selection
3. Build server connection flow
4. Wire character to game world

### Success Criteria
- [ ] Character creation works
- [ ] Character selection works
- [ ] Spawn at correct location
- [ ] Connected to server

---

## 3. Technical Specification

### Character Creation

```csharp
// CharacterCreateUI.cs
public partial class CharacterCreateUI : DarkAgesUI {
    [Export] private LineEdit nameInput;
    [Export] private OptionButton classSelect;
    [Export] private OptionButton appearanceSelect;
    [Export] private TextureRect preview;
    
    [Export] private Label nameError;
    [Export] private Button createButton;
    [Export] private Button backButton;
    
    private const int MIN_NAME_LENGTH = 3;
    private const int MAX_NAME_LENGTH = 16;
    private const string FORBIDDEN_CHARS = "<>[]";
    
    public override void Open() override;
    private void OnClassSelected(int index);
    private void OnCreateClicked();
    private bool ValidateName(string name);
    private void SendCreateRequest();
}

// Character classes
enum CharacterClass {
    Warrior = 1,
    Mage = 2,
    Rogue = 3
}
```

### Character Selection

```csharp
// CharacterSelectUI.cs
public partial class CharacterSelectUI : DarkAgesUI {
    [Export] private VBoxContainer characterList;
    [Export] private Button createNewButton;
    [Export] private Button deleteButton;
    [Export] private Button playButton;
    
    private const int MAX_CHARACTERS = 4;
    
    private void LoadCharacterList();
    private void OnCharacterClicked(int index);
    private void OnPlayClicked();
    private void OnDeleteClicked();
}

// CharacterInfo display
struct CharacterInfo {
    string name;
    CharacterClass classType;
    int level;
    uint32_t zoneId;
    Vector3 position;
    TimeSpan playTime;
}
```

### Login Sequence

```
┌────────────────┐     ┌────────────────┐     ┌────────────────┐
│   MainMenu     │ ──→ │ CharacterSelect│ ──→ │    Loading    │
└────────────────┘     └────────────────┘     └────────────────┘
                            │                     │
                            ↓                     ↓
                     ┌────────────────┐     ┌────────────────┐
                     │ CharacterCreate│     │ GameWorld      │
                     └────────────────┘     └────────────────┘
```

### Server Connection

```csharp
// ConnectionManager.cs
public partial class ConnectionManager : Node {
    private const string DEFAULT_SERVER = "localhost";
    private const int DEFAULT_PORT = 7777;
    
    public void ConnectToServer(string address, int port);
    public void Disconnect();
    public ConnectionState GetState();
    
    private void OnConnected();
    private void OnConnectionFailed();
    private void HandleTimeout();
    
    public event Action OnConnected;
    public event Action<string> OnConnectionFailed;
    
    public enum ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Failed
    }
}

// Login request/response
struct LoginRequest {
    string characterName;
}

struct LoginResponse {
    bool success;
    string errorMessage;
    uint64_t playerId;
    Vector3 spawnPosition;
    uint32_t spawnZone;
}
```

### Spawn Points

```json
{
  "spawn_points": [
    {
      "zone_id": 1,
      "name": "Town Square",
      "position": {"x": 0, "y": 0, "z": 0},
      "is_default": true
    },
    {
      "zone_id": 2,
      "name": "Goblin Camp Entrance",
      "position": {"x": 50, "y": 0, "z": 50},
      "unlock_level": 1
    }
  ]
}
```

---

## 4. User Stories

### US-001: New Character
**Description:** As a player, I want to create a new character so that I can start playing.

**Acceptance Criteria:**
- [ ] Name input with validation
- [ ] Class selection (Warrior/Mage/Rogue)
- [ ] Preview model displays
- [ ] Server confirms creation

### US-002: Character Selection
**Description:** As a player, I want to select an existing character so that I can resume playing.

**Acceptance Criteria:**
- [ ] List shows all characters
- [ ] Shows level/class info
- [ ] Can select and play

### US-003: Server Connection
**Description:** As a player, I want to connect to the server so that I can play multiplayer.

**Acceptance Criteria:**
- [ ] Connect request sent
- [ ] Loading during connect
- [ ] Handle failure gracefully

### US-004: Spawn at Location
**Description:** As a player, I want to spawn at the correct location so that I can begin playing.

**Acceptance Criteria:**
- [ ] Spawn at default point
- [ ] Character position set
- [ ] Zone loads

---

## 5. Functional Requirements

- FR-1: Character creation form validates
- FR-2: Character name sends to server
- Character selection populates from server
- FR-4: Connect request sends on play
- FR-5: Loading screen during connect
- FR-6: Spawn loads character position
- FR-7: Handle connection failure

---

## 6. Non-Goals

- No account creation (single player)
- No cross-server transfer
- No character transfer
- No deleted character recovery

---

## 7. Implementation Plan

### Week 1: Character Creation

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | CharacterCreateUI | form works |
| 3-4 | Name validation | validation works |
| 5-7 | Create flow | creates character |

### Week 2: Selection & Connection

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | CharacterSelectUI | selection works |
| 10-11 | Connection management | connects |
| 12-14 | Login flow | flow complete |

### Week 3: Spawn

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Spawn point loading | spawns work |
| 18-19 | Zone loading | zones load |
| 20-21 | Test and polish | all works |

---

## 8. Testing Requirements

### Integration Tests
- Create → select → spawn → play
- Connection failure handling
- Character persistence

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| LOC | ~500 |
| Skills | C#, Godot UI |

---

## 10. Open Questions

1. **Q: Single or multiplayer first?**
   - A: Single-player with server (local)

2. **Q: Character limit?**
   - A: 4 per account

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Create CharacterCreateUI