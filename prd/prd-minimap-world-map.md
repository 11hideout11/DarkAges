# PRD: Minimap & World Map System

## Introduction

Implement a minimap and world map system that shows player position, nearby entities, points of interest, and zone overview. The system should support both a small radar-style minimap and a full world map view.

## Goals

- Create minimap HUD element showing nearby area
- Show player position and direction
- Display nearby entities (players, NPCs, objectives)
- Create full world map with zone overview
- Show points of interest and NPCs
- Quest marker integration

## User Stories

### US-001: Minimap display
**Description:** As a player, I want to see a minimap so I know my surroundings.

**Acceptance Criteria:**
- [ ] Circular or square minimap in corner
- [ ] Player arrow shows position + direction
- [ ] Zoom level adjustable
- [ ] Entity dots colored by type
- [ ] Minimap toggle via key (M)

### US-002: Entity markers
**Description:** As a player, I want to see markers so I find things easily.

**Acceptance Criteria:**
- [ ] Party members shown in green
- [ ] Quest objectives shown in yellow
- [ ] NPCs shown with icons
- [ ] Enemies shown in red
- [ ] Custom waypoints shown

### US-003: Full world map
**Description:** As a player, I want to see the full world map so I can plan travel.

**Acceptance Criteria:**
- [ ] Opens with M key (hold) or button
- [ ] All zones visible
- [ ] Player location highlighted
- [ ] Zones color-coded by level
- [ ] Zone names displayed
- [ ] Click to set waypoint

### US-004: Quest integration
**Description:** As a player, I want quest markers on map so I find objectives.

**Acceptance Criteria:**
- [ ] Quest objectives show on minimap
- [ ] Quest objectives show on map
- [ ] Distance indicator
- [ ] Arrow pointing to far objectives
- [ ] Quest tracker links work

## Functional Requirements

- FR-1: MinimapComponent in HUD
- FR-2: WorldMapComponent with canvas
- FR-3: MapMarkerComponent for entities
- FR-4: QuestMarkerComponent for objectives
- FR-5: WaypointComponent for custom markers

## Technical Considerations

- Godot TextureRect or custom draw
- Map tiles at multiple zoom levels
- Culling for performance

## Success Metrics

- Minimap updates at 60 FPS
- Map loads in < 500ms
- No markers missing