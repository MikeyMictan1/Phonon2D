#pragma once

#include <godot_cpp/variant/vector2.hpp>

// One straight edge of a wall, in world space. Everything a ray test needs, precomputed at bake
// time so a tick never has to touch scene nodes or materials.
struct WallSegment {
	godot::Vector2 a;
	godot::Vector2 b;
	godot::Vector2 normal; // unit perpendicular to (b - a). Walls are two-sided: flipped per hit to face the ray
	float absorption = 0.0f;

	WallSegment() = default;
	WallSegment(const godot::Vector2 &p_a, const godot::Vector2 &p_b, float p_absorption) :
			a(p_a), b(p_b), absorption(p_absorption) {
		const godot::Vector2 edge = b - a;
		normal = godot::Vector2(-edge.y, edge.x).normalized(); // zero-length edge gives (0, 0): never hit anyway
	}
};
