#pragma once

#include "raycast/ray_hit.h"
#include "raycast/wall_segment.h"

#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

/**
 * The world as the simulation sees it: a list of wall segments plus a uniform grid to find the
 * nearby ones quickly. Pure maths and plain arrays, no scene nodes and no engine calls, so cast()
 * is cheap and (being const) safe to call from several threads later.
 *
 * Two kinds of segments:
 *  - static:  baked once, sorted into the grid, never touched again
 *  - dynamic: belong to moving bodies (doors, platforms), updated each tick by GeometryBaker::sync_dynamic,
 *             kept in a small separate list that cast() tests by brute force
 */
class PhononGeometry {
public:
	static constexpr float DEFAULT_CELL_SIZE = 64.0f;

	// A moving body's segments, stored in the body's local space so they can be re-placed
	// whenever the body moves without asking Godot for its shapes again.
	struct DynamicBody {
		uint64_t object_id = 0; // Godot ObjectID of the body, checked each sync in case it was freed
		godot::Transform2D last_transform; // global transform the world-space segments were built from
		std::vector<WallSegment> local_segments; // in body space
		uint32_t first = 0; // index of this body's first segment in dynamic_segments
	};

	// ---- Building (GeometryBaker calls these) ----
	void clear();
	void add_static_segment(const godot::Vector2 &p_a, const godot::Vector2 &p_b, float p_absorption);
	void add_dynamic_body(uint64_t p_object_id, const godot::Transform2D &p_global, std::vector<WallSegment> &&p_local_segments);
	// Sorts static segments into grid cells. Call once after all add_static_segment calls.
	void build(float p_cell_size = DEFAULT_CELL_SIZE);

	// Re-places one dynamic body's segments at a new global transform (pure maths)
	void update_dynamic_body(std::size_t p_index, const godot::Transform2D &p_global);

	// ---- Querying (the hot path) ----
	// Casts from p_from along p_dir (must be a unit vector) up to p_max_distance.
	// Returns true and fills r_hit with the nearest wall hit, or returns false if the path is clear.
	bool cast(const godot::Vector2 &p_from, const godot::Vector2 &p_dir, float p_max_distance, RayHit &r_hit) const;

	// ---- Inspection (debug drawing, stats) ----
	const std::vector<WallSegment> &get_static_segments() const { return static_segments; }
	const std::vector<WallSegment> &get_dynamic_segments() const { return dynamic_segments; }
	std::vector<DynamicBody> &get_dynamic_bodies() { return dynamic_bodies; }
	std::size_t get_static_count() const { return static_segments.size(); }
	std::size_t get_dynamic_count() const { return dynamic_segments.size(); }

private:
	std::vector<WallSegment> static_segments;
	std::vector<WallSegment> dynamic_segments; // world space, rewritten when a dynamic body moves
	std::vector<DynamicBody> dynamic_bodies;

	// Uniform grid over the static segments' bounding box. Stored as two flat arrays (CSR layout)
	// instead of a vector per cell: cell i's segment indices are
	// cell_items[cell_start[i] .. cell_start[i + 1]]. Two allocations total, cache-friendly.
	float cell_size = DEFAULT_CELL_SIZE;
	godot::Vector2 grid_origin; // world position of cell (0, 0)'s corner
	int grid_width = 0; // cells along x
	int grid_height = 0; // cells along y
	std::vector<uint32_t> cell_start; // grid_width * grid_height + 1 entries
	std::vector<uint32_t> cell_items; // static segment indices, grouped by cell

	// Ray vs one segment. If they cross within (0, p_max_distance], writes the distance to r_t.
	static bool intersect(const WallSegment &p_segment, const godot::Vector2 &p_from, const godot::Vector2 &p_dir, float p_max_distance, float &r_t);

	// Nearest static hit, walking the grid cells the ray passes through
	bool cast_static(const godot::Vector2 &p_from, const godot::Vector2 &p_dir, float p_max_distance, float &r_t, const WallSegment *&r_segment) const;
};
