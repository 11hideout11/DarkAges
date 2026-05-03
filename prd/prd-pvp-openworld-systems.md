# PRD-033: PvP/Open World System - Complete

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Critical (4 - PvP MMO needs)  

---

## 1. Introduction/Overview

Add the essential PvP-focused systems that an open-world MMO requires beyond basic combat. This addresses the player-killing, territory, and competitive features essential for PvP gameplay.

### Missing Systems Identified
- PK (Player Killer) system and karma mechanics
- PvP zones vs safe zones
- Guild vs Guild warfare
- Territory control
- World boss encounters
- Seasonal/dynamic events

---

## 2. Goals

- Enable PK mode with karma consequences
- Define PvP zones vs safe zones
- Implement guild warfare
- Add territory control points
- Add world boss spawns

---

## 3. User Stories

### US-033-001: PK Mode Toggle
**Description:** As a player, I want to enable PK mode so that I can attack other players.

**Acceptance Criteria:**
- [ ] Toggle to enable PK mode
- [ ] PK players can attack each other
- [ ] Non-PK players protected
- [ ] Visual indicator for PK players

### US-033-002: Karma System
**Description:** As a player, I want consequences for PKing so that there's reason not to grief.

**Acceptance Criteria:**
- [ ] Karma decreases on kill
- [ ] Low karma = negative effects
- [ ] High karma = positive effects
- [ ] Karma decay over time

### US-033-003: Zone PvP Classification
**Description:** As a designer, I want to define which zones are PvP so that players know the rules.

**Acceptance Criteria:**
- [ ] Safe zones defined (no PvP)
- [ ] PvP zones defined (full PvP)
- [ ] Contested zones (Guild vs Guild)
- [ ] Zone type displayed on entry

### US-033-004: Guild Warfare
**Description:** As a guild leader, I want to declare war so that my guild can battle another guild.

**Acceptance Criteria:**
- [ ] War declaration mechanic
- [ ] War status visible
- [ ] Guild members in combat
- [ ] War score tracking

### US-033-005: Territory Control
**Description:** As a guild, I want to control territory so that my guild has a home base.

**Acceptance Criteria:**
- [ ] Control points in zones
- [ ] Guild can capture points
- [ ] Points generate revenue
- [ ] Defense bonuses

### US-033-006: World Boss Spawns
**Description:** As a player, I want world bosses so that there's challenging raid content.

**Acceptance Criteria:**
- [ ] Boss spawn timers
- [ ] Boss spawns on world event
- [ ] Large health pools
- [ ] Rare loot tables

### US-033-007: Seasonal Events
**Description:** As a developer, I want seasonal events so that the game stays fresh.

**Acceptance Criteria:**
- [ ] Event schedule system
- [ ] Holiday themes
- [ ] Special rewards
- [ ] Event announcements

---

## 4. Functional Requirements

- FR-033-1: PlayerComponent has pk_mode flag
- FR-033-2: KarmaComponent tracks player karma (-1000 to +1000)
- FR-033-3: Zone definitions include zone_type (Safe/PvP/Contested)
- FR-033-4: GuildComponent has guild_id for territorial claims
- FR-033-5: TerritoryComponent tracks ownership and points
- FR-033-6: WorldBoss spawn system with timers
- FR-033-7: EventSchedule for seasonal content
- FR-033-8: GuildWar declaration via guild management

---

## 5. Non-Goals

- No arena/ranked matchmaking (deferred)
- No battleground/instanced PvP
- No global ladder system
- No esports features

---

## 6. Technical Considerations

### Zone Types
| Type | Abbr. | PvP Rules |
|------|-------|----------|
| SAFE | S | No PvP allowed |
| PVP | P | Full PvP open |
| CONTESTED | C | Guild vs Guild only |
| GUILD | G | Guild-controlled |

### Karma Effects
| Karma Range | Effect |
|-------------|--------|
| > 800 | +10% XP, +5% drop rate |
| 0-800 | None |
| < 0 | -10% XP, red name |
| < -500 | Drops item on death |

### Territory Control
- Control point per zone (castle/keep)
- Capture requires X players for Y duration
- Generates gold税收 each tick
- Guild members get discount

### World Bosses
| Boss | Zone | Spawn | Loot |
|------|------|-------|------|
| Dragon King | Dragon Peak | Weekly | Legendary |
| Lich Lord | Crypt | 4hr | Epic |
| Giant | Forest | Daily | Rare |

---

## 7. Success Metrics

