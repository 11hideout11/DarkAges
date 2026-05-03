# PRD-036: Player Progression System - Complete

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** ⚠️ Partial — ProgressionCalculator.hpp exists with stat recalculation and level-up methods, but XP/leveling not fully wired into game loop
**Priority:** P0-Critical  
**Category:** Player Progression - Cross-System Integration

---

## 1. Introduction/Overview

Unify XP gain, leveling, talent trees, and equipment progression into a cohesive player advancement loop. Currently these systems exist separately but are not integrated into a unified progression experience.

### Problem Statement
- ExperienceSystem.hpp exists but disconnected from talents and equipment
- Leveling provides stat bonuses but no talent points
- Equipment drops but no power scaling with player level
- No clear progression feedback loop when advancing
- Players don't feel "stronger" as they progress

### Why This Matters for AAA
- Progression is the core engagement loop of any MMO
- Must feel rewarding at every milestone
- Requires tight integration between combat, loot, and character systems

---

## 2. Goals

- XP gained from combat, quests, and exploration converts to character advancement
- Level milestones unlock talent points (every 2 levels: 2, 4, 6, 8, 10...)
- Equipment power scales with player level (Level + Quality + Rarity formula)
- Stat bonuses from equipment complement base character stats
- Visual feedback: particles, sounds, UI updates when leveling up
- Progression stored in character save data

---

## 3. User Stories

### US-036-001: XP Gain Integration
**Description:** As a player, I want to earn XP from defeating enemies so that I can progress my character.

**Acceptance Criteria:**
- [ ] Enemies grant XP on death (scaled by enemy level and type)
- [ ] Quests grant XP on completion (scaled by difficulty)
- [ ] Bonus XP for "first kill of the day" / daily challenges
- [ ] XP packet sent to client: PACKET_XP_UPDATE (new type)

### US-036-002: Level Up Trigger
**Description:** As a player, I want to level up when reaching XP thresholds so that my character grows stronger.

**Acceptance Criteria:**
- [ ] XP thresholds: Level N requires N*100 + N²*10 XP (Level 2=220, Level 5=575, Level 10=1100)
- [ ] Character stats increase on level up: +5 HP, +1 STR, +1 DEX, +1 VIT
- [ ] Talent point granted every 2 levels
- [ ] Level-up notification displays on client
- [ ] Level persists across sessions

### US-036-003: Talent Point Unlocks
**Description:** As a player with talent points, I want to spend them on abilities so that I can customize my build.

**Acceptance Criteria:**
- [ ] Points available at levels: 2, 4, 6, 8, 10...
- [ ] Talent UI shows available points
- [ ] Talents modify combat abilities (damage, cooldown, range)
- [ ] Active talents sync to combat system
- [ ] Respeccing costs in-game currency

### US-036-004: Equipment Power Scaling
**Description:** As a player with equipment, I want items to scale with my level so that gear remains relevant.

**Acceptance Criteria:**
- [ ] Equipment stats use formula: (Base + LevelBonus + QualityBonus + RarityBonus)
- [ ] LevelBonus = player_level * item_level_requirement * 0.5
- [ ] Quality: Common(+0%), Uncommon(+10%), Rare(+25%), Epic(+50%), Legendary(+100%)
- [ ] Higher-level content drops higher-scaling gear
- [ ] Tooltip shows scaled values

### US-036-005: Stat Calculation Integration
**Description:** As the combat system, I want final character stats so that damage and health are accurate.

**Acceptance Criteria:**
- [ ] Final_HP = Base_HP + (VIT * 10) + Equipment_HP_Bonus
- [ ] Final_DMG = Base_DMG + (STR * 2) + Equipment_DMG_Bonus
- [ ] Final_DEF = Equipment_DEF_Bonus + (VIT * 0.5)
- [ ] Combat uses final stats, not raw values
- [ ] Stat changes apply immediately

---

## 4. Functional Requirements

- FR-036-1: Integrate ExperienceSystem with CombatSystem (XP on kill)
- FR-036-2: Integrate ExperienceSystem with QuestSystem (XP on completion)
- FR-036-3: Integrate ExperienceSystem with Level up (trigger: checkThreshold, applyBonus)
- FR-036-4: Integrate TalentSystem with Level (unlock points)
- FR-036-5: Integrate ItemSystem with Level (scale equipment)
- FR-036-6: Integrate CombatSystem with all stats (use final values)
- FR-036-7: Serialize progression to save file
- FR-036-8: Deserialize progression on login
- FR-036-9: Network protocol for XP, level, talent updates

---

## 5. Non-Goals

- No auto-leveling or boosting (players earn progression)
- No cross-server progression (per-server only)
- No real-money transactions for progression
- No level cap increase beyond 50 in v1
- No prestige system in v1

---

## 6. Technical Considerations

- Stat calculation happens on server (server-authoritative)
- Client displays calculated stats (read-only)
- Save file stores: xp, level, talent_points_spent, talents_unlocked
- Level up triggers recalc of all derived stats
- Equipment change triggers stat recalc

### Dependencies
- ExperienceSystem (existing, incomplete)
- EquipmentSystem (existing, uses scaled stats)
- CombatSystem (existing, needs final stats)
- SaveSystem (existing GAP-014)

---

## 7. Success Metrics

- Player can progress from Level 1 to 10 in 30 minutes of play
- Level up provides visible stat increase
- Equipment power correlates to player level
- Talent trees provide build variety
- Zero exploits in progression system

---

## 8. Open Questions

- Should there be level-scaled damage reduction in lower-level zones?
- How to handle level-scaling in group content?
- Rarity drop rates for each source?