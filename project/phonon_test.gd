extends Node2D
## MVP test controls for the phonon_test scene.
## Space: emit again from the emitter. Left click: move the emitter to the mouse and emit.

@onready var emitter: PhononEmitter2D = $PhononEmitter2D
@onready var sensor: PhononSensor2D = $PhononSensor2D
@onready var listener: PhononListener2D = $PhononListener2D
@onready var label: Label = $Label
@onready var label2: Label = $Label2
@onready var label3: Label = $Label3

func _ready() -> void:
	# Debugging
	PhononServer.profiling_enabled = true
	Performance.add_custom_monitor("Phonon/tick_ms", func(): return PhononServer.get_stats().tick_ms / 1000.0, [], Performance.MONITOR_TYPE_TIME)
	Performance.add_custom_monitor("Phonon/active_packets", func(): return PhononServer.get_stats().active_packets)
	
	print("PhononServer.speed_of_sound = ", PhononServer.speed_of_sound)
	


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action("ui_accept"):
		emitter.play()

	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		emitter.global_position = event.position
		emitter.play()
	



func _process(_delta: float) -> void:
	label.text = "Active packets: %d\nSpace: emit   Click: emit at mouse" % PhononServer.get_active_packet_count()
	label2.text = "Sensor packets: %d\nListener voices playing: %d" % [sensor.get_hit_count(), listener.get_playing_voice_count()]

func _on_phonon_sensor_2d_triggered(condition: PhononSensorCondition, energy: float) -> void:
	label3.text = "Sensor detection TRIGGERED"
	print("trigger!")

func _on_phonon_sensor_2d_released(condition: PhononSensorCondition) -> void:
	label3.text = "Sensor detection"
