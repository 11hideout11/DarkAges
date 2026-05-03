# PRD: Audio System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-audio-system.md`, `prd/prd-audio-system-client.md`
- **Client Code**: NONE
- **Priority**: P3 (Polish)

## Implementation Checklist

### Client: AudioManager.cs
Create `src/client/scripts/AudioManager.cs`:
```csharp
public partial class AudioManager : Node
{
    public AudioStreamPlayer3D sfxPlayer;
    public AudioStreamPlayer musicPlayer;
    public AudioStreamPlayer3D voicePlayer;
    
    // SFX events
    public void PlaySFX(string sfxName);
    public void PlaySFX3D(Vector3 worldPosition, string sfxName);
    
    // Music
    public void PlayMusic(string trackName, float fadeTime = 1.0f);
    public void StopMusic(float fadeTime = 1.0f);
    
    // Volume control
    public float masterVolume = 1.0f;
    public float musicVolume = 0.8f;
    public float sfxVolume = 1.0f;
    public float voiceVolume = 1.0f;
}
```

### Audio Assets
Organize in `src/client/assets/audio/`:
```
audio/
├── sfx/
│   ├── ui_click.ogg
│   ├── attack_sword.ogg
│   ├── hit.ogg
│   ├── pickup.ogg
│   ├── quest_complete.ogg
│   └── ...
├── music/
│   ├── title_theme.ogg
│   ├── combat.ogg
│   ├── exploration.ogg
│   └── ...
└── voice/
    └── ...
```

### Integration Points
- **UI.cs**: PlaySFX("ui_click") on button press
- **PlayerCombat**: PlaySFX on attack/hit
- **QuestSystem**: PlaySFX("quest_complete") on quest finish
- **MusicZoneTrigger**: Music changes per zone

### Scene Integration
Add AudioManager to `Main.tscn`:
- Autoload (singleton-like behavior)
- Persist across scene changes

## Acceptance Criteria
- [ ] AudioManager.cs exists
- [ ] UI click sounds play
- [ ] Music plays and fades between zones
- [ ] Volume settings work
- [ ] 3D positional audio for nearby sounds