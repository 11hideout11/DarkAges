# PRD: Settings & Options UI

## Introduction

Implement a comprehensive settings and options UI that allows players to configure graphics, audio, controls, UI layout, and social preferences. The system should provide both quick toggle access and a full settings menu.

## Goals

- Create settings menu UI with categories
- Implement graphics options (resolution, quality, vsync)
- Implement audio options (master, music, sfx, voice)
- Implement control rebinding (keyboard, mouse)
- Implement UI customization (HUD positions, scale)
- Implement social options (chat filters, privacy)
- Save and load settings persistence

## User Stories

### US-001: Graphics settings
**Description:** As a player, I want to configure graphics so the game runs well on my hardware.

**Acceptance Criteria:**
- [ ] Resolution selector (native, 720p, 1080p, 1440p, 4K)
- [ ] Quality preset (Low, Medium, High, Ultra)
- [ ] VSync toggle
- [ ] FPS limit option (30, 60, 144, uncapped)
- [ ] Shadow quality slider
- [ ] Particle density slider
- [ ] Apply button for changes

### US-002: Audio settings
**Description:** As a player, I want to configure audio so the game sounds good.

**Acceptance Criteria:**
- [ ] Master volume slider (0-100)
- [ ] Music volume slider
- [ ] SFX volume slider
- [ ] Voice chat volume slider
- [ ] Audio device selector
- [ ] Test sound button

### US-003: Control rebinding
**Description:** As a player, I want to rebind keys so I can use my preferred controls.

**Acceptance Criteria:**
- [ ] Shows current key bindings
- [ ] Click to rebind shows "Press new key..."
- [ ] Resets to defaults option
- [ ] Mouse sensitivity slider
- [ ] Invert Y-axis toggle
- [ ] Key binding preview

### US-004: UI customization
**Description:** As a player, I want to customize the UI so it fits my playstyle.

**Acceptance Criteria:**
- [ ] HUD scale slider
- [ ] Health bar position (top-left, top-center, top-right)
- [ ] Minimap position
- [ ] Show/hide individual elements
- [ ] Chat position (bottom, side)
- [ ] UI scale applies immediately

### US-005: Social settings
**Description:** As a player, I want to configure social options so I control my privacy.

**Acceptance Criteria:**
- [ ] Chat filter (all, party, guild, system)
- [ ] Whisper privacy (everyone, friends, off)
- [ ] Trade requests toggle
- [ ] Party invites toggle
- [ ] Friend requests toggle
- [ ] Ignore list management

## Functional Requirements

- FR-1: SettingsMenu.tscn scene with tab navigation
- FR-2: GraphicsSettingsComponent
- FR-3: AudioSettingsComponent  
- FR-4: ControlSettingsComponent with InputManager
- FR-5: UICustomizationComponent
- FR-6: SocialSettingsComponent
- FR-7: SettingsPersistenceComponent (save to file)

## Technical Considerations

- Godot 4.2 Settings API compatibility
- InputEvent mapping via InputMap
- Settings stored in user:// /settings.json

## Success Metrics

- All settings save and load correctly
- No crashes during settings changes
- Settings apply immediately