- [ ] PK mode toggles correctly
- [ ] Karma tracks and effects apply
- [ ] Zone types enforced
- [ ] Guild warfare works
- [ ] Territory control captures
- [ ] World bosses spawn
- [ ] Seasonal events run

---

## 8. Open Questions

1. Q: Karma decay rate?
   A: 1 point per hour offline, 10 per hour online

2. Q: War declaration cost?
   A: 1000 gold from treasury

3. Q: Territory capture time?
   A: 10 minutes with 5+ players

---

**PRD Status:** Proposed - Ready for Implementation  
**Filename:** prd-033-pvp-openworld-systems.md

---

# PRD-034: Crafting System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High (RPG progression)  

---

## 1. Introduction/Overview

Add crafting system so players can create items from materials. Essential for economy and progression.

### Problem Statement
- No crafting system exists
- Materials exist in drop tables
- No use for gathered materials
- Economy incomplete

---

## 2. Goals

- Recipe system for crafting
- Crafting UI with material requirements
- Material gathering basics
- Quality tiers for crafted items

---

## 3. User Stories

### US-034-001: Recipe Learning
**Description:** As a player, I want to learn recipes so that I can craft items.

**Acceptance Criteria:**
- [ ] Recipes unlock on level
- [ ] Recipe book accessible
- [ ] Recipes show requirements
- [ ] Craftable items highlighted

### US-034-002: Crafting Process
**Description:** As a player, I want to craft items so that I can make gear.

**Acceptance Criteria:**
- [ ] Select recipe
- [ ] Check materials
- [ ] Confirm craft
- [ ] Item added to inventory

### US-034-003: Material Gathering
**Description:** As a player, I want to gather materials so that I can craft.

**Acceptance Criteria:**
- [ ] Mining nodes spawn
- [ ] Herb nodes spawn
- [ ] Gathering uses action
- [ ] Materials added to inventory

### US-034-004: Crafted Item Quality
**Description:** As a player, I want quality crafted items so that they're worth using.

**Acceptance Criteria:**
- [ ] Quality tiers (Normal, Fine, Masterwork)
- [ ] Higher tier = better stats
- [ ] Requires higher skill
- [ ] Random success chance

---

## 4. Functional Requirements

- FR-034-1: Recipe database with material requirements
- FR-034-2: CraftingComponent tracks known recipes
- FR-034-3: CraftingInteraction - craft button initiates
- FR-034-4: Gathering nodes spawn in zones
- FR-034-5: GatheringComponent tracks gather skill
- FR-034-6: Quality system with random roll

---

## 5. Non-Goals

- No complex crafting chains
- No enchanting/upgrading
- No consumable brewing
- No building/structure crafting

---

## 6. Technical Considerations

### Recipe Structure
```json
{
  "id": 1,
  "name": "Iron Sword",
  "type": "Weapon",
  "materials": {
    "iron_ingot": 3,
    "leather": 1
  },
  "skill": 10,
  "quality_tiers": {
    "normal": 0.8,
    "fine": 0.15,
    "masterwork": 0.05
  }
}
```

### Crafting Categories
| Category | Items | Skill |
|----------|-------|-------|
| Blacksmith | Weapons, Armor | Smithing |
| Leather | Armor, Accessories | Leatherworking |
| Alchemy | Potions, Scrolls | Alchemy |
| Enchanting | Gems, Scrolls | Enchanting |

### 20 Initial Recipes
- Weapons: 5 (Iron sword, steel sword, etc.)
- Armor: 8 (Leather, chain, plate)
- Consumables: 7 (Potions, scrolls)

---

## 7. Success Metrics

- [ ] Recipe system works
- [ ] Crafting UI functional
- [ ] Materials gatherable
- [ ] 20 recipes craftable
- [ ] Quality tiers apply

---

## 8. Open Questions

1. Q: Recipe learning method?
   A: Auto-learn at crafting level thresholds

2. Q: Gathering cooldown?
   A: 2 second gather time

3. Q: Node spawn rate?
   A: 5-10 per zone, 5 min respawn

---

**PRD Status:** Proposed  
**Filename:** prd-034-crafting-system.md

---

# PRD-035: Auction House / Market

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Medium (Economy)  

---

## 1. Introduction/Overview

Add player-to-player item trading via auction house. Essential for MMO economy.

### Problem Statement
- No trading system
- No market for items
- Players can't sell items
- Economy incomplete

---

## 2. Goals

- List items for sale
- Browse and search listings
- Purchase items
- Gold transaction system

