#pragma once

#include <cstdint>

// Stats for benchmarking
struct PhononStats {
	int64_t tick_ns = 0; // tick duration
	int64_t raycast_ns = 0; // sum of all intersect_ray calls (only when profiling_enabled)
	int raycast_count = 0;
	int active_packets = 0;
};

