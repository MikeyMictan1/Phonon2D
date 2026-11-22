#include "raycast/phonon_geometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

using godot::Transform2D;
using godot::Vector2;

namespace {
// A hit closer than this is ignored. Stops a packet that has just bounced from re-hitting the wall it
// is leaving (BOUNCE_OFFSET already moves it off, this is a second safety net).
constexpr float MIN_HIT_DISTANCE = 1e-4f;
// Slack on the "is the hit within the segment" test, as a fraction of segment length. Stops packets
// leaking through the tiny gap where two segments meet at a corner due to float rounding.
constexpr float SEGMENT_END_SLACK = 1e-4f;
// Segments are sorted into every cell their (slightly grown) bounding box touches, so a segment lying
// exactly on a cell border lands in the cells on both sides.
constexpr float CELL_BOX_GROW = 1e-3f;
// Upper bound on grid cells. A huge level grows the cell size instead of eating memory.
constexpr std::size_t MAX_CELLS = 1u << 22; // ~4M cells, 16 MB of cell_start
constexpr float INF = std::numeric_limits<float>::infinity();

// 2D cross product written out here so it always inlines (godot-cpp's Vector2::cross lives in a .cpp)
inline float cross(const Vector2 &p_a, const Vector2 &p_b) { return p_a.x * p_b.y - p_a.y * p_b.x; }
inline float dot(const Vector2 &p_a, const Vector2 &p_b) { return p_a.x * p_b.x + p_a.y * p_b.y; }
} // namespace

// ---- Building ----

void PhononGeometry::clear() {
	static_segments.clear();
	dynamic_segments.clear();
	dynamic_bodies.clear();
	cell_start.clear();
	cell_items.clear();
	grid_width = 0;
	grid_height = 0;
}

void PhononGeometry::add_static_segment(const Vector2 &p_a, const Vector2 &p_b, float p_absorption) {
	if (p_a.is_equal_approx(p_b)) {
		return; // zero-length edge can never be hit
	}
	static_segments.emplace_back(p_a, p_b, p_absorption);
}

void PhononGeometry::add_dynamic_body(uint64_t p_object_id, const Transform2D &p_global, std::vector<WallSegment> &&p_local_segments) {
	DynamicBody body;
	body.object_id = p_object_id;
	body.first = static_cast<uint32_t>(dynamic_segments.size());
	body.local_segments = std::move(p_local_segments);
	dynamic_segments.resize(dynamic_segments.size() + body.local_segments.size());
	dynamic_bodies.push_back(std::move(body));
	update_dynamic_body(dynamic_bodies.size() - 1, p_global);
}

void PhononGeometry::update_dynamic_body(std::size_t p_index, const Transform2D &p_global) {
	DynamicBody &body = dynamic_bodies[p_index];
	body.last_transform = p_global;
	for (std::size_t i = 0; i < body.local_segments.size(); i++) {
		const WallSegment &local = body.local_segments[i];
		dynamic_segments[body.first + i] = WallSegment(p_global.xform(local.a), p_global.xform(local.b), local.absorption);
	}
}

