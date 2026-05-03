# PRD: Sound & Music System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Medium  
**Category:** Client - Polish

---

## 1. Problem Statement

The client has audio directory but no sound system implementation. Game is silent - no combat sounds, ambient audio, UI feedback, or music. This severely impacts immersion.

### Current State
- ⚠️ audio/ directory exists (empty)
- ⚠️ No AudioSystem implementation
- ⚠️ No event-based sounds
- ⚠️ No background music

### Impact
- Silent gameplay experience
- No audio feedback for actions
- Missing immersion layer
- Incomplete demo

---

## 2. Goals

### Primary Goals
1. Create AudioSystem manager
2. Implement combat sound hooks
3. Add UI feedback sounds
4. Integrate background music

### Success Criteria
- [ ] Combat sounds play on hit/spell
- [ ] UI click sounds work
- [ ] Ambient zone sounds play
- [ ] BGM transitions

---

## 3. Technical Specification

### Sound Categories

| Category | Examples | Priority |
|----------|----------|----------|
| combat | hit, cast, swing, block | High |
| ui | click, hover, success, error | High |
| ambient | wind, birds, village | Low |
| music | battle, explore, boss | Medium |
| ambient_enemy | growl, death | Medium |

### Audio System

```csharp
// AudioManager.cs (singleton)
public partial class AudioManager : Node {
    private const int POOL_SIZE = 32;
    
    [Export] private AudioStreamPlayer[] pool;
    [Export] private AudioStreamPlayer musicPlayer;
    [Export] private AudioStreamPlayer ambientPlayer;
    [Export] private float masterVolume = 1.0f;
    
    public void PlaySound(string soundId);
    public void PlayMusic(string trackId);
    public void PlayAmbient(string ambientId);
    public void SetMasterVolume(float volume);
    public void SetSFXVolume(float volume);
    public void SetMusicVolume(float volume);
    
    private AudioStream GetSound(string soundId);
    private int GetFreePlayer();
}

// Sound mappings
// SoundManager plays sounds based on event mappings
private static readonly Dictionary<string, string> SoundMap = new() {
    { "combat.hit", "res://audio/combat/hit.wav" },
    { "combat.swing", "res://audio/combat/swing.wav" },
    { "combat.block", "res://audio/combat/block.wav" },
    { "ui.click", "res://audio/ui/click.wav" },
    { "ui.hover", "res://audio/ui/hover.wav" },
    { "ambient.forest", "res://audio/ambient/forest.wav" },
    { "music.explore", "res://audio/music/explore.mp3" },
};
```

### Event Hooks

```csharp
// Combat sound hooks
void CombatSystem.OnHit(Entity attacker, Entity target, DamageInfo info) {
    AudioManager.PlaySound("combat.hit");
}

void AbilitySystem.OnCast(Entity caster, Ability ability) {
    AudioManager.PlaySound($"combat.{ability.sound}");
}

// UI sound hooks
void UIButton.On pressed() {
    AudioManager.PlaySound("ui.click");
}

// Zone ambient
void ZoneSystem.OnZoneEnter(Zone zone) {
    AudioManager.PlayAmbient(zone.ambientSound);
}
```

### Music System

```csharp
// State-based music
private void Update() {
    if (inCombat && currentMusic != "battle") {
        CrossfadeTo("battle");
    } else if (!inCombat && currentMusic != "explore") {
        CrossfadeTo("explore");
    }
}
```

---

## 4. User Stories

### US-001: Combat Sounds
**Description:** As a player, I want to hear combat sounds so that fighting feels impactful.

**Acceptance Criteria:**
- [ ] Attack swing sounds on attack
- [ ] Hit sounds on contact
- [ ] Spell cast sounds on cast

### US-002: UI Feedback
**Description:** As a player, I want to hear UI sounds so that I know my clicks registered.

**Acceptance Criteria:**
- [ ] Click sound on button press
- [ ] Error sound on invalid action
- [ ] Success sound on completion

### US-003: Ambient Audio
**Description:** As a player, I want to hear ambient sounds so that zones feel alive.

**Acceptance Criteria:**
- [ ] Zone-specific ambient plays
- [ ] Sound continues across scenes
- [ ] Smooth transitions

### US-004: Background Music
**Description:** As a player, I want music so that the game has atmosphere.

**Acceptance Criteria:**
- [ ] Music plays during exploration
- [ ] Music changes in combat
- [ ] Volume controllable

---

## 5. Functional Requirements

- FR-1: Load sound assets from audio/
- FR-2: Play sound on event
- FR-3: Handle multiple simultaneous sounds (pool)
- FR-4: Volume controls (master, sfx, music)
- FR-5: Crossfade between tracks
- FR-6: Mute option in settings

---

## 6. Non-Goals

- No 3D spatial audio (deferred)
- No voice acting (deferred)
- No dynamic music composition
- No sound editor

---

## 7. Implementation Plan

### Week 1: Core System

| Day | Task | Deliverable |
|-----|------|-------------|
| 1-2 | AudioManager implementation | manager works |
| 3-4 | Sound pool management | pooling works |
| 5-7 | Volume controls | controls work |

### Week 2: Event Hooks

| Day | Task | Deliverable |
|-----|------|-------------|
| 8-10 | Combat sound hooks | sounds play |
| 11-12 | UI sound hooks | sounds work |
| 13-14 | Ambient system | ambience works |

### Week 3: Music

| Day | Task | Deliverable |
|-----|------|-------------|
| 15-17 | BGM integration | music plays |
| 18-19 | Crossfade system | fades work |
| 20-21 | Test and polish | all works |

---

## 8. Asset Requirements

### Sounds (placeholder OK)
- attack swing (1)
- hit (3 variants)
- block (1)
- cast (5 different for abilities)
- click (1)
- hover (1)
- success (1)
- error (1)

### Music
- explore (loop)
- battle (loop)
- village (loop)
- boss (loop)

### Ambient
- forest (loop)
- dungeon (loop)
- village (loop)

---

## 9. Resource Estimates

| Aspect | Estimate |
|--------|----------|
| Difficulty | Low |
| Time | 2 weeks |
| LOC | ~200 |
| Skills | Godot Audio, sound design |

---

## 10. Open Questions

1. **Q: Use placeholder sounds?**
   - A: Yes, generate or use free assets

2. **Q: License concerns?**
   - A: Use CC0 assets only

---

**PRD Status:** Proposed - Awaiting Implementation  
**Author:** OpenHands Analysis  
**Next Step:** Create AudioManager