---

## 3. User Stories

### US-035-001: Listing Items
**Description:** As a player, I want to list items for sale so that I can earn gold.

**Acceptance Criteria:**
- [ ] Select item from inventory
- [ ] Set price
- [ ] Confirm listing
- [ ] Item removed from inventory

### US-035-002: Browsing Listings
**Description:** As a player, I want to browse listings so that I can find items.

**Acceptance Criteria:**
- [ ] Search by name
- [ ] Filter by category
- [ ] Filter by price range
- [ ] Sort by newest/cheapest

### US-035-003: Purchasing
**Description:** As a player, I want to buy items so that I can get gear.

**Acceptance Criteria:**
- [ ] Click buy on listing
- [ ] Gold deducted
- [ ] Item added to inventory
- [ ] Seller receives gold

### US-035-004: Listing Management
**Description:** As a player, I want to manage my listings so that I can adjust prices.

**Acceptance Criteria:**
- [ ] View active listings
- [ ] Cancel listing
- [ ] Relist with new price

---

## 4. Functional Requirements

- FR-035-1: ListingComponent tracks player listings
- FR-035-2: AuctionDatabase stores all listings
- FR-035-3: Search by name/category/price
- FR-035-4: Buy transaction handles gold
- FR-035-5: Tax system (5% on sale)
- FR-035-6: Expire listings after 48 hours

---

## 5. Non-Goals

- No instant buyout offers
- No auction bids (price only)
- No guild stores
- No limited-time auctions

---

## 6. Technical Considerations

### Listing Structure
```json
{
  "id": 1,
  "seller_id": 123,
  "item_id": 5,
  "price": 100,
  "created_at": 1234567890,
  "expires_at": 1234582690
}
```

### Categories
| Category | Items |
|----------|-------|
| Weapons | Swords, axes, bows |
| Armor | Helmets, chest, legs |
| Accessories | Rings, amulets |
| Consumables | Potions, food |
| Materials | Ores, herbs |

### Fees
| Fee | Amount |
|-----|--------|
| Listing fee | Free |
| Sale tax | 5% |
| Relist fee | Free |

---

## 7. Success Metrics

- [ ] Items listable
- [ ] Search works
- [ ] Purchase works
- [ ] Gold transfers
- [ ] 48hr expiry works

---

## 8. Open Questions

1. Q: Max listings per player?
   A: 10 active

2. Q: Stack listing?
   A: Yes, for consumables

3. Q: Search method?
   A: Full-text search on name

---

**PRD Status:** Proposed  
**Filename:** prd-035-auction-house.md

---

# PRD-036: Progression & Leveling System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High (RPG core)  

---

## 1. Introduction/Overview

Add character progression via leveling, stats, and advancement systems. Without this, no sense of character growth.

### Problem Statement
- No leveling system
- No stat allocation
- No advancement milestones
- No character growth

---

## 2. Goals

- XP from kills/quests
- Level-up system (max 50)
- Stat allocation
- Skill point spending

---

## 3. User Stories

### US-036-001: XP Gained
**Description:** As a player, I want to gain XP so that I can level up.

**Acceptance Criteria:**
- [ ] Kill grants XP
- [ ] Quest grants bonus XP
- [ ] XP accumulates
- [ ] Level-up when threshold met

### US-036-002: Level-Up
**Description:** As a player, I want to level up so that my character grows.

**Acceptance Criteria:**
- [ ] Level increases
- [ ] Stat points awarded
- [ ] Skill points awarded
- [ ] Health/mana restored

### US-036-003: Stat Allocation
**Description:** As a player, I want to allocate stats so that I can customize my character.

**Acceptance Criteria:**
- [ ] Stat points available
- [ ] Allocate to STR/DEX/INT
- [ ] Stats apply bonuses
- [ ] Allocation locked on confirm

### US-036-004: Skill Points
**Description:** As a player, I want skill points so that I can learn abilities.

**Acceptance Criteria:**
- [ ] Skill points on level
- [ ] Spend on ability unlocks
- [ ] 1 ability per level learned
- [ ] Cannot undo (permanent)

---

## 4. Functional Requirements

- FR-036-1: XPComponent tracks player XP
- FR-036-2: Level up on XP threshold
- FR-036-3: StatComponent for STR/DEX/INT/CON
- FR-036-4: SkillTreeComponent for ability unlocks
- FR-036-5: XP from KillEvent, QuestCompleteEvent
- FR-036-6: Base stats scale with level

