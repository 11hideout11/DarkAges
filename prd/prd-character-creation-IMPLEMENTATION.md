# PRD: Character Creation - Implementation

## Status
**PARTIALLY IMPLEMENTED - NEEDS EXPANSION**

## Gap Analysis
- **PRD File**: `prd/prd-character-creation-ui.md`
- **Client Code**: PARTIAL - minimal
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Client: CharacterCreator.tscn
Create `src/client/scenes/CharacterCreator.tscn`:
```
Control (CharacterCreator)
├── Background (TextureRect)
├── NameInput (LineEdit)
│   └── NameValidationLabel
├── ClassSelector (HBoxContainer)
│   ├── WarriorButton (class icon)
│   ├── MageButton
│   ├── RogueButton
│   └── RangerButton
├── AppearancePanel (VBoxContainer)
│   ├── FaceSelector (GridContainer)
│   ├── HairSelector  
│   └── ColorPicker
├── StatsPreview (VBoxContainer)
│   ├── StrengthValue
│   ├── AgilityValue
│   └── IntelligenceValue
├── CreateButton
└── BackButton
```

### Client: CharacterCreator.cs
Create `src/client/scripts/CharacterCreator.cs`:
```csharp
public partial class CharacterCreator : Control
{
    [Export] private LineEdit nameInput;
    [Export] private Button[] classButtons;
    [Export] private Control appearancePanel;
    
    // Class data from data/classes.json
    public string selectedClass;
    public Vector2 appearance;
    
    public void OnClassSelected(string className);
    public bool ValidateName(string name); // 3-16 chars, alphanumeric
    public void CreateCharacter();
    public void OnBackPressed();
}
```

### Integration Points
- LoginScreen: Button to open CharacterCreator
- NetworkManager: PACKET_CHARACTER_CREATE for server
- CharacterSelection: Refresh list after create

### Data Integration
Classes defined in JSON (create if not exists):
```json
{
  "classes": [
    {"id": "warrior", "name": "Warrior", "stats": {"str": 10, "agi": 5, "int": 2}},
    {"id": "mage", "name": "Mage", "stats": {"str": 2, "agi": 5, "int": 10}},
    ...
  ]
}
```

## Acceptance Criteria
- [ ] UI displays with name input
- [ ] Class selection works
- [ ] Name validation (3-16 alphanumeric)
- [ ] Stats preview updates on class change
- [ ] Character created and appears in selection