// Sort static segments into a uniform grid. Done in two passes over the segments (count, then fill)
// so the whole grid is two flat arrays with no per-cell allocations.
void PhononGeometry::build(float p_cell_size) {
	cell_start.clear();
	cell_items.clear();
	grid_width = 0;
	grid_height = 0;
	if (static_segments.empty()) {
		return;
	}

	// Bounding box of all static walls
	Vector2 min_corner(INF, INF);
	Vector2 max_corner(-INF, -INF);
	for (const WallSegment &segment : static_segments) {
		min_corner = min_corner.min(segment.a).min(segment.b);
		max_corner = max_corner.max(segment.a).max(segment.b);
	}
	min_corner -= Vector2(1, 1); // small margin so edges on the border are safely inside
	max_corner += Vector2(1, 1);
	const Vector2 extent = max_corner - min_corner;

	cell_size = std::max(p_cell_size, 1.0f);
	grid_width = std::max(1, static_cast<int>(std::ceil(extent.x / cell_size)));
	grid_height = std::max(1, static_cast<int>(std::ceil(extent.y / cell_size)));
	const std::size_t cells = static_cast<std::size_t>(grid_width) * grid_height;
	if (cells > MAX_CELLS) {
		cell_size *= std::sqrt(static_cast<float>(cells) / MAX_CELLS) * 1.01f;
		grid_width = std::max(1, static_cast<int>(std::ceil(extent.x / cell_size)));
		grid_height = std::max(1, static_cast<int>(std::ceil(extent.y / cell_size)));
	}
	grid_origin = min_corner;

	auto cell_x = [&](float x) { return std::clamp(static_cast<int>(std::floor((x - grid_origin.x) / cell_size)), 0, grid_width - 1); };
	auto cell_y = [&](float y) { return std::clamp(static_cast<int>(std::floor((y - grid_origin.y) / cell_size)), 0, grid_height - 1); };

	// Calls p_func(cell_index) for every cell a segment's bounding box overlaps
	auto for_each_cell = [&](const WallSegment &segment, auto &&p_func) {
		const int x0 = cell_x(std::min(segment.a.x, segment.b.x) - CELL_BOX_GROW);
		const int x1 = cell_x(std::max(segment.a.x, segment.b.x) + CELL_BOX_GROW);
		const int y0 = cell_y(std::min(segment.a.y, segment.b.y) - CELL_BOX_GROW);
		const int y1 = cell_y(std::max(segment.a.y, segment.b.y) + CELL_BOX_GROW);
		for (int y = y0; y <= y1; y++) {
			for (int x = x0; x <= x1; x++) {
				p_func(static_cast<std::size_t>(y) * grid_width + x);
			}
		}
	};

	// Pass 1: count segments per cell
	const std::size_t cell_count = static_cast<std::size_t>(grid_width) * grid_height;
	cell_start.assign(cell_count + 1, 0);
	for (const WallSegment &segment : static_segments) {
		for_each_cell(segment, [&](std::size_t cell) { cell_start[cell + 1]++; });
	}
	// Prefix sum: cell_start[i] becomes where cell i's run begins
	for (std::size_t i = 0; i < cell_count; i++) {
		cell_start[i + 1] += cell_start[i];
	}

	// Pass 2: write segment indices into each cell's run
	cell_items.resize(cell_start[cell_count]);
	std::vector<uint32_t> cursor(cell_start.begin(), cell_start.end() - 1);
	for (uint32_t index = 0; index < static_segments.size(); index++) {
		for_each_cell(static_segments[index], [&](std::size_t cell) { cell_items[cursor[cell]++] = index; });
	}
}

// ---- Querying ----

// Ray P + t*D against segment A + u*(B - A). Setting them equal and taking 2D cross products of
// both sides gives t and u directly. Hit if the crossing is on the segment (0 <= u <= 1) and ahead
// of the ray within range (0 < t <= max).
bool PhononGeometry::intersect(const WallSegment &p_segment, const Vector2 &p_from, const Vector2 &p_dir, float p_max_distance, float &r_t) {
	const Vector2 edge = p_segment.b - p_segment.a;
	const float denom = cross(p_dir, edge);
	if (std::abs(denom) < 1e-9f) {
		return false; // parallel: never crosses
	}
	const Vector2 to_start = p_segment.a - p_from;
	const float t = cross(to_start, edge) / denom;
	if (t < MIN_HIT_DISTANCE || t > p_max_distance) {
		return false;
	}
	const float u = cross(to_start, p_dir) / denom;
	if (u < -SEGMENT_END_SLACK || u > 1.0f + SEGMENT_END_SLACK) {
		return false;
	}
	r_t = t;
	return true;
}

