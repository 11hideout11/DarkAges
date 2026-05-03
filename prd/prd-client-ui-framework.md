# PRD: Client UI Framework

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical  
**Category:** Client - Foundation

---

## 1. Problem Statement

The client lacks a unified UI framework to display inventory, quests, abilities, and other player-facing interfaces. Without this, players cannot interact with progression systems.

### Current State
- ⚠️ No base UI framework
- ⚠️ Individual UI尝试 exist but disconnected
- ⚠️ No navigation or consistency
- ⚠️ Control scheme undefined

### Impact
- Players cannot see inventory
- No quest log display
- No ability bar
- Missing core UX for gameplay

---

## 2. Goals

### Primary Goals
1. Create base UI framework (panels, navigation)
2. Implement HUD with hotbars
3. Build options/settings menu
4. Wire all UI to game state

### Success Criteria
- [ ] All UI panels functional
- [ ] Consistent visual style
- [ ] Keyboard/mouse navigation works
- [ ] UI responds to game state

---

## 3. Technical Specification

### UI Architecture

```
Main.tscn
├── HUD (CanvasLayer)
│   ├── TopBar (gold, exp, level)
│   ├── Hotbar (10 slots)
│   ├── MiniMap
│   └── CombatTextAnchor
├── Screens (Control)
│   ├── InventoryPanel
│   ├── QuestLogPanel
│   ├── AbilitiesPanel
│   ├── CharacterPanel
│   ├── SocialPanel
│   └── SettingsPanel
└── Overlays (Control)
    ├── DialogueOverlay
    ├── ShopOverlay
    └── TooltipOverlay
```

### Base Classes

```csharp
// DarkAgesUI.cs - Base class for all panels
public abstract partial class DarkAgesUI : Control {
    protected bool isOpen;
    protected AnimationPlayer animPlayer;
    
    public virtual void Open();
    public virtual void Close();
    public virtual void Toggle();
    protected virtual void OnOpen() {}
    protected virtual void OnClose() {}
}

// InventoryPanel.cs
public partial class InventoryPanel : DarkAgesUI {
    [Export] private GridContainer itemGrid;
    [Export] private VBoxContainer equipmentSlots;
    [Export] private Label goldLabel;
    [Export] private TooltipPanel tooltip;
    
    public override void Open() override;
    public void Refresh();
    private void OnItemSlotClicked(int slot);
    private void OnEquipmentSlotClicked(EquipmentSlot slot);
}

// Hotbar.cs
public partial class Hotbar : HBoxContainer {
    private const int SLOT_COUNT = 10;
    [Export] private Texture2D[] abilityIcons;
    [Export] private KeyLabel[] keyLabels;
    
    public void SetSlot(int index, AbilityData ability);
    public void ClearSlot(int index);
    private void OnSlotClicked(int index);
}
```

### Control Scheme

| Key | Action |
|-----|--------|
| I | Toggle Inventory |
| Q | Toggle Quest Log |
| K | Toggle Abilities |
| C | Toggle Character |
| O | Toggle Options |
| ESC | Close all panels |
| 1-0 | Hotbar slots |
| Tab | Cycle hotbar page |

### Theme/Style

```
Colors:
- Background: #1A1A2E (dark blue)
- Panel: #16213E (navy)
- Accent: #E94560 (crimson)
- Text: #EAEAEA (off-white)
- Gold: #FFD93D (gold)

Fonts:
- Headers: Bold, 24px
- Body: Regular, 16px
- Hotkeys: Bold, 14px

Effects:
- Panel: subtle border glow on hover
- Buttons: scale 1.05 on hover
- Transitions: 200ms ease-out
```

---

## 4. User Stories

### US-001: Hotbar Display
**Description:** As a player, I want to see my abilities on a hotbar so that I can quickly cast them.

**Acceptance Criteria:**
- [ ] 10 slots displayed at bottom of screen
- [ ] Number keys 1-0 trigger slots
- [ ] Cooldown overlay shows remaining time
- [ ] Empty slots show "—" placeholder

### US-001: Hotbar Display
**Description:** As a player, I want to see my abilities on a hotbar so that I can quickly cast them.

**Acceptance Criteria:**
- [ ] 10 slots displayed at bottom of screen
- [ ] Number keys 1-0 trigger slots
- [ ] Cooldown overlay shows remaining time
- [ ] Empty slots show "—" placeholder

### US-002: Inventory Panel
**Description:** As a player, I want to view my inventory so that I can manage my items.

**Acceptance Criteria:**
- [ ] Opens with I key or click inventory icon
- [ ] 24-slot grid displays items
- [ ] Equipment slots on right panel
- [ ] Gold count displayed
- [ ] Close with ESC or X button

### US-003: Quest Log
**Description:** As a player, I want to see my active quests so that I can track objectives.

**Acceptance Criteria:**
- [ ] Opens with Q key
- [ ] Lists all active quests
- [ ] Shows objective progress (e.g., "0/10 rats")
- [ ] Completed quests in separate tab

### US-004: Character Panel
**Description:** As a player, I want to view my character stats so that I know my progress.

**Acceptance Criteria:**
- [ ] Opens with C key
- [ ] Shows level, HP, MP, EXP
- [ ] Shows attributes (STR, AGI, VIT, INT, WIS)
- [ ] Shows equipment bonuses
- [ ] Displays title/class

### US-005: Settings Menu
**Description:** As a player, I want to adjust settings so that I can customize my experience.

**Acceptance Criteria:**
- [ ] Opens with O key
- [ ] Volume sliders (master, sfx, music)
- [ ] Graphics quality dropdown
- [ ] Key binding display
- [ ] Save settings to disk

---

## 5. Functional Requirements

- FR-1: All panels inherit from DarkAgesUI base class
- FR-2: ESC closes any open panel
- FR-3: Only one panel open at a time (except HUD elements)
- FR-4: Panel state persists across scene changes
- FR-5: Tooltips show on hover for inventory items
- FR-6: Keyboard navigation via Tab/Shift+Tab
- FR-7: Mouse click-through to game world when panel open

---

## 6. Non-Goals

- No chat UI (existing)
- No party UI (existing)
- No guild UI (existing)
- No achievement UI (deferred)
- No mail UI (deferred)

---

## 7. Implementation Plan

### Week 1: Framework

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | Create DarkAgesUI base class | Base works |
| 3-4 | Create Main HUD layout | HUD displays |
| 5-7 | Create Hotbar with animation | Hotbar works |

### Week 2: Panels

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-10 | Create Inventory Panel | Inventory works |
| 11-12 | Create Quest Log | Quests display |
| 13-14 | Create Character Panel | Stats display |

### Week 3: Settings

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | Create Settings Menu | Settings save |
| 18-21 | Polish and test | All panels working |

---

## 8. Data Flow

```
Server State → NetworkManager → GameState.cs → UI Components
                    ↓
            InventoryComponent → InventoryPanel
                    ↓
              QuestComponent → QuestLogPanel
                    ↓
           AbilitiesComponent → Hotbar
```

---

## 9. Testing Requirements

### Visual Tests
- All panels render at correct positions
- Animations play smoothly (60fps)
- Fonts render clearly

### Functional Tests
- All keybinds work
- Panel open/close sequences correct
- Tooltips display with correct data

---

## 10. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Medium |
| Time | 3 weeks |
| LOC | ~800 (C#) |
| Skills | C#, Godot UI, UX design |

---

## 11. Open Questions

1. **Q: Resolution support?**
   - A: 1920x1080 primary, scale UI for others

2. **Q: UI scaling options?**
   - A: 100%, 125%, 150% options

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Create DarkAgesUI base class