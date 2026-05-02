# DarkAges MMO - Documentation Index

**Welcome to the DarkAges technical documentation hub.**

This index organizes all project documentation by domain. Use this as your entry point.

---

## 🎯 QUICK START

**New contributor?** Read these in order:

1. **Project Overview** → `docs/IMPLEMENTATION_SUMMARY.md`
2. **Current Status** → `docs/DOCUMENT_INDEX.md` (you are here)
3. **Art Pipeline** (if contributing art) → `docs/ART_PIPELINE_EXECUTIVE.md`
4. **Network Protocol** → `docs/NETWORK_PROTOCOL.md`
5. **Database Schema** → `docs/DATABASE_SCHEMA.md`
6. **Testing** → `docs/TESTING_AND_VALIDATION_PLAN.md`

---

## 📚 DOMAIN INDEX

### Architecture & Planning
| Document | Purpose | Last Updated |
|----------|---------|--------------|
| `IMPLEMENTATION_SUMMARY.md` | High-level system architecture, tech stack, core systems | Phase 0↑ |
| `NETWORK_PROTOCOL.md` | UDP packet format, RPC handshake, snapshot compression | Phase 9↑ |
| `DATABASE_SCHEMA.md` | Redis schema, state persistence, session management | Phase 6↑ |
| `API_CONTRACTS.md` | Server-client REST/WebSocket endpoints | Phase 6↑ |

### Build & DevOps
| Document | Purpose | Key Commands |
|----------|---------|--------------|
| `docs/PROJECT_STATUS.md` | Current phase-by-phase status dashboard | → View |
| `PHASE_6_12_ROADMAP.md` | 6-12 roadmap (GNS hardening, auth, persistence) | → View |
| `SCYLLA_IMPLEMENTATION.md` | Database migration plan (Redis→ScyllaDB) | → View |
| `TESTING_AND_VALIDATION_PLAN.md` | Test strategy, coverage, CI/CD | `ctest -j8` |

### **ART PIPELINE** ← YOU ARE HERE
All art pipeline documentation is consolidated under this section.

**Start here:** `docs/ART_PIPELINE_EXECUTIVE.md`

**Complete set:**
| Document | Size | Purpose |
|----------|------|---------|
| `ART_PIPELINE_SPECIFICATION.md` | 33 KB | 📖 Full technical specification (read first) |
| `ART_PIPELINE_ASSET_INVENTORY.md` | 20 KB | ✅ Production tracker with Tier 1-4 checklists |
| `ART_PIPELINE_TOOLING.md` | 18 KB | 🔧 Tool installation + validation scripts |
| `ART_PIPELINE_EXECUTIVE.md` | 13 KB | 📋 Quick start guide |
| `RESEARCH_CORPUS_INDEX.md` | 11 KB | 🔍 Research documentation meta-index |
| `assets/manifest.json` | 38 KB | 💾 Machine-readable asset database |

**Total art pipeline corpus:** 133 KB of documentation + 38 KB manifest = **171 KB**

---

## 🎨 ART PIPELINE QUICK LINKS

### Immediate Actions (Today)
- [ ] Install toolchain: `docs/ART_PIPELINE_TOOLING.md` Section 2
- [ ] Validate tools: Section 4.1 commands
- [ ] Read spec Sections 1-3: requirements + import pipeline
- [ ] Pick first asset from `ART_PIPELINE_ASSET_INVENTORY.md` Tier 1

### Asset Manifest
- `assets/manifest.json` → all assets in machine-readable format
- Load this JSON into scripts for automation
- ID = asset_id used in filenames and Git branches

### Asset Categories (by priority)

**Tier 1 CRITICAL (MVP blockers):**
- `char_player_male` → male player base model
- `weapon_sword_1h` → primary weapon
- `env_training_floor` → Zone98 foundation
- `vfx_damage_number` → combat feedback
- `vfx_hit_flash_screen` → hit response

**Tier 2 HIGH:**
- `npc_guard`, `weapon_sword_2h`, `weapon_axe_1h`
- `env_wall_straight_4m`, VFX outline shader, UI health bar

**Tier 3 MEDIUM:**
- `boss_creature`, armor sets, foliage, particle systems

**Tier 4 LOW / Deferred:**
- `char_player_female` (deferred), audio assets (deferred)

---

## 📖 DOCUMENT CONVENTIONS

### File Naming
- **Spec docs:** UPPER_SNAKE_CASE with descriptive name
- **Manifest/Config:** lowercase_snake_case (JSON, YAML)
- **Reference material:** lowercase with hyphens in skill `references/`
- **Raw research:** lowercase with underscores (historical artifacts)

### Section Markers
Documents include these standardized sections:
- **QUICK START** - fastest path to first action
- **OVERVIEW** - what the doc covers
- **REQUIREMENTS** - preconditions to use this doc
- **WORKFLOW** - step-by-step process
- **TROUBLESHOOTING** - common issues Q&A
- **APPENDIX** - reference tables, glossaries

### Cross-References
Documents reference each other by relative path from docs/:
- See also: `docs/ART_PIPELINE_SPECIFICATION.md#section-7`
- Asset inventory: `docs/ART_PIPELINE_ASSET_INVENTORY.md`
- Tool installation: `docs/ART_PIPELINE_TOOLING.md`

---

## 🔄 DOCUMENT LIFECYCLE

### When to Update Each Doc

