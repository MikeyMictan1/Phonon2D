extends Node2D
## MVP test controls for the phonon_test scene.
## Space: emit again from the emitter. Left click: move the emitter to the mouse and emit.

@onready var emitter: PhononEmitter2D = $PhononEmitter2D
@onready var label: Label = $Label


func _ready() -> void:
	print("PhononServer.speed_of_sound = ", PhononServer.speed_of_sound)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		emitter.emit_phonons()
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		emitter.global_position = event.position
		emitter.emit_phonons()


func _process(_delta: float) -> void:
	label.text = "Active packets: %d\nSpace: emit   Click: emit at mouse" % PhononServer.get_active_packet_count()
