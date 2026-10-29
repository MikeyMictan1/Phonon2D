#pragma once

#include "nodes/phonon_receiver.h"

using namespace godot;

// Gameplay node. Checks arriving phonons against conditions and emits signals.
class PhononSensor2D : public PhononReceiver {
	GDCLASS(PhononSensor2D, PhononReceiver)

protected:
	static void _bind_methods();
};
