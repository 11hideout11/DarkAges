# PRD: Settings & Options UI

## Introduction

DarkAges has no settings UI. Players cannot adjust video, audio, controls, or keybindings. This PRD implements basic settings for player customization.

## Goals

- Video settings (quality, resolution)
- Audio settings (volume levels)
- Controls (keybindings)
- Options persistence

## User Stories

### SET-001: Video Settings
**Description:** As a player, I want to adjust graphics quality.

**Acceptance Criteria:**
- [ ] Quality preset (Low/Medium/High)
- [ ] Resolution selection
- [ ] Fullscreen toggle
- [ ] Vsync toggle

### SET-002: Audio Settings
**Description:** As a player, I want to adjust audio volumes.

**Acceptance Criteria:**
- [ ] Master volume slider
- [ ] Music volume slider
- [ ] SFX volume slider
- [ ] Voice volume slider

### SET-003: Controls Settings
**Description:** As a player, I want to customize keybindings.

**Acceptance Criteria:**
- [ ] Movement keys displayed
- [ ] Combat keys displayed
- [ ] Key remapping
- [ ] Reset to defaults

### SET-004: Options Persistence
**Description:** As a player, I want settings to save.

**Acceptance Criteria:**
- [ ] Settings save to file
- [ ] Settings load on startup
- [ ] Per-character settings

## Functional Requirements

- FR-1: SettingsManager singleton
- FR-2: SettingsPanel UI scene
- FR-3: JSON config file
- FR-4: KeyBindingMap

## Technical Considerations

### Settings File Format
```json
{
  "video": {
    "quality": "medium",
    "resolution": [1920, 1080],
    "fullscreen": true
  },
  "audio": {
    "master": 0.8,
    "music": 0.6,
    "sfx": 1.0
  },
  "controls": {
    "forward": "KeyW",
    "attack": "MouseLeft"
  }
}
```

## Non-Goals

- No advanced renderer settings
- No network settings
- No macro system

## Success Metrics

- Settings menu opens
- Settings apply and persist

## Open Questions

- Default keybindings?
- Settings file location?