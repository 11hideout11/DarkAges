# DarkAges Art Pipeline - Tooling & Setup Guide

**Target:** Hermes autonomous agents + human artists  
**Platform:** WSL2 (Ubuntu Linux) on Windows  
**Engine:** Godot 4.2.2 Mono (C#)  
**Date:** 2026-04-29

---

## QUICK START

This document tells you **exactly which tools to install** and **how to configure them** for the DarkAges art pipeline.

**Time to complete setup:** 30-60 minutes (download bandwidth dependent)

---

## 1. PREREQUISITES CHECK

Verify current environment:

```bash
# In WSL2 terminal
echo "=== System ==="
uname -a
echo ""
echo "=== Disk space ==="
df -h /home /root
echo ""
echo "=== GPU ===" 
nvidia-smi  # if you have NVIDIA drivers installed
```

**Expected:**
- Ubuntu 22.04+ (WSL2)
- At least 20GB free disk space
- Optional: NVIDIA GPU for GPU-accelerated viewport in Blender

---

## 2. TOOL INSTALLATION

### 2.1 Blender 4.x (3D Modeling & Animation)

**Why Blender:** Free, open-source, industry-standard. Perfect for low-poly game assets.

**Recommended version:** 4.1 or 4.2 (LTS for stability)

**Install via Flatpak (easiest on WSL2 with X11):**

```bash
# Add Flathub repo
flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

# Install Blender
flatpak install flathub org.blender.Blender

# Run
flatpak run org.blender.Blender
```

**Alternative: AppImage (portable)**

```bash
cd /tmp
wget https://download.blender.org/release/Blender4.2/blender-4.2.0-linux-x64.tar.xz
tar -xf blender-4.2.0-linux-x64.tar.xz
sudo mv blender-4.2.0-linux-x64 /opt/blender
sudo ln -s /opt/blender/blender /usr/local/bin/blender
```

**Verify:**
```bash
blender --version
# Expected: Blender 4.2.x (sub 1, hash XXXXXXXX)
```

**Post-install Blender Configuration:**

1. Launch Blender → Preferences → Add-ons
2. Enable: `Auto-Rig Pro` (if installed separately)
3. Enable: `Node Wrangler` (bundled)
4. Enable: `Import-Export: Godot 4.2` (if available)

**Blender Add-ons Install:**

```bash
# Method 1: Blender's built-in add-on installer
# In Blender: Edit → Preferences → Add-ons → Install...
# Select .zip file

# Method 2: BlenderToGodot4Pipeline
cd /tmp
wget https://michaeljared.itch.io/blender-to-godot-4-pipeline-addon/download
# Follow instructions on page
```

**Blender configuration file backup location:**
```
~/.config/blender/4.2/config/
```

### 2.2 ArmorPaint (Texture Painting) [OPTIONAL BUT RECOMMENDED]

**Why ArmorPaint:** Paint roughness/metallic/normal simultaneously on 3D mesh - extremely efficient.

**Install:**

```bash
# Download AppImage from https://armorpaint.org/
cd /opt
sudo wget https://github.com/armory3d/armor/releases/download/HEAD/armor.zip
sudo unzip armor.zip
sudo chmod +x ArmorPaint.AppImage
sudo ln -s /opt/ArmorPaint.AppImage /usr/local/bin/armorpaint

# Run
armorpaint
```

**Alternative: Snap/Flatpak may be available**

**Verify:**
```bash
armorpaint --version
```

### 2.3 Material Maker (Procedural Textures) [OPTIONAL]

**Why:** Generate tiled textures, normal maps, roughness variation procedurally.

**Install:**

```bash
# Snap (if available)
sudo snap install material-maker

# Or AppImage
cd /opt
wget https://materialmaker.org/public/builds/Material_Maker_linux.AppImage
chmod +x Material_Maker_linux.AppImage
sudo mv Material_Maker_linux.AppImage /opt/material-maker
sudo ln -s /opt/material-maker /usr/local/bin/materialmaker
```

**Verify:**
```bash
materialmaker --help  # should not error
```

### 2.4 Godot 4.2 Mono (Editor)

**Already installed?** Verify:
```bash
godot --version
# Expected: 4.2.x (mono)
```

**If not installed:**
```bash
cd /tmp
wget https://github.com/godotengine/godot/releases/download/4.2.2-stable/Godot_v4.2.2-stable_mono_linux_x86_64.zip
unzip Godot_v4.2.2-stable_mono_linux_x86_64.zip
sudo mv godot /usr/local/bin/
sudo chmod +x /usr/local/bin/godot
```

**Verify:**
```bash
godot --version
# Godot 4.2.2.stable (mono) (c) 2014-present Godot Engine
```

### 2.5 Git LFS (Large File Storage)

**Critical** for binary .glb and textures.

```bash
# Install
sudo apt-get update
sudo apt-get install -y git-lfs

# Initialize in your user account
git lfs install

# Verify
git lfs version
# Expected: git-lfs/3.x.x
```

**Configure LFS tracking for DarkAges:**

```bash
cd /root/projects/DarkAges

# Track binary formats
git lfs track "*.glb"
git lfs track "*.png"
git lfs track "*.jpg"
git lfs track "*.tres"
git lfs track "*.tscn"
git lfs track "*.anim"
git lfs track "*.ogg"
git lfs track "*.wav"

# Commit .gitattributes
git add .gitattributes
git commit -m "chore(lfs): track binary art assets"
```

### 2.6 Additional Tools (Optional)

**ImageMagick** (texture batch conversion/resizing):
```bash
sudo apt-get install -y imagemagick
```

**Inkscape** (SVG → PNG for UI icons):
```bash
sudo apt-get install -y inkscape
```

**GIMP** (raster image editing):
```bash
sudo apt-get install -y gimp
```

---

## 3. PROJECT CONFIGURATION

### 3.1 Godot Import Preset Setup

**Standard GLTF Import for DarkAges:**

1. Start Godot editor: `godot --path src/client`
2. Open Project → Project Settings
3. Import tab → 3D Scene
4. Default settings for GLTF:

| Setting | Value | Notes |
|---------|-------|-------|
- Meshes → **Ensure Tangents** | ✓ | Required for normal maps  
- Meshes → **Generate LOD** | ✗ | Manual LOD for control
- Meshes → **Create Shadow Meshes** | ✓ (static) or ✗ (dynamic)
- Meshes → **Import Scale** | 1.0 | Blender meters = Godot meters
- Meshes → **Apply Modifiers** | ✓ (default)
- Meshes → **Import as** | Meshes (DEFAULT)
- Animations → **Import as** | AnimationPlayer
- Animations → **Keep Enabled** | ✓
- Animations → **Optimize** | ✓ (reduce keyframes)
- Animation clips → **Fix tilt X axis** | ✓ (Mixamo compatible)

**Create import template:**

`assets/templates/godot_import_settings.cfg`:
```ini
[3d_scene]
meshes/ensure_tangents=true
meshes/apply_modifiers=true
meshes/import_scale=1.0
animations/import_as=AnimationPlayer
animations/keep_enabled=true
animations/optimize=true
```

### 3.2 Asset Folder Hierarchy

**Create folder structure:**

```bash
cd /root/projects/DarkAges/src/client

mkdir -p assets/3d/characters
mkdir -p assets/3d/weapons  
mkdir -p assets/3d/environment
mkdir -p assets/3d/vfx
mkdir -p assets/textures/characters
mkdir -p assets/textures/ui
mkdir -p assets/textures/environment
mkdir -p assets/materials
mkdir -p assets/shaders
mkdir -p assets/audio
mkdir -p assets/icons

# Git-track new folders
git add assets/
git commit -m "chore(assets): create initial asset folder hierarchy"
```

**Structure verified:**
```bash
tree -L 3 src/client/assets
# Should show directories, not files
```

### 3.3 Blender Export Preset

**Create a custom export preset for Godot GLB:**

1. Open Blender
2. File → Export → glTF 2.0
3. Configure:
   - Format: `.glb` (Binary)
   - Include: Selected Objects
   - Apply Modifiers: ✓
   - UVs: ✓
   - Normals: ✓
   - Tangents: ✓
   - Vertex Colors: ✓ (if used)
   - Materials: Export
   - Images: Embed (single-file)
   - Animation: Include
   - Reset bone connections: ✓ (for Mixamo compatibility)
   - Force enable: Active clamp: 4 (max 4 bones per vertex)
4. Click **+** button to save preset as `Godot_Mobile_GLB`
5. Set as default

**Export verification:**
```bash
# Test export
blender --background --python /tmp/export_test.py
# Where export_test.py creates cube, exports to /tmp/test.glb
ls -lh /tmp/test.glb  # Should be < 100KB
```

### 3.4 Texture Import Defaults

**Set texture compression in Godot project:**

`project.godot` already has:
```
textures/vram_compression/import_etc2_astc=true
```

**Confirm settings:**

1. Open Godot editor
2. Import tab → select any PNG texture
3. Confirm:
   - **Lossless** for albedo/normal? No - Godot compresses
   - **Compress Mode:** VRAM Compressed
   - **Format:** BC7 / ETC2 / ASTC (auto-chosen)
   - **Mipmaps:** Enabled for albedo/roughness/metallic
   - **Mipmaps:** Disabled for normal maps (IMPORTANT)

---

## 4. VALIDATION PIPELINE

### 4.1 Test Asset Pipeline

**Create test asset to validate full pipeline:**

`tools/art_pipeline/test_export_import.py`:

```python
# Creates: cube with UV, exports to GLB, imports to Godot
# Validates: meshes, materials, scale
# Requires: Blender Python API

import bpy
import subprocess

# Create cube
bpy.ops.mesh.primitive_cube_add(size=2.0)
cube = bpy.context.active_object
cube.name = "TestCube"

# UV unwrap
bpy.ops.uv.smart_project(angle_limit=66, island_margin=0.02)

# Material
mat = bpy.data.materials.new("TestMat")
mat.use_nodes = True
cube.data.materials.append(mat)

# Export
bpy.ops.export_scene.gltf(
    filepath="/tmp/test_cube.glb",
    export_format='GLB',
    mesh_triangulate=True
)

print("Exported test_cube.glb")
```

Run:
```bash
cd /root/projects/DarkAges
blender --background --python tools/art_pipeline/test_export_import.py
ls -lh /tmp/test_cube.glb

# Import into Godot (copy to project)
cp /tmp/test_cube.glb src/client/assets/3d/test/
cd src/client && godot --headless --quit  # triggers import
```

**Expected outcome:**
- `test_cube.glb.import` file created
- No errors in `godot.log`
- `assets/3d/test/TestCube.tscn` can be instanced

### 4.2 Shader Compilation Check

**Create test shader:**

`assets/shaders/test_phong.gdshader`:
```glsl
shader_type spatial;

uniform vec4 albedo_color : source_color = vec4(1.0, 0.0, 0.0, 1.0);

void fragment() {
    ALBEDO = albedo_color.rgb;
    METALLIC = 0.0;
    ROUGHNESS = 0.5;
}
```

**Validate:**

```bash
# Use Godot headless to check
godot --headless --script tools/validate_shader.py -- shader=assets/shaders/test_phong.gdshader
# Should exit code 0 and print: Shader compiled successfully
```

### 4.3 Scene Integration Test

**Create test scene:**

`tools/art_pipeline/integration_test.py`:
```python
import subprocess
import os

project = "/root/projects/DarkAges"

# Verify test_cube.glb can be instanced
result = subprocess.run(
    ['godot', '--headless', '--script', 'tools/art_pipeline/test_load.gd',
     '--', '--path', 'src/client', '--scene', 'assets/3d/test/test_cube.glb'],
    capture_output=True, text=True
)
print(result.stdout)
print(result.stderr)
assert result.returncode == 0, "Failed to load test asset"
```

Run:
```bash
python3 tools/art_pipeline/integration_test.py
```

---

## 5. WORKFLOW INTEGRATION

### 5.1 Daily Workflow Scripts

**Script 1: Asset import batch**

`tools/art_pipeline/import_all.py`:
```python
#!/usr/bin/env python3
"""Re-import all changed .glb files in assets/"""

import os, subprocess

assets_dir = "src/client/assets"
for root, dirs, files in os.walk(assets_dir):
    for f in files:
        if f.endswith('.glb'):
            glb_path = os.path.join(root, f)
            # Touch .import to trigger reimport
            import_path = glb_path + '.import'
            if os.path.exists(import_path):
                os.utime(import_path, None)  # update timestamp
            print(f"Queued: {glb_path}")

print("Done. Restart Godot to see changes.")
```

**Script 2: Asset validator**

`tools/art_pipeline/validate_asset.py`:
```python
#!/usr/bin/env python3
"""Validate single asset meets spec"""

import sys, json, os
from pathlib import Path

def validate_character(asset_path):
    errors = []
    warnings = []
    
    # Check required files
    model_path = os.path.join(asset_path, 'model.glb')
    if not os.path.exists(model_path):
        errors.append("model.glb missing")
    
    textures = ['albedo.png', 'normal.png', 'roughness.png', 'metallic.png']
    for tex in textures:
        if not os.path.exists(os.path.join(asset_path, 'textures', tex)):
            warnings.append(f"Texture missing: {tex}")
    
    # Check tri count (would need Blender bpy)
    # Can parse GLTF JSON if .glb extracted
    
    return {"errors": errors, "warnings": warnings}

if __name__ == "__main__":
    asset = sys.argv[1]
    result = validate_character(asset)
    print(json.dumps(result, indent=2))
    sys.exit(1 if result['errors'] else 0)
```

**Script 3: Shader syntax checker**

`tools/art_pipeline/check_shader_syntax.py`:
```python
#!/usr/bin/env python3
"""Batch-compile all .gdshader files"""

import subprocess, glob, sys

shaders = glob.glob("src/client/shaders/**/*.gdshader", recursive=True)
failed = []

for shader in shaders:
    result = subprocess.run(
        ['godot', '--headless', '--editor', '--check-only', shader],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        failed.append((shader, result.stderr))
        print(f"✗ {shader}")
    else:
        print(f"✓ {shader}")

if failed:
    print("\n=== COMPILE ERRORS ===")
    for shader, err in failed:
        print(f"\n{shader}:\n{err}")
    sys.exit(1)
```

Make executable:
```bash
chmod +x tools/art_pipeline/*.py
```

### 5.2 Hot-Reload Workflow

**Fast iteration:**
1. Edit in Blender → Export GLB to `assets/temp/`
2. Run `./tools/art_pipeline/import_all.py`
3. Switch to Godot (Running) → scene auto-reloads if open
4. Test in `PlayerTest.tscn`

**Comparison view:**
```bash
# Side-by-side Blender + Godot
blender --geometry 800x600 assets/temp/model_v2.blend &
godot --path src/client --editor &
```

---

## 6. AUTOMATION

### 6.1 Hermes Agent Hooks

**Future: Autonomous iteration**

When Hermes is tasked with creating an asset via PRD spec:

1. **Parse spec** (.yaml asset definition)
2. **Generate placeholder** (box model with material placeholder)
3. **Commit as draft**
4. **Delegate to subagent** (Claude Code or Codex) with art task
5. **Subagent workflow:**
   - Load Blender via API or use procedural mesh (if closed-source)
   - If Claude Code: Cannot run Blender GUI → ASCII description only
   - If Codex CLI: Can write Blender Python scripts
6. **Art agent produces:**
   - .blend file (source) committed to repo (git-lfs)
   - Exported .glb + textures
   - Validation report: tri count, UV layout screenshot
7. **Hermes merges PR after:**
   - Asset validator passes
   - Demo run successful
   - No test regressions

**Current limitation:** Autonomous generation cannot run GUI Blender. Tools:
- **Blender Python API** via CLI: `blender --background --python script.py`
- **Procedural mesh** generation (Godot MeshDataTool) as placeholder
- **External agents** with GUI access (Codex with Xvfb?)

### 6.2 Asset Build Scripts

**Export all assets for release:**

`tools/build_assets.py`:
```python
#!/usr/bin/env python3
"""Pack all GLB assets into release bundle"""

import subprocess, os

release_dir = "build/release/assets"
os.makedirs(release_dir, exist_ok=True)

# Copy all .glb
result = subprocess.run(
    ['find', 'src/client/assets', '-name', '*.glb', '-exec', 'cp', '{}', release_dir, ';'],
    capture_output=True, text=True
)

print(f"Exported assets to {release_dir}")
subprocess.run(['du', '-sh', release_dir])
```

---

## 7. TROUBLESHOOTING Q&A

**Q: Blender export missing animations**
A: Ensure `Apply Modifiers` is checked, and `Animation` is ticked in export options. Armature must have actions stored in NLA or action editor.

**Q: Godot shows pink textures**
A: Check texture import settings:
- Albedo: sRGB ✓
- Normal: Filter: Linear, Mipmaps: Disabled ✓
- Re-import (select texture → Advanced → Reimport)

**Q: Model appears tiny or huge**
A: Check Blender scale. 1 Blender unit = 1 meter. If model imported at 0.01, scale it 100× in Godot or fix Blender export scale (set to 1.0).

**Q: Normals look broken/sharp**
A: In Blender, add Edge Split modifier before export, or Mark Sharp edges. Also ensure Tangents generated in import settings.

**Q: Animation bone flailing / misaligned**
A: Check bone naming. Godot expects snake_case (spine_01). Rename in Blender. Ensure bone hierarchy matches humanoid skeleton.

**Q: Performance drops when zone loads**
A: Check draw call count in Godot debugger (Performance → Rendering → Draw Calls in Frame). If > 200, merge meshes, use MultiMesh.

**Q: Asset doesn't appear in demo zoom**
A: Reference path mismatch. Godot path `res://assets/...` vs absolute path. Check scene `.tscn` references are res://-prefixed.

**Q: LFS files still in Git history (too large)**
A: Fix: `git lfs migrate import --include="*.glb,*.png" --everything`
   Force prune old objects: `git reflog expire --expire=now --all && git gc --prune=now`

**Q: Shader compile error: unexpected token**
A: Godot 4.2 uses GLSL ES 3.0 variant. Check `shader_type spatial;` at top. Uppercase keywords. Semicolons mandatory.

---

## 8. RESOURCES

### Official Documentation
- Godot 4.2 Docs: https://docs.godotengine.org/en/4.2/
- 3D Import (GLTF): https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/importing_3d_scenes.html
- Shaders: https://docs.godotengine.org/en/stable/tutorials/shaders/
- Performance: https://docs.godotengine.org/en/stable/tutorials/performance/optimizing_for_3d.html

### Blender
- Blender Manual: https://docs.blender.org/manual/en/latest/
- GLTF Export Best Practices: https://docs.blender.org/manual/en/latest/addons/io_scene_gltf2.html

### Community
- Godot Forums: https://godotforums.org/
- Reddit: r/godot
- Discord: Godot Official Discord

### Tutorials
- GDQuest YouTube (Godot 4.2 tutorials)
- HeartBeast (action RPG)
- KidsCanCode (networking patterns)

---

## 9. CHECKLIST (Before Starting Art Work)

**Environment:**
- [ ] Blender 4.1+ installed and launches
- [ ] Godot 4.2.2 mono installed and launches
- [ ] Git LFS installed and configured for repo
- [ ] `/root/projects/DarkAges/src/client/assets/` folders created

**Configuration:**
- [ ] Blender export preset saved as `Godot_Mobile_GLB`
- [ ] Godot import default settings verified (tangents on, scale 1.0)
- [ ] Asset directory added to Godot project (FileSystem dock)
- [ ] Test cube export → import → instancing verified

**Toolchain Validation:**
```bash
# Run validation
blender --version
godot --version
git lfs version

# Execute test pipeline
python3 tools/art_pipeline/test_export_import.py
python3 tools/art_pipeline/integration_test.py
```

**Expected output:** All ✓ green checks, zero errors

---

## 10. SUPPORT & ESCALATION

**If stuck:**
1. Check this doc's Troubleshooting section
2. Search Godot documentation (linked above)
3. Search Godot Forums (issue likely solved)
4. Open issue: `docs/ART_PIPELINE_ISSUES.md` (template provided)

**Common failure modes:**
- Missing dependencies → re-install via apt
- Path issues (WSL mount points) → use Linux-native paths not /mnt/c
- File permission errors → `sudo chown -R $USER:$USER assets/`
- GPU not available in WSL → use software rendering (not ideal for Blender)

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-29  
**Next Review:** After initial test asset pipeline validated

**Maintained by:** Hermes Agent autonomous workflow  
**Reviewed by:** Iam (human-in-the-loop for PRD phase)
