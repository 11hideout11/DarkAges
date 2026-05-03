# PRD: NPC Dialogue System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High  
**Category:** Gameplay - Player Progression

---

## 1. Problem Statement

NPC dialogue system is required to enable quest-giving, shop interactions, and story progression. While dialogue files may exist, there's no integrated implementation connecting server dialogue logic to client UI display.

### Current State
- ⚠️ Dialogue data format exists in data/quests.json (embedded dialogue)
- ⚠️ No stand-alone dialogue system component
- ⚠️ Server-to-client dialogue RPC not wired

### Impact
- Players cannot interact with NPCs
- Quests cannot have dialogue branches
- Shops display no interaction UI
- Missing core RPG interaction loop

---

## 2. Goals

### Primary Goals
1. Create dialogue data format (JSON)
2. Implement DialogueComponent for NPCs
3. Build DialogueUI for client display
4. Wire server dialogue events to client RPC

### Success Criteria
- [ ] Dialogue data loads from JSON
- [ ] NPCs display greeting when approached
- [ ] Dialogue tree navigable with choices
- [ ] Quest handoff via dialogue works

---

## 3. Technical Specification

### Dialogue Tree Structure

```json
{
  "dialogues": [
    {
      "id": "npc_merchant_01",
      "npcId": 1001,
      "rootNode": "greeting",
      "nodes": {
        "greeting": {
          "text": "Welcome, traveler! What can I do for you?",
          "options": [
            {"text": "Show me your wares", "next": "shop", "action": "open_shop"},
            {"text": "Never mind", "next": "farewell"}
          ]
        },
        "shop": {
          "text": "Take a look at my inventory.",
          "action": "open_shop"
        },
        "farewell": {
          "text": "Safe travels!"
        }
      }
    }
  ]
}
```

### Server Components

```cpp
// DialogueComponent.hpp
struct DialogueComponent {
    uint32_t dialogueId;
    std::string currentNode;
    std::vector<std::string> completedNodes;
};

// DialogueSystem.hpp
class DialogueSystem {
public:
    void ShowDialogue(Entity npc, Entity player);
    void SelectOption(Entity player, uint32_t optionIndex);
    void CloseDialogue(Entity player);
private:
    std::unordered_map<uint32_t, DialogueData> dialogues_;
};
```

### Client Implementation

```csharp
// DialogueUI.cs
public partial class DialogueUI : Control {
    [Export] private Label npcNameLabel;
    [Export] private Label dialogueText;
    [Export] private VBoxContainer optionContainer;
    
    public void ShowDialogue(DialogueData data);
    public void OnOptionSelected(int index);
    public void Hide();
}
```

### Integration Points

| Component | Integration | File |
|-----------|------------|------|
| QuestSystem | Quest handoff via dialogue action | QuestSystem.cpp |
| ShopSystem | Open shop via dialogue action | ItemSystem.cpp |
| NetworkManager | RPC dialogue events | NetworkManager.cpp |

---

## 4. User Stories

### US-001: NPC Greeting
**Description:** As a player, I want NPCs to display a greeting when I approach them so that I know I can interact.

**Acceptance Criteria:**
- [ ] Proximity trigger activates at 3_unit distance
- [ ] Greeting dialogue displays automatically
- [ ] Can dismiss with ESC or clicking away

### US-002: Dialogue Branching
**Description:** As a player, I want to choose dialogue options so that I can navigate conversations.

**Acceptance Criteria:**
- [ ] 2-4 options displayed per node
- [ ] Clicking option advances to next node
- [ ] Choice history tracked

### US-003: Quest Handoff
**Description:** As a player, I want to receive quests through dialogue so that quest flow feels natural.

**Acceptance Criteria:**
- [ ] "Accept Quest" action implemented
- [ ] Quest appears in quest log after dialogue
- [ ] Dialogue updates to "quest active" branch

---

## 5. Functional Requirements

- FR-1: Load dialogue data from `data/dialogues.json`
- FR-2: Display dialogue UI when player enters NPC proximity
- FR-3: Navigate dialogue tree via option selection
- FR-4: Execute dialogue actions (give item, start quest, open shop)
- FR-5: Close dialogue on ESC key or distance exceeded
- FR-6: Sync dialogue state server-to-client via RPC

---

## 6. Non-Goals

- No voiced dialogue (text only for MVP)
- No reputation system integration
- No animated portrait display
- No dialogue recording/playback

---

## 7. Implementation Plan

### Week 1: Data & Server

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Define dialogue JSON schema | dialogues.json |
| 3-4 | Implement DialogueComponent | Component compiles |
| 5-7 | Implement DialogueSystem | System wired to ECS |

### Week 2: Client UI

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-9 | Create DialogueUI scene | UI renders |
| 10-11 | Wire RPC to dialogue events | Events fire |
| 12-14 | Test full flow | Integration passes |

---

## 8. Testing Requirements

### Unit Tests
- Dialogue tree parsing
- Option navigation
- Action execution

### Integration Tests
- Player approaches NPC → dialogue opens
- Complete dialogue → quest received
- Distance exceeded → dialogue closes

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 2 weeks |
| LOC | ~400 (server) + ~300 (client) |
| Skills | C++, C#, Godot UI |

---

## 10. Open Questions

1. **Q: How many NPCs need dialogue?**
   - A: 10 for MVP (3 merchants, 5 quest givers, 2 generic)

2. **Q: Support quest chains?**
   - A: Deferred - single quest per NPC for MVP

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Define dialogue JSON schema