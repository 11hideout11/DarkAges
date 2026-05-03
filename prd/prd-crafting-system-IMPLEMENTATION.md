# PRD: Crafting System - Implementation

## Status
**PRD EXISTED BUT NOT IMPLEMENTED**

## Gap Analysis
- **PRD File**: `prd/prd-crafting-system.md`
- **Data**: `data/items.json` has crafting ingredients
- **Server Code**: NONE
- **Client Code**: NONE
- **Priority**: P2 (Gameplay)

## Implementation Checklist

### Server: CraftingSystem.hpp
Create `src/server/src/content/CraftingSystem.hpp`:
```cpp
namespace DarkAges::content {

struct Recipe {
  uint32_t recipe_id;
  uint32_t output_item_id;
  uint32_t quantity;
  struct Ingredient { uint32_t item_id; uint32_t quantity; } ingredients[];
  uint32_t required_skill_level;
  uint32_t crafting_time_ms;
};

struct CraftingProgress {
  uint64_t player_id;
  uint32_t recipe_id;
  uint32_t elapsed_ms;
  bool is_complete;
};

class CraftingSystem {
public:
  void LoadRecipes(); // Load from data or config
  bool CanCraft(uint64_t player_id, uint32_t recipe_id);
  void StartCrafting(uint64_t player_id, uint32_t recipe_id);
  void CancelCrafting(uint64_t player_id);
  void Tick(uint32_t delta_ms); // Progress crafting
  auto GetActiveCrafting(uint64_t player_id) -> std::optional<CraftingProgress>;
  auto GetKnownRecipes(uint64_t player_id) -> std::vector<uint32_t>;
};

}
```

### Server: CraftingSystem.cpp
- LoadRecipes: parse recipe database (from JSON or hardcode)
- CanCraft: check player meets requirements (skill level, ingredients)
- StartCrafting: deduct ingredients, start timer
- Tick: progress crafting, complete when elapsed >= required
- Complete: add output item to player inventory, notify client

### Packet Types
- PACKET_CRAFT_START = 70
- PACKET_CRAFT_COMPLETE = 71
- PACKET_CRAFT_CANCEL = 72
- PACKET_CRAFT_PROGRESS = 73 // Sync progress bar

### Client: CraftingUI.tscn
Create `src/client/scenes/Crafting/`:
- CraftingPanel.tscn - Recipe list and craft buttons
- CraftingSlot.tscn - Ingredient slots
- ProgressBar - Crafting timer

## Data Integration
Items.json already has:
- Crafting ingredients (type: "material")
- Tools (type: "tool")
- Recipe data can be JSON or hardcoded

## Acceptance Criteria - IMPLEMENTATION VERIFIED

- [ ] Server: CraftingSystem.hpp/cpp exists
- [ ] Can craft simple recipe (combine 2 materials)
- [ ] Ingredients deducted on start
- [ ] Output item received on complete
- [ ] Client shows crafting UI with recipes
- [ ] Progress bar updates during craft
- [ ] Integration test: craft an item