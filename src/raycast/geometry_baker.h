#pragma once

#include "raycast/phonon_geometry.h"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/shape2d.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <vector>

namespace godot {
class Node;
class Object;
class CollisionObject2D;
} // namespace godot

/**
 * The only part of the raycast system that talks to Godot. Turns the scene's collision shapes into
 * WallSegments for PhononGeometry. All the engine calls (tree walk, shapes, transforms, materials)
 * happen here, once per bake, instead of inside every ray query.
 *
 * Static functions only, no state: everything it produces lives in the PhononGeometry it fills.
 */
class GeometryBaker {
public:
	struct Settings {
		uint32_t collision_mask = 0; // bodies on none of these layers are ignored (same rule as Godot's raycast)
		float default_absorption = 0.1f; // shapes without a PhononMaterial
		float cell_size = PhononGeometry::DEFAULT_CELL_SIZE;
		godot::Callable on_changed; // connected to material/shape signals so edits trigger a rebake. May be empty.
	};

	// Clears r_geometry and fills it from every collision body under p_root.
	// StaticBody2D -> static segments (in the grid). AnimatableBody2D, CharacterBody2D, RigidBody2D -> dynamic.
	static void bake(godot::Node *p_root, const Settings &p_settings, PhononGeometry &r_geometry);

	// Once per tick: re-reads each dynamic body's global transform (one engine call per moving body,
	// not per packet) and re-places its segments if it moved. Returns how many bodies moved.
	static int sync_dynamic(PhononGeometry &r_geometry);

private:
	static void visit(godot::Node *p_node, const Settings &p_settings, PhononGeometry &r_geometry);
	static void add_body(godot::CollisionObject2D *p_body, const Settings &p_settings, PhononGeometry &r_geometry);

	// Converts one shape into segments in body space (p_xform = the shape owner's transform within the body)
	static void add_shape(const godot::Ref<godot::Shape2D> &p_shape, const godot::Transform2D &p_xform, float p_absorption, std::vector<WallSegment> &r_out);
	static void add_polygon(const godot::Vector2 *p_points, int p_count, bool p_closed, const godot::Transform2D &p_xform, float p_absorption, std::vector<WallSegment> &r_out);

	// The shape owner's PhononMaterial absorption, or the default. Also hooks up change signals.
	static float get_absorption(godot::Object *p_owner, const Settings &p_settings);
};
