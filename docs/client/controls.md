# DarkAges Client Controls

## Movement Controls

| Key | Action | Description |
|-----|--------|-------------|
| W | Move Forward | Standard movement |
| S | Move Backward | Standard movement |
| A | Move Left | Standard movement |
| D | Move Right | Standard movement |
| Shift | Sprint | Hold while moving to sprint (1.5x speed) |
| Space | Jump | Jump when on ground |
| Q | Dodge | Roll/dodge animation (invulnerable) |

## Combat Controls

| Key | Action | Description |
|-----|--------|-------------|
| Left Click | Attack | Basic melee attack (GCD: 1.2s) |
| Right Click | Block | Block incoming attacks |
| T | Target Lock | Lock onto nearest enemy |
| Tab | Target Next | Cycle to next target |
| 1-8 | Abilities | Hotbar ability slots |

## Interaction Controls

| Key | Action | Description |
|-----|--------|-------------|
| E | Interact | Interact with NPCs, objects, loot |
| I | Inventory | Open inventory screen |
| J | Quest Tracker | Toggle quest tracker UI |
| Enter | Chat | Open chat input |
| F1 | Toggle Debug | Show/hide debug visualization |

## UI Controls

| Key | Action | Description |
|-----|--------|-------------|
| Escape | Release Mouse | Toggle mouse capture for UI |

## Control Requirements Validation

### Movement
- ✓ WASD responsive with smooth acceleration
- ✓ Sprint feels distinct from walking (speed multiplier)
- ✓ Jump height appropriate for combat
- ✓ Dodge provides i-frames feedback

### Mouse
- ✓ Camera rotation smooth (60Hz physics tick)
- ✓ Sensitivity tuned for 3rd person combat
- ✓ Lock-on targeting available
- ✓ Mouse capture/releases properly

### Combat
- ✓ Attack has clear wind-up/execute/follow-through
- ✓ Global cooldown prevents spam (1.2s)
- ✓ Hit-stop provides impact feedback (0.05s time freeze)
- ✓ Block held for sustained defense

### Known Issues
- None currently reported

## Camera Defaults

- SpringArm length: 4.0 units
- Vertical angle limits: -45° to +60°
- Horizontal sensitivity: 1.0
- Vertical sensitivity: 0.8
- Collision margin: 0.5 units
