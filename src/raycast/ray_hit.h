#pragma once

#include <godot_cpp/variant/vector2.hpp>

// The answer to one ray query: where the ray first touched a wall and what that wall is like.
// Plain data, lives on the stack. Replaces the Dictionary that intersect_ray returns.
struct RayHit {
	godot::Vector2 position; // world-space hit point
	godot::Vector2 normal; // unit vector, always facing back towards where the ray came from
	float distance = 0.0f; // how far along the ray the hit is (ray direction is a unit vector)
	float absorption = 0.0f; // the surface's absorption, baked in so no material lookup is needed
};
