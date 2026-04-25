extends SceneTree

func _init():
	var lib = AnimationLibrary.new()

	# Idle
	var idle = Animation.new()
	idle.length = 1.0
	idle.loop = true
	var t = idle.add_track(Animation.TrackType.TYPE_VALUE)
	idle.track_set_path(t, NodePath("Model:rotation"))
	idle.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	idle.track_set_loop_wrap(t, true)
	idle.track_insert_key(t, 0.0, Vector3(0,0,0))
	idle.track_insert_key(t, 0.5, Vector3(0,0.02,0))
	idle.track_insert_key(t, 1.0, Vector3(0,0,0))
	lib.add_animation("Idle", idle)

	# Walk
	var walk = Animation.new()
	walk.length = 0.8
	walk.loop = true
	t = walk.add_track(Animation.TrackType.TYPE_VALUE)
	walk.track_set_path(t, NodePath("Model:position"))
	walk.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	walk.track_set_loop_wrap(t, true)
	walk.track_insert_key(t, 0.0, Vector3(0,0.9,0))
	walk.track_insert_key(t, 0.2, Vector3(0,0.95,0))
	walk.track_insert_key(t, 0.4, Vector3(0,0.9,0))
	walk.track_insert_key(t, 0.6, Vector3(0,0.85,0))
	walk.track_insert_key(t, 0.8, Vector3(0,0.9,0))
	lib.add_animation("Walk", walk)

	# Run
	var run = Animation.new()
	run.length = 0.6
	run.loop = true
	t = run.add_track(Animation.TrackType.TYPE_VALUE)
	run.track_set_path(t, NodePath("Model:position"))
	run.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	run.track_set_loop_wrap(t, true)
	run.track_insert_key(t, 0.0, Vector3(0,0.9,0))
	run.track_insert_key(t, 0.15, Vector3(0,1.0,0))
	run.track_insert_key(t, 0.3, Vector3(0,0.9,0))
	run.track_insert_key(t, 0.45, Vector3(0,1.0,0))
	run.track_insert_key(t, 0.6, Vector3(0,0.9,0))
	lib.add_animation("Run", run)

	# Sprint
	var sprint = Animation.new()
	sprint.length = 0.5
	sprint.loop = true
	t = sprint.add_track(Animation.TrackType.TYPE_VALUE)
	sprint.track_set_path(t, NodePath("Model:position"))
	sprint.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	sprint.track_set_loop_wrap(t, true)
	sprint.track_insert_key(t, 0.0, Vector3(0,0.9,0))
	sprint.track_insert_key(t, 0.125, Vector3(0,1.05,0))
	sprint.track_insert_key(t, 0.25, Vector3(0,0.9,0))
	sprint.track_insert_key(t, 0.375, Vector3(0,1.05,0))
	sprint.track_insert_key(t, 0.5, Vector3(0,0.9,0))
	lib.add_animation("Sprint", sprint)

	# Jump
	var jump = Animation.new()
	jump.length = 0.8
	jump.loop = false
	t = jump.add_track(Animation.TrackType.TYPE_VALUE)
	jump.track_set_path(t, NodePath("Model:position"))
	jump.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	jump.track_set_loop_wrap(t, false)
	jump.track_insert_key(t, 0.0, Vector3(0,0.9,0))
	jump.track_insert_key(t, 0.2, Vector3(0,1.2,0))
	jump.track_insert_key(t, 0.4, Vector3(0,0.9,0))
	jump.track_insert_key(t, 0.6, Vector3(0,0.9,0))
	jump.track_insert_key(t, 0.8, Vector3(0,0.9,0))
	lib.add_animation("Jump", jump)

	# Attack
	var attack = Animation.new()
	attack.length = 0.5
	attack.loop = false
	t = attack.add_track(Animation.TrackType.TYPE_VALUE)
	attack.track_set_path(t, NodePath("Model:position"))
	attack.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	attack.track_set_loop_wrap(t, false)
	attack.track_insert_key(t, 0.0, Vector3(0,0.9,0))
	attack.track_insert_key(t, 0.1, Vector3(0,0.9,-0.2))
	attack.track_insert_key(t, 0.3, Vector3(0,0.9,-0.4))
	attack.track_insert_key(t, 0.5, Vector3(0,0.9,0))
	lib.add_animation("Attack", attack)

	# Dodge
	var dodge = Animation.new()
	dodge.length = 0.4
	dodge.loop = false
	t = dodge.add_track(Animation.TrackType.TYPE_VALUE)
	dodge.track_set_path(t, NodePath("Model:rotation"))
	dodge.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	dodge.track_set_loop_wrap(t, false)
	dodge.track_insert_key(t, 0.0, Vector3(0,0,0))
	dodge.track_insert_key(t, 0.2, Vector3(0,0,0.3))
	dodge.track_insert_key(t, 0.4, Vector3(0,0,0))
	lib.add_animation("Dodge", dodge)

	# Hit
	var hit = Animation.new()
	hit.length = 0.3
	hit.loop = false
	t = hit.add_track(Animation.TrackType.TYPE_VALUE)
	hit.track_set_path(t, NodePath("Model:position"))
	hit.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	hit.track_set_loop_wrap(t, false)
	hit.track_insert_key(t, 0.0, Vector3(0,0.9,0))
	hit.track_insert_key(t, 0.1, Vector3(0.1,1.0,0))
	hit.track_insert_key(t, 0.2, Vector3(-0.1,0.8,0))
	hit.track_insert_key(t, 0.3, Vector3(0,0.9,0))
	lib.add_animation("Hit", hit)

	# Death
	var death = Animation.new()
	death.length = 1.0
	death.loop = false
	t = death.add_track(Animation.TrackType.TYPE_VALUE)
	death.track_set_path(t, NodePath("Model:rotation"))
	death.track_set_interpolation_type(t, Animation.INTERPOLATION_LINEAR)
	death.track_set_loop_wrap(t, false)
	death.track_insert_key(t, 0.0, Vector3(0,0,0))
	death.track_insert_key(t, 0.5, Vector3(1.57,0,0))
	death.track_insert_key(t, 1.0, Vector3(1.57,0,0))
	lib.add_animation("Death", death)

	var err = ResourceSaver.save(lib, "res://scenes/PlayerAnimations.tres")
	if err != OK:
		print("SAVE FAILED: ", err)
	get_tree().quit(0)