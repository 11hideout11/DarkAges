# PRD: Loading Screen Implementation

## Introduction

DarkAges has no loading screen between zones or during initial connection. Transitions are jarring with instant switches. This PRD implements a loading screen with async loading and tips.

## Goals

- Loading progress display
- Async scene loading
- Tips/guidance display
- Background artwork

## User Stories

### LOAD-001: Zone Transition Loading
**Description:** As a player, I want to see a loading screen when changing zones.

**Acceptance Criteria:**
- [ ] Black screen with loading indicator
- [ ] Progress shown (percent or spinner)
- [ ] Current zone name displayed
- [ ] Target zone name displayed

### LOAD-002: Tips Display
**Description:** As a loading player, I want to see tips.

**Acceptance Criteria:**
- [ ] Combat tips rotating
- [ ] Control tips
- [ ] Game lore tips
- [ ] Can skip tip

### LOAD-003: Connection Loading
**Description:** As a new player, I want a loading screen on connect.

**Acceptance Criteria:**
- [ ] "Connecting to server..." shown
- [ ] Server name displayed
- [ ] Timeout handling

## Functional Requirements

- FR-1: LoadingPanel scene
- FR-2: Tip database
- FR-3: Async load functions
- FR-4: Cancel load option

## Non-Goals

- Video loading screen
- Skip button for critical loads

## Technical Considerations

```gdscript
# LoadingScreen.gd
func load_zone(zone_id):
    loading_screen.show()
    await load_scene_async()
    loading_screen.hide()
```

## Success Metrics

- Loading shown during zone changes
- Tips rotate correctly

## Open Questions

- Tip rotation interval?
- Number of tips?