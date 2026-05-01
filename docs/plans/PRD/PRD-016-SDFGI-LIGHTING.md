# PRD-016: SDFGI/SSAO/SSIL Post-Processing Lighting

**Version:** 1.0
**Status:** 🔄 Not Started — WorldEnvironment not configured (COMPATIBILITY_ANALYSIS: "Not configured")
**Owner:** RENDERING_AGENT
**Priority:** HIGH (P1 — Visual Coherence Requirement per COMPATIBILITY_ANALYSIS.md)
**Dependencies:** None (independent visual polish)

---

## 1. Overview

### 1.1 Purpose
Configure Godot 4.2's **SDFGI (Signed Distance Field Global Illumination)**, **SSAO (Screen Space Ambient Occlusion)**, and **SSIL (Screen Space Indirect Lighting)** to achieve AAA-quality lighting that meets the project's research alignment standards (UE5.5+ quality bar).

### 1.2 Scope
`src/client/` rendering configuration:
- Update `WorldEnvironment.tscn` or equivalent (likely embedded in `Player.tscn` or `Main.tscn`)
- Configure `Environment` resource with SDFGI/SSAO/SSIL
- Tune quality vs. performance balance for demo (60fps target)
- Documentation: `docs/lighting-setup.md`

**EXCLUDED:**
- Volumetric fog (post-MVP)
- Ray-traced reflections (requires RTX GPU, out of scope for demo)
- Dynamic weather system (future work)

---

## 2. Requirements

### 2.1 Functional Requirements
ID    | Requirement             | Priority | Target Value
------|-------------------------|----------|-------------
LIT-001 | SDFGI enabled            | P0       | On, quality = 2 (Balanced)
LIT-002 | SSAO enabled             | P0       | On, radius = 1.5, intensity = 0.75
LIT-003 | SSIL enabled             | P1       | On, quality = Medium
LIT-004 | Screen-space reflections| P2       | Off (too expensive) or quality=Low
LIT-005 | Tone mapping             | P0       | ACES (film look)
LIT-006 | Auto-exposure            | P1       | Enabled, adaptation rate 0.8
LIT-007 | Glow/Bloom               | P2       | Medium intensity (0.4)

### 2.2 Performance Budget
- Total GPU time for post-processing: **<4ms/frame** (at 1440p, GTX 1060+)
- Memory: Environment resource <1MB
- Must maintain **60fps lock** on minimum spec (GTX 1050 Ti, 4-core CPU)

---

## 3. Current Gap Analysis

**Current State:**
```
Search for WorldEnvironment:
  src/client/scenes/
    - Player.tscn: Has Camera3D, no WorldEnvironment visible
    - Main.tscn or Environment.tscn? → Need to check
    - No dedicated lighting configuration found

Project status claims:
  PROJECT_STATUS.md says: "SDFGI/SSAO pending" (under COMPATIBILITY_ANALYSIS section)
  → CONFIRMED: lighting not configured yet
```

**Current lighting:**
- Likely using default Godot environment (no GI, no SSAO)
- Terrain has checkerboard texture but no lighting quality
- No ambient occlusion → flat shadows

**Target quality:**
SDFGI provides diffuse global illumination (realistic light bounce).
SSAO adds contact shadows in crevices (depth-based).
SSIL adds indirect lighting refinement (screen-space).

---

## 4. Technical Implementation

### 4.1 WorldEnvironment Node Location

**Find/Create appropriate scene:**
```
Option A (preferred): src/client/scenes/Main.tscn
  - Top-level game scene with WorldEnvironment as child
  - Loaded at startup

Option B: src/client/scenes/WorldEnvironment.tscn (new file)
  - Single-purpose lighting scene
  - Autoloaded via ProjectSettings

Option C: Embedded in Player.tscn (test mode only)
  - Not suitable for final game (each player would have own env)
```

**Action:** Create `WorldEnvironment.tscn` and autoload it.

### 4.2 Environment Resource Configuration

**Godot Inspector → Environment → SDFGI:**
```
Mode: SDFGI
Quality: 2 (Balanced)
Probe Bias: 1.2
Half Resolution: false  // full res for quality
```

**Godot Inspector → Environment → SSAO:**
```
Radius: 1.5
Intensity: 0.75
Bias: 0.5
Distance: 2.0
Light Affect: 0.4 (subtle darkening)
```

**Godot Inspector → Environment → SSIL:**
```
Enabled: true
Quality: 1 (Medium)
Sharpness: 0.8 (avoid bleeding)
```

**Godot Inspector → Environment → Tone Mapping:**
```
Mode: ACES
Exposure: 1.0 (auto-exposure handles it)
```

### 4.3 File Deliverables

| File | Action | Details |
|------|--------|---------|
| `src/client/scenes/WorldEnvironment.tscn` | NEW | PackedScene with WorldEnvironment + Environment resource |
| `src/client/scenes/Main.tscn` | MODIFY | Add `WorldEnvironment` as child (if not present) |
| `docs/lighting-setup.md` | NEW | Configuration guide + performance tips |
| `tools/demo/client_instrumentation_validator.py` | MODIFY | Add lighting validation (SSAO enabled check) |

### 4.4 Tuning Playbook

