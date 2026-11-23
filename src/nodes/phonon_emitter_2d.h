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

	void set_loudness(float p_loudness);
	float get_loudness() const;

	void set_band_low(float p_value);
	float get_band_low() const;

	void set_band_mid(float p_value);
	float get_band_mid() const;

	void set_band_high(float p_value);
	float get_band_high() const;

	void set_pitch(float p_pitch);
	float get_pitch() const;

	void set_emit_on_ready(bool p_enabled);
	bool get_emit_on_ready() const;

protected:
	static void _bind_methods();

private:
	int packet_count = 180;    // packets per emission, spread evenly around 360 degrees
	float loudness = 1.0f;     // total energy of one emission, shared across all packets (each gets loudness / packet_count)

	// Band mix: how the sound's energy is spread across low/mid/high. Relative amounts:
	// normalised to sum to 1 at emit time, so (2, 1, 1) and (0.5, 0.25, 0.25) are the same sound.
	float band_low = 1.0f;
	float band_mid = 1.0f;
	float band_high = 1.0f;

	float pitch = 440.0f;      // Hz; gameplay label carried by packets for sensor conditions, not used by the physics
	bool emit_on_ready = true; // fire once automatically when the scene starts
};
