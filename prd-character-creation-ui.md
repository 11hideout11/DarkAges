# PRD: Character Creation & Selection UI

## Introduction

Implement a character creation and selection UI that allows players to create new characters, customize appearance, select from existing characters, and manage character slots. The system should handle multiple characters per account with persistence.

## Goals

- Create character selection screen showing all characters
- Implement character creation with appearance options
- Add character deletion with confirmation
- Show character details (level, class, playtime)
- Handle maximum character slots (8 per account)
- Integrate with AccountComponent and CharacterComponent

## User Stories

### US-001: Character selection
**Description:** As a player, I want to select my character so I can play.

**Acceptance Criteria:**
- [ ] Shows character portraits in grid
- [ ] Click character to select, shows details
- [ ] "Enter World" button to login
- [ ] Character level, class displayed
- [ ] Last login timestamp shown
- [ ] Create new if slots available

### US-002: Character creation
**Description:** As a new player, I want to create a character so I can start playing.

**Acceptance Criteria:**
- [ ] Name input with validation (3-16 chars, alphanumeric)
- [ ] Gender selection (Male, Female, Other)
- [ ] Class selection (Warrior, Mage, Rogue)
- [ ] Appearance customization (face, hair, color)
- [ ] Starting location: Inn at level 1
- [ ] Confirm/Cancel buttons

### US-003: Character deletion
**Description:** As a player, I want to delete an old character so I can free a slot.

**Acceptance Criteria:**
- [ ] Delete button shows confirmation dialog
- [ ] Requires password confirmation
- [ ] Cannot be undone warning
- [ ] Character slot freed after deletion

### US-004: Character appearance
**Description:** As a player, I want to customize my character's look so I stand out.

**Acceptance Criteria:**
- [ ] Face shape selector
- [ ] Hair style selector
- [ ] Hair color picker
- [ ] Skin tone picker
- [ ] Initial equipment shown
- [ ] Mirror preview in creation

## Functional Requirements

- FR-1: CharacterSelect.tscn scene
- FR-2: CharacterCreate.tscn scene
- FR-3: CharacterAppearanceComponent
- FR-4: CharacterValidatorComponent
- FR-5: CharacterSlotManagerComponent
- FR-6: CharacterPortraitCache

## Non-Goals

- No advanced sculpting (preset only)
- No dye system (prestige feature)
- No character transfer between accounts

## Technical Considerations

- CharacterComponent in CoreTypes.hpp
- Character select via login server packet
- Portrait thumbnail generated on creation

## Success Metrics

- Character select loads in < 1s
- Character creation completes in < 5s
- No duplicate character names allowed