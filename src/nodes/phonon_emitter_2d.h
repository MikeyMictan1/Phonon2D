#pragma once

#include <godot_cpp/classes/audio_stream_player2d.hpp>

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

	void set_intensity(float p_intensity);
	float get_intensity() const;

	void set_frequency(float p_frequency);
	float get_frequency() const;

	void set_emit_on_ready(bool p_enabled);
	bool get_emit_on_ready() const;

protected:
	static void _bind_methods();

private:
	int packet_count = 180;    // packets per emission, spread evenly around 360 degrees
	float intensity = 1.0f;    // starting energy of each packet
	float frequency = 440.0f;  // Hz; carried by packets, not used by the simulation yet
	bool emit_on_ready = true; // fire once automatically when the scene starts
};
