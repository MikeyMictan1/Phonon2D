#pragma once

#include <godot_cpp/classes/object.hpp>

#include "server/phonon_packet_pool.h"

using namespace godot;

// Mediator singleton. Emitters hand it new phonons; it advances packets each
// tick, resolves collisions against materials and dispatches to receivers.
class PhononServer : public Object {
	GDCLASS(PhononServer, Object)

protected:
	static void _bind_methods();

public:
	// Singleton access and per-tick update added during implementation.
};