// Walk the grid cells along the ray in order (Amanatides & Woo "grid DDA"). In each cell, test its
// segments. Stop as soon as the best hit so far is before the point where the ray leaves the cell:
// nothing in a later cell can be closer.
bool PhononGeometry::cast_static(const Vector2 &p_from, const Vector2 &p_dir, float p_max_distance, float &r_t, const WallSegment *&r_segment) const {
	if (grid_width == 0) {
		return false;
	}

	// Clip the ray to the grid's box (slab test), so rays starting outside the walls' area still work
	const Vector2 box_min = grid_origin;
	const Vector2 box_max = grid_origin + Vector2(grid_width * cell_size, grid_height * cell_size);
	float t_enter = 0.0f;
	float t_exit = p_max_distance;
	for (int axis = 0; axis < 2; axis++) {
		const float origin = p_from[axis];
		const float dir = p_dir[axis];
		if (std::abs(dir) < 1e-12f) {
			if (origin < box_min[axis] || origin > box_max[axis]) {
				return false; // parallel to this slab and outside it
			}
			continue;
		}
		float t_a = (box_min[axis] - origin) / dir;
		float t_b = (box_max[axis] - origin) / dir;
		if (t_a > t_b) {
			std::swap(t_a, t_b);
		}
		t_enter = std::max(t_enter, t_a);
		t_exit = std::min(t_exit, t_b);
		if (t_enter > t_exit) {
			return false; // misses the grid entirely
		}
	}

	// Starting cell
	const Vector2 start = p_from + p_dir * t_enter;
	int cx = std::clamp(static_cast<int>(std::floor((start.x - grid_origin.x) / cell_size)), 0, grid_width - 1);
	int cy = std::clamp(static_cast<int>(std::floor((start.y - grid_origin.y) / cell_size)), 0, grid_height - 1);

	// Per axis: which way we step, the ray distance to the next cell border, and the distance between borders
	auto setup_axis = [&](float dir, float from, float origin, int cell, int &step, float &t_next, float &t_delta) {
		if (dir > 0.0f) {
			step = 1;
			t_next = (origin + (cell + 1) * cell_size - from) / dir;
			t_delta = cell_size / dir;
		} else if (dir < 0.0f) {
			step = -1;
			t_next = (origin + cell * cell_size - from) / dir;
			t_delta = -cell_size / dir;
		} else {
			step = 0;
			t_next = INF;
			t_delta = INF;
		}
	};
	int step_x, step_y;
	float t_next_x, t_next_y, t_delta_x, t_delta_y;
	setup_axis(p_dir.x, p_from.x, grid_origin.x, cx, step_x, t_next_x, t_delta_x);
	setup_axis(p_dir.y, p_from.y, grid_origin.y, cy, step_y, t_next_y, t_delta_y);

	float best_t = p_max_distance;
	const WallSegment *best_segment = nullptr;

	while (true) {
		const std::size_t cell = static_cast<std::size_t>(cy) * grid_width + cx;
		for (uint32_t k = cell_start[cell]; k < cell_start[cell + 1]; k++) {
			const WallSegment &segment = static_segments[cell_items[k]];
			float t;
			if (intersect(segment, p_from, p_dir, best_t, t)) {
				best_t = t;
				best_segment = &segment;
			}
		}

		const float t_leave_cell = std::min(t_next_x, t_next_y);
		if (best_segment != nullptr && best_t <= t_leave_cell) {
			break; // found a hit inside this cell: later cells are all further away
		}
		if (t_leave_cell >= t_exit) {
			break; // the ray ends (or leaves the grid) inside this cell
		}

		// Step into whichever neighbouring cell the ray reaches first
		if (t_next_x < t_next_y) {
			cx += step_x;
			t_next_x += t_delta_x;
			if (cx < 0 || cx >= grid_width) {
				break;
			}
		} else {
			cy += step_y;
			t_next_y += t_delta_y;
			if (cy < 0 || cy >= grid_height) {
				break;
			}
		}
	}

	if (best_segment == nullptr) {
		return false;
	}
	r_t = best_t;
	r_segment = best_segment;
	return true;
}

bool PhononGeometry::cast(const Vector2 &p_from, const Vector2 &p_dir, float p_max_distance, RayHit &r_hit) const {
	float best_t = p_max_distance;
	const WallSegment *best_segment = nullptr;

	// Static walls through the grid
	float t;
	const WallSegment *segment = nullptr;
	if (cast_static(p_from, p_dir, best_t, t, segment)) {
		best_t = t;
		best_segment = segment;
	}

	// Moving walls: few of them, so brute force. Passing best_t means only closer hits count.
	for (const WallSegment &dynamic : dynamic_segments) {
		if (intersect(dynamic, p_from, p_dir, best_t, t)) {
			best_t = t;
			best_segment = &dynamic;
		}
	}

	if (best_segment == nullptr) {
		return false;
	}

	r_hit.distance = best_t;
	r_hit.position = p_from + p_dir * best_t;
	// Two-sided walls: make the normal face back towards the incoming ray
	r_hit.normal = dot(best_segment->normal, p_dir) > 0.0f ? -best_segment->normal : best_segment->normal;
	r_hit.absorption = best_segment->absorption;
	return true;
}
