# PRD: Tutorial System Implementation

## Introduction

DarkAges has no tutorial system. New players have no guidance on controls, combat, or game mechanics. This PRD implements a built-in tutorial system for new player onboarding.

## Goals

- In-game tutorial prompts
- Interactive hints
- First-time completion tracking
- Boss/quest hints

## User Stories

### TUT-001: Movement Tutorial
**Description:** As a new player, I want to learn movement controls.

**Acceptance Criteria:**
- [ ] WASD movement prompt
- [ ] Mouse look prompt
- [ ] Jump prompt
- [ ] Can skip tutorial

### TUT-002: Combat Tutorial
**Description:** As a new player, I want to learn combat.

**Acceptance Criteria:**
- [ ] Attack prompt
- [ ] Target lock prompt
- [ ] Hit/avoid prompting
- [ ] Kill tutorial target

### TUT-003: Interface Tutorial
**Description:** As a new player, I want to learn the UI.

**Acceptance Criteria:**
- [ ] Health bar tutorial
- [ ] Inventory tutorial (I key)
- [ ] Ability bar tutorial
- [ ] Chat tutorial

### TUT-004: Tutorial Progress
**Description:** As a system, I want to track progress.

**Acceptance Criteria:**
- [ ] Progress saved
- [ ] New characters restart
- [ ] Completion rewards

## Functional Requirements

- FR-1: TutorialStep in data
- FR-2: TutorialPanel UI
- FR-3: Progress saved to database

## Non-Goals

- Video tutorials
- Voice over

## Technical Considerations

### Tutorial Steps Data
```json
{
  "steps": [
    { "id": 1, "type": "prompt", "text": "Press W to move forward" },
    { "id": 2, "type": "action", "action": "move_forward" },
    { "id": 3, "type": "combat", "lesson": "attack" }
  ]
}
```

## Success Metrics

- Tutorial prompts appear
- Progress tracked

## Open Questions

- Number of tutorial steps?
- Rewards for completion?