---

## 5. Non-Goals

- No respec (permanent choices)
- No Prestige levels
- No multiclassing
- No elite specializations

---

## 6. Technical Considerations

### Stats
| Stat | Abbr. | Effect |
|------|-------|--------|
| Strength | STR | +2 damage |
| Dexterity | DEX | +1% crit |
| Intelligence | INT | +10 mana |
| Constitution | CON | +20 health |

### Level Curve
| Level | XP Required | Total XP |
|-------|-----------|---------|
| 1 | 0 | 0 |
| 2 | 100 | 100 |
| 3 | 300 | 400 |
| ... | ... | ... |
| 50 | 1,000,000 | 2,500,000 |

### Points Per Level
| Level | Stat Points | Skill Points |
|-------|------------|--------------|
| 1-10 | 2 | 1 |
| 11-30 | 3 | 2 |
| 31-50 | 4 | 3 |

---

## 7. Success Metrics

- [ ] XP from kills works
- [ ] Level-up triggers correctly
- [ ] Stat allocation works
- [ ] Skill points spendable
- [ ] Max level 50 alcanzable

---

## 8. Open Questions

1. Q: XP from grouping?
   A: +25% per party member

2. Q: Death XP penalty?
   A: No (PvP game)

3. Q: Level sync?
   A: Party members to lowest level

---

**PRD Status:** Proposed  
**Filename:** prd-036-progression-leveling.md

---

# PRD-037: Social & Chat Systems

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Medium (Core MMO)  

---

## 1. Introduction/Overview

Add chat, friends, and social systems essential for MMO community.

### Problem Statement
- No chat channels
- No friends list
- No whisper/private messaging
- No emote system

---

## 2. Goals

- Multiple chat channels
- Friends list with online status
- Whisper/PM system
- Emote animations

---

## 3. User Stories

### US-037-001: Chat Channels
**Description:** As a player, I want chat channels so that I can communicate.

**Acceptance Criteria:**
- [ ] Global/Say channel
- [ ] Zone/Shout channel
- [ ] Guild channel
- [ ] Party channel
- [ ] Trade channel

### US-037-002: Friends List
**Description:** As a player, I want a friends list so that I can see who's online.

**Acceptance Criteria:**
- [ ] Add friend by name
- [ ] Friend shows online/offline
- [ ] Friend shows zone
- [ ] Remove friend
- [ ] Block player

### US-037-003: Whisper
**Description:** As a player, I want to whisper so that I can talk privately.

**Acceptance Criteria:**
- [ ] /w player message
- [ ] Message delivered
- [ ] Reply with /r
- [ ] Cannot whisper strangers (optional)

### US-037-004: Emotes
**Description:** As a player, I want emotes so that I can express myself.

**Acceptance Criteria:**
- [ ] /wave, /dance, /laugh
- [ ] Animation plays
- [ ] Others see emote
- [ ] Shortcut buttons

---

## 4. Functional Requirements

- FR-037-1: ChatComponent tracks channel subscriptions
- FR-037-2: ChatMessageEvent broadcasts to channel
- FR-037-3: FriendsComponent stores friends list
- FR-037-4: Online status updates via heartbeat
- FR-037-5: WhisperComponent handles PMs
- FR-037-6: EmoteAnimation triggers on command
- FR-037-7: BlockComponent blocks muted players

---

## 5. Non-Goals

- No voice chat
- No video chat
- No avatar/customization chat
- No chat moderation tools

---

## 6. Technical Considerations

### Chat Channels
| Channel | Range | Commands |
|---------|-------|----------|
| SAY | 20m | /s, /say |
| SHOUT | Zone | /sh, /shout |
| GUILD | Guild | /g, /guild |
| PARTY | Party | /p, /party |
| TRADE | Zone | /trade |

### Friend Limits
- Max friends: 100
- Max blocked: 50

### Emotes List
/wave, /nod, /dance, /laugh, /cry, /cheer, /clap, /sit, /sleep, /eat

---

## 7. Success Metrics

- [ ] All chat channels work
- [ ] Friends list works
- [ ] Whisper delivered
- [ ] Emotes play
- [ ] Block works

---

## 8. Open Questions

1. Q: Chat history length?
   A: 100 messages

2. Q: AFK auto-set?
   A: After 10 minutes

3. Q: Default channel?
   A: Say on join

---

**PRD Status:** Proposed  
**Filename:** prd-037-social-chat.md

---

# PRD-038: UI/HUD System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** High (Playability)  

