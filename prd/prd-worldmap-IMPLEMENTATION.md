# PRD: World Map - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-minimap-world-map.md`
- **Client Code**: NONE
- **Priority**: P3 (Polish)

## Implementation Checklist

### Client: WorldMap.tscn
Create `src/client/scenes/WorldMap.tscn`:
```
Control (WorldMap)
├── FullZoneMap (TextureRect) // Large map image
├── ZoneLabels (Container)
│   ├── TutorialLabel
│   ├── ArenaLabel
│   └── BossLabel
├── PlayerMarker (Sprite2D)
├── ZoomControls (HBoxContainer)
├── CloseButton
└── TeleportConfirmDialog
```

### Client: WorldMap.cs
Create `src/client/scripts/WorldMap.cs`:
```csharp
public partial class WorldMap : Control
{
    [Export] private Texture2D mapTexture;
    
    public void Open();
    public void OnZoneClick(uint32_t zoneId);
    public void ShowTeleportConfirm(uint32_t zoneId);
    public void ConfirmTeleport();
    
    // Zone data from data/zones.json
    private Dictionary<uint32_t, Vector2> zonePositions;
}
```

### Integration
- Minimap: "M" key opens WorldMap
- ZoneClick: Check requirements, confirm teleport
- Teleport: ZoneServer migration via handoff

## Acceptance Criteria
- [ ] Opens with "M" key
- [ ] All zones displayed with labels
- [ ] Player position shown
- [ ] Teleport confirmation works