| Document | Update Trigger | Owner |
|----------|---------------|--------|
- `SPECIFICATION.md` | New asset type, format change, toolchain version bump | Hermes Agent (auto) + Iam (review)
- `ASSET_INVENTORY.md` | Asset status changes (missing → in_progress → complete) | Hermes Agent (auto) + Artist (manual)
- `TOOLING.md` | Tool install/version changes, new validation script | Hermes Agent (auto)
- `manifest.json` | Every asset creation/modification | Hermes Agent (auto via manifest_update)
- `RESEARCH_CORPUS_INDEX.md` | New research compilation (rare) | Hermes Agent (auto)

**Update workflow:**
1. Work on asset in Blender/Godot
2. Run validation script (if available)
3. Update `ASSET_INVENTORY.md` checklist ticked ✓
4. Update `manifest.json` status field
5. Git commit: "feat(asset): char_player_male rigged - status complete"
6. PR → merge → docs auto-sync

---

## 🎯 ART PIPELINE MILESTONES

### Milestone 1: Foundation Complete (Week 4)
- [ ] All tools installed + validated
- [ ] Player male model exported + imported to Godot
- [ ] 4 PBR texture maps applied
- [ ] Tri count 12K-18K verified
- [ ] Rig 32 bones working
- [ ] 6 animations (idle, walk, run, jump_start, jump_loop, jump_land)

**Evidence:**
- `assets/manifest.json` status: `complete`
- `docs/ART_PIPELINE_ASSET_INVENTORY.md` checklist ticked ✓
- Screenshot: `docs/art_evidence/week4_player_complete.png`
- Demo run: Zone98 loads, player visible, no errors

### Milestone 2: Combat Ready (Week 8)
- [ ] Weapon 1H integrated into Player.tscn
- [ ] Combat FSM hitboxes active (Combat FSM PR #28)
- [ ] Attack animation (0.6s) + hitbox timing aligned
- [ ] VFX shaders: damage numbers + hit flash
- [ ] NPC guard in Zone99, using same animations

**Evidence:**
- Combat log: `Entity NPC killed by Player` events visible
- Demo: `/demo --server-only --npcs 5 --duration 30` shows hits
- Asset evidence: screenshot with damage numbers

### Milestone 3: Zones Populated (Week 12)
- [ ] Zone98 complete (tutorial arena)
- [ ] Zone99 complete (combat arena) with props
- [ ] Zone100 complete (boss pit) + boss creature
- [ ] Performance: draw calls < 150, 60fps sustained

**Evidence:**
- Full demo run (`--full --duration 60`) completes
- `docs/PHASE_8_COMPLETION_REPORT.md` (new doc) with screenshots
- All Tier 1 & 2 assets status: complete

---

## 🛠️ SKILLS REFERENCE

**Hermes skills relevant to art pipeline:**

| Skill | Purpose | Load When |
|-------|---------|-----------|
- `darkages-art-pipeline` | This art pipeline spec | ANY art/asset task ✓
- `godot-42-csharp-fixes` | Godot C# build issues | import fails, build errors
- `godot-networking-physics-fixes` | Physics sync | hitbox misalignment
- `godot-remote-player-scene-fixes` | RemotePlayer visibility | NPC not rendering

Load with: `skill_view(name='darkages-art-pipeline')` before any art work.

---

## 📊 ART PIPELINE STATISTICS

**Asset counts by category:**
```
Characters:  5  (player 2, npc 2, boss 1)
Weapons:     6  (sword 1H/2H, axe, spear, shield)
Environment: 10 (walls, rocks, trees, grass, props, terrain)
VFX:         8  (shaders + particles)
Textures:    6  (atlases + sets)
UI Icons:    8  (ability, HUD, buffs)
Total:      43  distinct assets (Tier 1-4 combined)
```

**Production hours estimate:**
- Tier 1: 40 + 6*6 + 5 + 15 = **~121 hours**
- Tier 2: 35 + 40 + 10 + 30 = **~115 hours**
- Tier 3: 60 + 20 + 30 = **~110 hours**
- **Total:** ~346 hours = ~8.5 weeks with 1 artist

**With 2 parallel tracks:** Character + Weapon tracks independent → 5 weeks

---

## 🔗 EXTERNAL RESOURCES

### Asset Libraries
- **Godot Asset Library** (built-in): Editor → AssetLib tab
- **Quaternius**: https://quaternius.com/ (CC0, Godot-ready)
- **Kenney.nl**: https://kenney.nl/assets/medieval-kit
- **Poly Haven**: https://polyhaven.com/ (PBR textures, HDRIs)
- **AmbientCG**: https://ambientcg.com/ (free PBR materials)

### Tools
- **Blender**: https://www.blender.org/download/
- **ArmorPaint**: https://armorpaint.org/
- **Material Maker**: https://materialmaker.org/

### Documentation
- Godot 4.2: https://docs.godotengine.org/en/4.2/
- GLTF Spec: https://registry.khronos.org/glTF/
- PBR Guide: https://learnopengl.com/PBR

---

## 🐛 ISSUES & IMPROVEMENTS

**Found a problem in this documentation?**

1. Check `docs/ART_PIPELINE_ISSUES.md` if it exists
2. Update the spec + commit with `fix(docs):` prefix
3. PR against `main` branch with evidence of corrected workflow

**Documentation drift:** If real process differs from spec, update spec immediately - do NOT work around outdated docs.

---

## 📝 CHANGE LOG

| Date | Version | Changes | Author |
|------|---------|---------|--------|
| 2026-04-29 | 1.0.0 | Initial complete specification package | Hermes Agent |

**Migration path:** None (new document suite)

---

**Last updated:** 2026-05-02  
**Status:** Phase 7-9 Complete (build, tests, performance) | Phase 10-11 Security & Chaos Testing In Progress  
**Maintainer:** Hermes Agent (autonomous) + Iam (review)

**Ready to begin art production.** Start with `docs/ART_PIPELINE_TOOLING.md` Installation section.
