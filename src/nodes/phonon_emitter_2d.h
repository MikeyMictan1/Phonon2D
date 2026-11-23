#pragma once

#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

// Sound source. Plays audio like a normal AudioStreamPlayer2D and also
// emits phonons into the PhononServer.
// MVP: omnidirectional only (an evenly spaced ring of packets). Cone/beam spread comes later.
class PhononEmitter2D : public AudioStreamPlayer2D {
	GDCLASS(PhononEmitter2D, AudioStreamPlayer2D)

public:
	void _ready() override;

	// Fires packet_count phonons outward in a ring from this node's global position. Bound to GDScript.
	void emit_phonons();

	void set_packet_count(int p_count);
	int get_packet_count() const;

	void set_loudness(float p_loudness);
	float get_loudness() const;

	void set_frequency(float p_frequency);
	float get_frequency() const;

	void set_tag(const StringName &p_tag);
	StringName get_tag() const;

	void set_emit_on_ready(bool p_enabled);
	bool get_emit_on_ready() const;

protected:
	static void _bind_methods();

private:
	int packet_count = 180;    // packets per emission, spread evenly around 360 degrees
	float loudness = 1.0f;     // how loud the sound is: louder carries further (lives longer) and delivers more energy to receivers
	float frequency = 440.0f;  // Hz; gameplay label carried by packets for sensor conditions, not used by the physics (yet)
	StringName tag;            // what kind of sound, e.g. "footstep". Sensor conditions can filter on it
	bool emit_on_ready = true; // fire once automatically when the scene starts
};
