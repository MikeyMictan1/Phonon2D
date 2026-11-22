extends Node2D
## MVP test controls for the phonon_test scene.
## Space: emit again from the emitter. Left click: move the emitter to the mouse and emit.
## R: switch raycaster (custom / godot). P: toggle per-ray profiling. G: draw baked walls.

@onready var emitter: PhononEmitter2D = $PhononEmitter2D
@onready var label: Label = $Label


func _ready() -> void:
	# Debugging
	PhononServer.profiling_enabled = false
	Performance.add_custom_monitor("Phonon/tick_ms", func(): return PhononServer.get_stats().tick_ms / 1000.0, [], Performance.MONITOR_TYPE_TIME)
	Performance.add_custom_monitor("Phonon/active_packets", func(): return PhononServer.get_stats().active_packets)
	
	print("PhononServer.speed_of_sound = ", PhononServer.speed_of_sound)
	
	


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action("ui_accept"):
		emitter.emit_phonons()

	elif event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_R:
				PhononServer.use_custom_raycast = not PhononServer.use_custom_raycast
			KEY_P:
				PhononServer.profiling_enabled = not PhononServer.profiling_enabled
			KEY_G:
				$PhononWorld2D.debug_draw_geometry = not $PhononWorld2D.debug_draw_geometry

	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		emitter.global_position = event.position
		emitter.emit_phonons()
	



func _process(_delta: float) -> void:
	var stats: Dictionary = PhononServer.get_stats()
	var ns_per_ray := 0.0
	if stats.raycast_count > 0:
		ns_per_ray = stats.raycast_ms * 1.0e6 / stats.raycast_count
	label.text = "Active packets: %d\nRaycaster: %s   (R to switch)\ntick: %.3f ms   raycast: %.3f ms   %.0f ns/ray   (profiling %s, P)\nwalls: %d static, %d moving   last bake: %.3f ms (%d bakes)\nSpace: emit   Click: emit at mouse   G: draw walls" % [
		stats.active_packets, stats.raycaster, stats.tick_ms, stats.raycast_ms, ns_per_ray,
		"on" if PhononServer.profiling_enabled else "off",
		stats.static_segments, stats.dynamic_segments, stats.last_bake_ms, stats.bake_count]
