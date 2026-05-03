# PRD: Crafting System Implementation

## Introduction

DarkAges has a CraftingSystem header but no implementation. Players cannot craft items, combine materials, or create equipment. This PRD implements basic crafting for player progression.

## Goals

- Recipe-based crafting
- Material consumption
- Crafted item creation
- Crafting UI

## User Stories

### CRA-001: Recipe Learning
**Description:** As a player, I want to learn crafting recipes.

**Acceptance Criteria:**
- [ ] Recipes learned from trainers
- [ ] Starting recipes for basic items
- [ ] Recipe list in UI

### CRA-002: Crafting Process
**Description:** As a player, I want to craft items.

**Acceptance Criteria:**
- [ ] Select recipe in UI
- [ ] Materials consumed
- [ ] Item created in inventory
- [ ] Crafting time (optional)

### CRA-003: Recipe Data
**Description:** As a developer, I want recipes in data.

**Acceptance Criteria:**
- [ ] Recipe database
- [ ] Input/output materials
- [ ] Required skill level

## Functional Requirements

- FR-1: RecipeDefinition struct
- FR-2: CraftingInputPacket
- FR-3: Integration with ItemSystem
- FR-4: CraftingPanel UI

## Non-Goals

- No complex crafting trees
- No recipe discovery
- No crafting skills

## Technical Considerations

### Recipe Definition
```cpp
struct RecipeDefinition {
    uint32_t recipeId;
    std::string name;
    std::vector<MaterialInput> inputs;
    uint32_t outputItemId;
    uint32_t outputQuantity;
    uint32_t craftingTimeMs;
};
```

## Success Metrics

- Recipe list displays
- Items can be crafted
- Materials consumed

## Open Questions

- Number of starting recipes?
- Crafting UI location?