```
Step 1: Enable SDFGI, set quality=1 (Fast)
  - Test on target machine: should be >55fps with all other effects
  - If <55fps, reduce SDFGI quality to 0 (Low) or disable → fallback to baked lighting

Step 2: Enable SSAO (radius=1.5, intensity=0.75)
  - Check edges for noise/banding → adjust radius if too aggressive
  - If performance tight, reduce SSAO intensity to 0.5

Step 3: Enable SSIL (quality=Medium)
  - Check for light bleeding → reduce intensity if needed

Step 4: Enable Tone Mapping (ACES)
  - Verify colors aren't oversaturated
  - Adjust exposure bias if scene too dark

Step 5: Profile (Godot profiler + tools/demo/instruments)
  - Post-process total <4ms? ✓ Valid
  - Post-process >6ms? → lower SDFGI quality or disable SSIL
```

---

## 5. Platform Fallbacks

**Low-end GPUs (GTX 1050 / integrated):**
- SDFGI → OFF (use baked lightmaps — but not in scope for MVP)
- SSAO → ON (low radius 1.0, intensity 0.5)
- SSIL → OFF
- Tone mapping → Filmic (lighter than ACES)

**High-end GPUs (RTX 3060+):**
- SDFGI → quality 3 (High)
- SSAO → radius 2.0, intensity 1.0
- SSIL → quality 2 (High)

**Detection:**
```csharp
// Auto-detect based on GPU
var adapter = RenderingServer.GetVideoAdapterName();
var isLowEnd = adapter.Contains("1050") || adapter.Contains("Intel");
ConfigureLightingForTier(isLowEnd ? Low : High);
```

---

## 6. Integration with Demo Validator

**tools/demo/client_instrumentation_validator.py — Lighting Check:**

Add new validation rule:
```python
def validate_lighting_setting(self, screenshot_path):
    """Check SDFGI/SSAO are enabled via resource inspection."""
    # Parse .tscn or use Godot's resource dump
    # Easier: check screenshot for SSAO artifacts (vignette darkening at edges)
    # But more reliable: add debug overlay showing SDFGI probes
    pass  # implement as part of demo validation
```

Alternative: Add debug command `F6` to dump Environment settings to console for automated check.

---

## 7. File Changes Summary

```
📁 src/client/scenes/
   ├── WorldEnvironment.tscn (NEW — 30 lines)
   └── Main.tscn (MODIFY — insert WorldEnvironment node)

📁 src/client/scenes/Main.tscn (or root game scene)
   [existing nodes...]
   └── WorldEnvironment (node)
        └── Environment (resource) ← configure here

📁 docs/lighting-setup.md (NEW — 200 lines)
  - Step-by-step config screenshots (omitted in text)
  - Performance tuning table
  - GPU tier fallback chart

📁 tools/demo/client_instrumentation_validator.py (MODIFY)
  - Add `validate_lighting_quality()` method
  - Check FPS stability with lighting on (no drops below 55fps)
```

---

## 8. Testing & Validation

### 8.1 Visual QA Checklist
- [ ] No visible banding in shadow transitions (SSAO smooth)
- [ ] Corners/environment crevices show subtle darkening (ambient occlusion working)
- [ ] Light bounce visible: sunlight on ground illuminates underside of objects (SDFGI)
- [ ] No flickering during movement (SSIL stable)
- [ ] Tone mapping renders colors naturally (no oversaturation)

### 8.2 Performance QA
```bash
# Run demo with lighting ON
tools/demo/run_demo.py --quick --check-fps

# Expected: 58-60fps average on GTX 1060+
# Minimum: 55fps (acceptable)
# Below 50fps → adjust settings (PRD-016a: Lighting Fallbacks)
```

### 8.3 Regression Tests
- Baseline: 2129 test cases (all passing) — no changes to server
- Client-only change → zero server test impact
- Only risk: new scene parsing warnings in headless mode

---

## 9. Acceptance Criteria

✅ **Functional Completeness**
- SDFGI enabled (quality >= 1) with visible light bounce
- SSAO enabled (intensity >= 0.5) with visible edge darkening
- Tone mapping (ACES) active
- Auto-exposure working (scene brightens/dims as camera moves)

✅ **Performance**
- Demo runs at ≥55fps average on GTX 1060 (baseline GPU)
- No frame spikes >5ms from post-processing
- Memory <1MB for Environment resource

✅ **Code Quality**
- WorldEnvironment.tscn loads without errors
- Zero warnings in Godot editor (no missing dependencies)
- Configuration documented in `docs/lighting-setup.md`

✅ **Integration**
- Lighting visible in both player cam and demo capture
- No art pipeline conflicts (no asset rebuild needed)
- Works on Windows + Linux (WSL2 display forwarding)

---

## 10. Dependencies & Blockers

**Hard Dependencies:**
- Godot 4.2 Mono build (already in use)
- GPU supporting SDFGI (Vulkan 1.2+)
- Developer access to GPU for tuning (cannot tune over SSH)

**Soft Dependencies:**
- Foot IK (PRD-011) — should be tuned with final lighting (shadows on feet)
- Combat FSM (PRD-008) — no direct dependency

---

## 11. Timeline

- **Day 1 (3h):** Create WorldEnvironment.tscn, configure SDFGI baseline
- **Day 2 (3h):** Tune SSAO/SSIL, integrate with Main.tscn
- **Day 3 (2h):** Performance profiling, fallback tiers, documentation
- **Total:** 8 agent-hours (parallelizable; independent of server work)

---

## 12. Post-MVP Extensions (Out of Scope)

- Volumetric fog & god rays
- Ray-traced global illumination (RTGI)
- Dynamic weather lighting changes
- Day/night cycle (requires dynamic sun + GI update)

These are **Phase 11+ visual polish** items not required for MVP.

---

**Prepared by:** Hermes Agent (gap analysis 2026-05-01)
**Next:** Assign to RENDERING_AGENT for lighting configuration
