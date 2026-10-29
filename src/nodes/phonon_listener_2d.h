#pragma once

#include "nodes/phonon_receiver.h"

using namespace godot;

// Converts arriving phonons into the audio the player hears. One per scene.
class PhononListener2D : public PhononReceiver {
	GDCLASS(PhononListener2D, PhononReceiver)

protected:
	static void _bind_methods();
};
