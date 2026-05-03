# PRD: Client Settings UI - Implementation

## Status
**PARTIALLY IMPLEMENTED - NEEDS EXPANSION**

## Gap Analysis
- **PRD File**: `prd/prd-settings-ui.md`, `prd/prd-client-settings-ui.md`
- **Client Code**: PARTIAL - basic UI.tscn
- **Priority**: P3 (Polish)

## Implementation Checklist

### Client: SettingsPanel.tscn Expansion
Extend `src/client/scenes/UI.tscn` or create new `src/client/scenes/SettingsPanel.tscn`:

```
Control (SettingsPanel)
├── TabContainer
│   ├── GraphicsTab
│   │   ├── ResolutionDropdown
│   │   ├── QualityDropdown (Low/Medium/High/Ultra)
│   │   ├── VSyncCheckbox
│   │   ├── SSAOCheckbox
│   │   ├── SDFGICheckbox
│   │   └── FPSLimitSlider
│   ├── AudioTab
│   │   ├── MasterVolumeSlider (0-100)
│   │   ├── MusicVolumeSlider
│   │   ├── SFXVolumeSlider
│   │   └── VoiceVolumeSlider
│   ├── ControlsTab
│   │   ├── MouseSensitivitySlider
│   │   ├── InvertYCheckbox
│   │   ├── KeybindList (scrollable)
│   │   └── ResetDefaultsButton
│   └── NetworkTab
│       ├── ServerAddressInput
│       ├── PingDisplay
│       └── NetworkStatsLabel
└── ApplyButton
└── CancelButton
```

### Client: SettingsManager.cs
Extend or create `src/client/scripts/SettingsManager.cs`:
```csharp
public partial class SettingsManager : Node
{
    public GraphicsSettings graphics = new();
    public AudioSettings audio = new();
    public ControlSettings controls = new();
    
    public void LoadSettings(); // From user://settings.cfg
    public void SaveSettings(); // To user://settings.cfg
    public void ApplyGraphics();
    public void ApplyAudio();
    public void ResetToDefaults();
}
```

### Settings Storage
- Location: `user://settings.cfg` (JSON)
- Load on game start
- Save on Apply or Exit

## Acceptance Criteria
- [ ] Graphics tab with resolution/quality
- [ ] Audio tab with volume sliders
- [ ] Controls tab with sensitivity
- [ ] Settings persist across restarts
- [ ] Reset to defaults works