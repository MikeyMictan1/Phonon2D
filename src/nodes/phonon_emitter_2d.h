#pragma once

#include <godot_cpp/classes/audio_stream_player2d.hpp>

using namespace godot;

// Sound source. Plays audio like a normal AudioStreamPlayer2D and also
// emits phonons into the PhononServer.
class PhononEmitter2D : public AudioStreamPlayer2D {
	GDCLASS(PhononEmitter2D, AudioStreamPlayer2D)

protected:
	static void _bind_methods();
};
