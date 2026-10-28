#pragma once

#include <godot_cpp/classes/node2d.hpp>

using namespace godot;

// Shared base for anything that receives phonons (listener, sensor), so the
// server dispatches to one list. Registered abstract: can't be added to a scene.
class PhononReceiver : public Node2D {
	GDCLASS(PhononReceiver, Node2D)

protected:
	static void _bind_methods();

public:
	// receive_packet(packet) added during implementation.
};
