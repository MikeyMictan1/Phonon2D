#pragma once

#include <cstdint>

// Stats for benchmarking
struct PhononStats {
	int64_t tick_ns = 0; // tick duration
	int64_t raycast_ns = 0; // sum of all ray queries (Godot or custom) (only when profiling_enabled)
	int raycast_count = 0;
	int64_t dynamic_sync_ns = 0; // re-placing moving walls (custom raycaster only)
	int dynamic_bodies_moved = 0;
	int active_packets = 0;
};