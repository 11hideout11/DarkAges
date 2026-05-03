# PRD: Client Minimap - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-minimap-system.md`
- **Client Code**: NONE
- **Priority**: P3 (Polish)
- **Location Needed**: `src/client/scenes/`

## Implementation Checklist

### Client: Minimap.tscn
Create `src/client/scenes/Minimap.tscn`:
```
Node3D (Control subclass for UI overlay)
├── MapImage (TextureRect) - terrain overview
├── PlayerArrow (Sprite2D) - current position/rotation
├── PartyIcons (Container) - party member dots
├── NPCMarkers (Container) - quest/boss markers
└── ZoomControls (HBoxContainer)
  - ZoomIn button
  - ZoomOut button
```

### Client: Minimap.cs
Create `src/client/scripts/Minimap.cs`:
```csharp
public partial class Minimap : Control
{
    [Export] private Texture2D mapTexture;
    [Export] private float zoomLevel = 1.0f;
    [Export] private float minZoom = 0.5f;
    [Export] private float maxZoom = 3.0f;
    
    public void UpdatePlayerPosition(Vector3 worldPos);
    public void AddPartyMember(uint64_t id, Vector3 worldPos, Color color);
    public void RemovePartyMember(uint64_t id);
    public void AddMarker(Vector3 worldPos, MarkerType type, string label);
    public void OnZoomChanged(float newZoom);
    
    // Convert world pos to minimap coordinates
    private Vector2 WorldToMap(Vector3 worldPos);
}
```

### Integration Points
1. **OnPlayerMove** - Update position on minimap
2. **OnPartyUpdate** - Sync party member positions
3. **OnQuestAccept** - Show quest objective markers
4. **OnNPCSpawn** - Show NPC markers (optional)

### World Map Extension
Create `src/client/scenes/WorldMap.tscn`:
- Full zone overview
- Click to teleport (with warning for cross-zone)
- Zone name labels

## Acceptance Criteria
- [ ] Minimap.tscn exists in scenes/
- [ ] Player arrow shows rotation and position
- [ ] Zoom in/out works
- [ ] Party members appear as dots
- [ ] Quest markers show when available