---

## 1. Introduction/Overview

Consolidate all client UI into unified HUD with health, mana, minimap, quest tracker, and inventory accessible via hotkeys.

### Problem Statement
- UI scattered
- No hotkey integration
- No HUD overlay
- Incomplete menus

---

## 2. Goals

- Unified HUD overlay
- Hotkey menu access
- Compact and expanded modes
- Configurable keybinds

---

## 3. User Stories

### US-038-001: HUD Display
**Description:** As a player, I want to see my status so that I know my state.

**Acceptance Criteria:**
- [ ] Health bar visible
- [ ] Mana bar visible
- [ ] XP bar visible
- [ ] Level shown

### US-038-002: Hotkey Menus
**Description:** As a player, I want hotkey menus so that I can access everything.

**Acceptance Criteria:**
- [ ] I = Inventory
- [ ] Q = Quest log
- [ ] K = Skills/Abilities
- [ ] M = Map
- [ ] P = Party
- [ ] G = Guild

### US-038-003: Minimap
**Description:** As a player, I want a minimap so that I don't get lost.

**Acceptance Criteria:**
- [ ] Zone map displayed
- [ ] Player position shown
- [ ] Quest marker shown
- [ ] Guild marker shown

### US-038-004: Quest Tracker
**Description:** As a player, I want a quest tracker so that I know what to do.

**Acceptance Criteria:**
- [ ] Current quest shown
- [ ] Objective progress shown
- [ ] Click to show in log

### US-038-005: Settings
**Description:** As a player, I want settings so that I can customize.

**Acceptance Criteria:**
- [ ] Graphics settings
- [ ] Audio settings
- [ ] Keybind customization
- [ ] UI scale slider

---

## 4. Functional Requirements

- FR-038-1: HUD overlay with all bars
- FR-038-2: GodotControl hotkey detection
- FR-038-3: MinimapComponent shows zone map
- FR-038-4: QuestTracker shows current
- FR-038-5: Settings saved to config
- FR-038-6: UI scale from 75%-150%

---

## 5. Non-Goals

- No advanced macro system
- No UI themes
- No nameplates customization
- No damage/heal logs (deferred to combat text)

---

## 6. Technical Considerations

### Default Hotkeys
| Key | Function |
|-----|----------|
| I | Inventory |
| Q | Quests |
| K | Abilities |
| M | Map (fullscreen) |
| Tab | Minimap toggle |
| C | Character |
| P | Party |
| G | Guild |
| B | Auction House |
| O | Options |
| Esc | Menu |

### HUD Layout
```
┌──────────────────────────────┐
│ [HP] ████████░░ [MP] ████░░ │ ← Top bar
│ [Lvl 5] [XP] ████░░        │
├────────────────────────────┤
│                             │
│   [■] Player          [■]   │ ← Minimap corner
│                             │
├────────────────────────────┤
│ [Quest: Goblins 3/10]        │ ← Quest tracker
├──────────────────────────────┤
│ [Ability] [Ability] [Ability]│ ← Ability bar
└──────────────────────────────┘
```

---

## 7. Success Metrics

- [ ] All HUD elements visible
- [ ] All hotkeys functional
- [ ] Minimap shows position
- [ ] Quest tracker updates
- [ ] Settings save/load

---

## 8. Open Questions

1. Q: HUD opacity?
   A: 80% transparent background

2. Q: Keybinds save?
   A: Yes, to config file

3. Q: Multiple UI profiles?
   A: No, single profile

---

**PRD Status:** Proposed  
**Filename:** prd-038-ui-hud-system.md

---

**PRD Collection Complete**  
**Author:** OpenHands Analysis  
**Date:** 2026-05-03  
**Count:** 8 new PRD documents created

| PRD | Name | Priority |
|-----|------|----------|
| PRD-027 | GNS Runtime Integration | Critical |
| PRD-028 | Combat FSM Integration | Critical |
| PRD-029 | Zone Objectives | Critical |
| PRD-030 | Inventory/Equipment | High |
| PRD-031 | Abilities System | High |
| PRD-032 | Quest System | High |
| PRD-033 | PvP/Open World Systems | Critical (PvP MMO) |
| PRD-034 | Crafting System | High (Economy) |
| PRD-035 | Auction House | Medium (Economy) |
| PRD-036 | Progression System | High (RPG) |
| PRD-037 | Social/Chat | Medium (Core MMO) |
| PRD-038 | UI/HUD System | High (Playability) |