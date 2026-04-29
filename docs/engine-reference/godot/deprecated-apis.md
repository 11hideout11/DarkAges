# Godot 4.2 - Deprecated APIs

If an agent suggests any API in the "Deprecated" column, use the "Use Instead" column.

## Nodes & Classes

| Deprecated | Use Instead | Since | Notes |
|------------|-------------|-------|-------|
| `TileMap` | `TileMapLayer` | 4.0 | Multi-layer node split into per-layer nodes |
| `VisibilityNotifier2D` | `VisibleOnScreenNotifier2D` | 4.0 | Renamed for clarity |
| `VisibilityNotifier3D` | `VisibleOnScreenNotifier3D` | 4.0 | Renamed |
| `YSort` | `Node2D.y_sort_enabled` | 4.0 | Property, not separate node |
| `Navigation2D` / `Navigation3D` | `NavigationServer2D` / `NavigationServer3D` | 4.0 | Server-based API |
| `PackedScene.instance()` | `PackedScene.instantiate()` | 4.0 | Renamed |

## Methods & Properties

| Deprecated | Use Instead | Since |
|------------|-------------|-------|
| `yield()` | `await signal_name` | 4.0 |
| `connect(signal, obj, method)` | `signal.connect(callable)` | 4.0 |
| `get_world()` | `get_world_3d()` or `get_world_2d()` | 4.0 |
| `OS.get_ticks_msec()` | `Time.get_ticks_msec()` | 4.0 |
| `OS.delay_msec()` | `await get_tree().create_timer(ms/1000).timeout` | 4.0 |
| `duplicate()` for nested resources | `duplicate_deep()` | 4.5 (but 4.2 still works) |

## Patterns (Not Deprecated but Discouraged)

| Pattern | Better Alternative |
|---------|-------------------|
| `$NodePath` in _process() | `@onready var node = GetNode<Node>("Path")` cached |
| String-based signal connections | Typed callable: `button.Pressed += OnButtonPressed` |
| Untyped `Array` / `Dictionary` | `Array[Type]`, typed variables |
| Manual post-process chains | Use `Compositor` + `CompositorEffect` (4.3+) — not available in 4.2; use CameraAttributes manually |
