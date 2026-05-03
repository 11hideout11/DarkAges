# PRD: Minimap & World Map System

## Introduction

DarkAges has no minimap or world map. Players cannot see their position in the zone or navigate between zones. This PRD implements both minimap and world map.

## Goals

- Minimap showing local area
- Player position indicator
- Party member positions
- World map showing zone layout

## User Stories

### MAP-001: Minimap
**Description:** As a player, I want a minimap showing my surroundings.

**Acceptance Criteria:**
- [ ] Local area rendered
- [ ] Player arrow showing direction
- [ ] Zoom in/out
- [ ] Toggle via key (M)

### MAP-002: World Map
**Description:** As a player, I want a world map showing zones.

**Acceptance Criteria:**
- [ ] All zones displayed
- [ ] Current zone highlighted
- [ ] Click zone for info
- [ ] Zone portal locations

### MAP-003: Party Positions
**Description:** As a party member, I want to see party positions.

**Acceptance Criteria:**
- [ ] Party members on minimap
- [ ] Different colors
- [ ] Names on hover

## Functional Requirements

- FR-1: MinimapPanel UI scene
- FR-2: WorldMapPanel UI scene
- FR-3: Zone position data

## Non-Goals

- Automatic pathfinding
- NPC markers

## Technical Considerations

### Minimap Implementation
```gdscript
# Minimap renders local entities
# Uses viewport texture
func _process(delta):
    update_player_arrow()
    render_local_entities()  # Within 50m
```

## Success Metrics

- Minimap toggles with M key
- World map accessible

## Open Questions

- Minimap radius?
- Map update frequency?