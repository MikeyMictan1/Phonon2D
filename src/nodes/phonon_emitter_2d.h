#pragma once

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

// Sound source. Set `stream` and call play(), like an AudioStreamPlayer2D, but it makes no sound itself:
// play() fires a ring of phonons, and the PhononListener2D plays `stream` when they reach it.
// (Not an AudioStreamPlayer2D subclass: that would also play the clip directly, ignoring walls and travel time.)
// MVP: omnidirectional only (an evenly spaced ring of packets). Cone/beam spread comes later.
class PhononEmitter2D : public Node2D {
	GDCLASS(PhononEmitter2D, Node2D)

public:
	void _ready() override;

	// Fires packet_count phonons outward in a ring from this node's global position. Bound to GDScript.
	void play();

	void set_stream(const Ref<AudioStream> &p_stream);
	Ref<AudioStream> get_stream() const;

	void set_packet_count(int p_count);
	int get_packet_count() const;

	void set_loudness(float p_loudness);
	float get_loudness() const;

	void set_frequency(float p_frequency);
	float get_frequency() const;

	void set_tag(const StringName &p_tag);
	StringName get_tag() const;

	void set_autoplay(bool p_enabled);
	bool get_autoplay() const;

protected:
	static void _bind_methods();

private:
	Ref<AudioStream> stream;   // the clip the listener plays when this sound's phonons arrive
	int packet_count = 180;    // packets per emission, spread evenly around 360 degrees
	float loudness = 1.0f;     // how loud the sound is: louder carries further (lives longer) and delivers more energy to receivers
	float frequency = 440.0f;  // Hz; gameplay label carried by packets for sensor conditions, not used by the physics (yet)
	StringName tag;            // what kind of sound, e.g. "footstep". Sensor conditions can filter on it
	bool autoplay = true;      // play() once automatically when the scene starts
};
