#!/usr/bin/env python3
"""Generate placeholder UI assets for DarkAges MMO demo.

Creates simple procedural textures that can be used as icons and UI elements
until final assets are sourced.
"""

import os
from PIL import Image, ImageDraw, ImageFilter

# Asset directories
BASE_DIR = "/root/projects/DarkAges/src/client/assets"
UI_DIR = os.path.join(BASE_DIR, "ui")
TEXTURE_DIR = os.path.join(BASE_DIR, "textures")

def ensure_dirs():
    for d in [UI_DIR, TEXTURE_DIR, os.path.join(UI_DIR, "icons"), os.path.join(UI_DIR, "ability")]:
        os.makedirs(d, exist_ok=True)

def create_ability_icon(name, color, shape="circle"):
    """Create a simple ability icon."""
    size = 64
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    if shape == "circle":
        draw.ellipse([4, 4, size-4, size-4], fill=color, outline=(255,255,255,255), width=2)
    elif shape == "square":
        draw.rectangle([4, 4, size-4, size-4], fill=color, outline=(255,255,255,255), width=2)
    elif shape == "diamond":
        points = [(size//2, 4), (size-4, size//2), (size//2, size-4), (4, size//2)]
        draw.polygon(points, fill=color, outline=(255,255,255,255), width=2)
    elif shape == "hexagon":
        import math
        r = size // 2 - 4
        cx, cy = size // 2, size // 2
        points = [(cx + r*math.cos(a), cy + r*math.sin(a)) for a in [0, math.pi/3, 2*math.pi/3, math.pi, 4*math.pi/3, 5*math.pi/3]]
        draw.polygon(points, fill=color, outline=(255,255,255,255), width=2)
    
    path = os.path.join(UI_DIR, "ability", f"{name}.png")
    img.save(path)
    print(f"Created: {path}")
    return path

def create_item_icon(name, color):
    """Create a simple item icon (square with rounded corners)."""
    size = 48
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Rounded rect
    r = 8
    draw.rounded_rectangle([2, 2, size-2, size-2], radius=r, fill=color, outline=(255,255,255,255), width=1)
    
    path = os.path.join(UI_DIR, "icons", f"{name}.png")
    img.save(path)
    print(f"Created: {path}")
    return path

def create_bar_texture(name, colors):
    """Create a health/energy bar texture."""
    size = (256, 32)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Background (dark)
    draw.rounded_rectangle([0, 0, size[0]-1, size[1]-1], radius=6, fill=colors[0])
    
    # Fill
    if colors[1]:
        w = int(size[0] * 0.7)
        draw.rounded_rectangle([2, 2, w, size[1]-2], radius=4, fill=colors[1])
    
    # Border
    draw.rounded_rectangle([0, 0, size[0]-1, size[1]-1], radius=6, outline=colors[2], width=2)
    
    path = os.path.join(TEXTURE_DIR, f"{name}.png")
    img.save(path)
    print(f"Created: {path}")
    return path

def create_portrait(name, color):
    """Create a simple NPC portrait for UI."""
    size = (128, 128)
    img = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Circle background
    draw.ellipse([4, 4, size[0]-4, size[1]-4], fill=color)
    
    # Simple face (circle)
    cx, cy = size[0]//2, size[1]//2
    draw.ellipse([cx-20, cy-25, cx+20, cy+25], fill=(240, 220, 200, 255))
    draw.ellipse([cx-8, cy-10, cx-2, cy-2], fill=(50, 30, 20, 255))
    draw.ellipse([cx+2, cy-10, cx+8, cy-2], fill=(50, 30, 20, 255))
    draw.arc([cx-10, cy+5, cx+10, cy+15], 0, 180, fill=(50, 30, 20, 255), width=2)
    
    path = os.path.join(UI_DIR, f"{name}.png")
    img.save(path)
    print(f"Created: {path}")
    return path

def main():
    ensure_dirs()
    print("Generating placeholder UI assets...")
    
    # Ability icons (key bindings 1-4)
    create_ability_icon("ability_1_attack", (200, 50, 50, 255), "circle")
    create_ability_icon("ability_2_fire", (200, 80, 30, 255), "hexagon")
    create_ability_icon("ability_3_heal", (50, 200, 80, 255), "circle")
    create_ability_icon("ability_4_shield", (80, 100, 200, 255), "diamond")
    create_ability_icon("ability_5_teleport", (150, 50, 200, 255), "square")
    create_ability_icon("ability_6_ultimate", (200, 180, 30, 255), "hexagon")
    
    # Item icons
    create_item_icon("item_potion_health", (200, 50, 50, 255))
    create_item_icon("item_potion_mana", (50, 50, 200, 255))
    create_item_icon("item_sword_iron", (180, 180, 180, 255))
    create_item_icon("item_armor_leather", (139, 90, 43, 255))
    create_item_icon("item_gold_coin", (255, 215, 0, 255))
    create_item_icon("item_key", (100, 100, 100, 255))
    
    # Bar textures
    create_bar_texture("health_bar", ((50, 20, 20, 255), (200, 50, 50, 255), (100, 100, 100, 255)))
    create_bar_texture("mana_bar", ((20, 20, 50, 255), (50, 50, 200, 255), (100, 100, 100, 255)))
    create_bar_texture("xp_bar", ((50, 40, 20, 255), (255, 180, 0, 255), (100, 100, 100, 255)))
    create_bar_texture("energy_bar", ((30, 50, 30, 255), (50, 200, 100, 255), (100, 100, 100, 255)))
    
    # Portrait placeholders
    create_portrait("portrait_player", (100, 120, 140, 255))
    create_portrait("portrait_merchant", (140, 100, 80, 255))
    create_portrait("portrait_guard", (80, 90, 120, 255))
    create_portrait("portrait_boss", (120, 50, 50, 255))
    
    print("\nAsset generation complete!")
    print(f"Base directory: {BASE_DIR}")

if __name__ == "__main__":
    main()