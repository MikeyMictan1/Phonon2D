#include "raycast/geometry_baker.h"
#include "nodes/phonon_collision_shape_2d.h"
#include "resources/phonon_material.h"

#include <godot_cpp/classes/animatable_body2d.hpp>
#include <godot_cpp/classes/area2d.hpp>
#include <godot_cpp/classes/capsule_shape2d.hpp>
#include <godot_cpp/classes/circle_shape2d.hpp>
#include <godot_cpp/classes/collision_object2d.hpp>
#include <godot_cpp/classes/concave_polygon_shape2d.hpp>
#include <godot_cpp/classes/convex_polygon_shape2d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/rectangle_shape2d.hpp>
#include <godot_cpp/classes/segment_shape2d.hpp>
#include <godot_cpp/classes/static_body2d.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cmath>

using namespace godot;

namespace {
// Circles and capsule ends become polygons with this many sides per full turn. Fine for sound.
constexpr int CIRCLE_SIDES = 16;

// Appends points along an arc (angles in radians, inclusive of both ends)
void append_arc(std::vector<Vector2> &r_points, const Vector2 &p_center, float p_radius, float p_from_angle, float p_to_angle, int p_steps) {
	for (int i = 0; i <= p_steps; i++) {
		const float angle = p_from_angle + (p_to_angle - p_from_angle) * static_cast<float>(i) / p_steps;
		r_points.push_back(p_center + Vector2(std::cos(angle), std::sin(angle)) * p_radius);
	}
}
} // namespace

// ---- Bake ----

void GeometryBaker::bake(Node *p_root, const Settings &p_settings, PhononGeometry &r_geometry) {
	r_geometry.clear();
	if (p_root != nullptr) {
		visit(p_root, p_settings, r_geometry);
	}
	r_geometry.build(p_settings.cell_size);
}

// Depth-first walk of the whole tree under p_node
void GeometryBaker::visit(Node *p_node, const Settings &p_settings, PhononGeometry &r_geometry) {
	if (CollisionObject2D *body = Object::cast_to<CollisionObject2D>(p_node)) {
		add_body(body, p_settings, r_geometry);
	}
	const int child_count = p_node->get_child_count();
	for (int i = 0; i < child_count; i++) {
		visit(p_node->get_child(i), p_settings, r_geometry);
	}
}

void GeometryBaker::add_body(CollisionObject2D *p_body, const Settings &p_settings, PhononGeometry &r_geometry) {
	// Same filters as the Godot raycast we replace: bodies only (no areas), on a layer the mask includes
	if (Object::cast_to<Area2D>(p_body) != nullptr) {
		return;
	}
	if ((p_body->get_collision_layer() & p_settings.collision_mask) == 0) {
		return;
	}

	// Collect every enabled shape as segments in the body's local space
	std::vector<WallSegment> local_segments;
	const PackedInt32Array owners = p_body->get_shape_owners();
	for (int64_t i = 0; i < owners.size(); i++) {
		const uint32_t owner_id = static_cast<uint32_t>(owners[i]);
		if (p_body->is_shape_owner_disabled(owner_id)) {
			continue;
		}
		const Transform2D owner_xform = p_body->shape_owner_get_transform(owner_id); // shape's place within the body
		const float absorption = get_absorption(p_body->shape_owner_get_owner(owner_id), p_settings);

		const int shape_count = p_body->shape_owner_get_shape_count(owner_id);
		for (int s = 0; s < shape_count; s++) {
			add_shape(p_body->shape_owner_get_shape(owner_id, s), owner_xform, absorption, local_segments);
		}
	}
	if (local_segments.empty()) {
		return;
	}

	const Transform2D global = p_body->get_global_transform();

	// StaticBody2D is assumed not to move. AnimatableBody2D derives from StaticBody2D but exists to be
	// moved (doors, platforms), so it counts as dynamic, like CharacterBody2D and RigidBody2D.
	const bool is_static = Object::cast_to<StaticBody2D>(p_body) != nullptr && Object::cast_to<AnimatableBody2D>(p_body) == nullptr;
	if (is_static) {
		for (const WallSegment &local : local_segments) {
			r_geometry.add_static_segment(global.xform(local.a), global.xform(local.b), local.absorption);
		}
	} else {
		r_geometry.add_dynamic_body(p_body->get_instance_id(), global, std::move(local_segments));
	}
}

// ---- Shapes -> segments ----

void GeometryBaker::add_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform, float p_absorption, std::vector<WallSegment> &r_out) {
	if (p_shape.is_null()) {
		return;
	}

	if (const RectangleShape2D *rect = Object::cast_to<RectangleShape2D>(p_shape.ptr())) {
		const Vector2 h = rect->get_size() * 0.5f; // rectangles are centred on their origin
		const Vector2 corners[4] = { Vector2(-h.x, -h.y), Vector2(h.x, -h.y), Vector2(h.x, h.y), Vector2(-h.x, h.y) };
		add_polygon(corners, 4, true, p_xform, p_absorption, r_out);

	} else if (const CircleShape2D *circle = Object::cast_to<CircleShape2D>(p_shape.ptr())) {
		std::vector<Vector2> points;
		append_arc(points, Vector2(), circle->get_radius(), 0.0f, static_cast<float>(Math_TAU), CIRCLE_SIDES);
		points.pop_back(); // last point equals the first; the closed polygon joins them
		add_polygon(points.data(), static_cast<int>(points.size()), true, p_xform, p_absorption, r_out);

	} else if (const CapsuleShape2D *capsule = Object::cast_to<CapsuleShape2D>(p_shape.ptr())) {
		// Vertical capsule: half-circle on top, half-circle on the bottom, straight sides between
		const float radius = capsule->get_radius();
		const float half_mid = std::max(0.0f, capsule->get_height() * 0.5f - radius);
		std::vector<Vector2> points;
		append_arc(points, Vector2(0, -half_mid), radius, static_cast<float>(Math_PI), static_cast<float>(Math_TAU), CIRCLE_SIDES / 2); // top
		append_arc(points, Vector2(0, half_mid), radius, 0.0f, static_cast<float>(Math_PI), CIRCLE_SIDES / 2); // bottom
		add_polygon(points.data(), static_cast<int>(points.size()), true, p_xform, p_absorption, r_out);

	} else if (const SegmentShape2D *segment = Object::cast_to<SegmentShape2D>(p_shape.ptr())) {
		const Vector2 ends[2] = { segment->get_a(), segment->get_b() };
		add_polygon(ends, 2, false, p_xform, p_absorption, r_out);

	} else if (const ConvexPolygonShape2D *convex = Object::cast_to<ConvexPolygonShape2D>(p_shape.ptr())) {
		// Also what a solid CollisionPolygon2D turns into (one or more convex pieces)
		const PackedVector2Array points = convex->get_points();
		add_polygon(points.ptr(), static_cast<int>(points.size()), true, p_xform, p_absorption, r_out);

	} else if (const ConcavePolygonShape2D *concave = Object::cast_to<ConcavePolygonShape2D>(p_shape.ptr())) {
		// Already a list of separate segments: pairs of points
		const PackedVector2Array points = concave->get_segments();
		for (int64_t i = 0; i + 1 < points.size(); i += 2) {
			const Vector2 ends[2] = { points[i], points[i + 1] };
			add_polygon(ends, 2, false, p_xform, p_absorption, r_out);
		}

	} else {
		UtilityFunctions::push_warning("Phonon2D: shape type ", p_shape->get_class(), " is not supported by the phonon raycaster and will not block sound.");
	}
}

// Edges between consecutive points (and last -> first if closed), transformed into body space
void GeometryBaker::add_polygon(const Vector2 *p_points, int p_count, bool p_closed, const Transform2D &p_xform, float p_absorption, std::vector<WallSegment> &r_out) {
	if (p_count < 2) {
		return;
	}
	const int edge_count = p_closed ? p_count : p_count - 1;
	for (int i = 0; i < edge_count; i++) {
		const Vector2 a = p_xform.xform(p_points[i]);
		const Vector2 b = p_xform.xform(p_points[(i + 1) % p_count]);
		if (!a.is_equal_approx(b)) {
			r_out.emplace_back(a, b, p_absorption);
		}
	}
}

// ---- Materials ----

float GeometryBaker::get_absorption(Object *p_owner, const Settings &p_settings) {
	PhononCollisionShape2D *shape = Object::cast_to<PhononCollisionShape2D>(p_owner);
	if (shape == nullptr) {
		return p_settings.default_absorption; // plain CollisionShape2D / CollisionPolygon2D
	}

	// Rebake if this shape gets a different material, or its material's values are edited
	const bool watch = p_settings.on_changed.is_valid();
	if (watch && !shape->is_connected(PhononCollisionShape2D::MATERIAL_CHANGED_SIGNAL, p_settings.on_changed)) {
		shape->connect(PhononCollisionShape2D::MATERIAL_CHANGED_SIGNAL, p_settings.on_changed);
	}

	const Ref<PhononMaterial> material = shape->get_phonon_material();
	if (material.is_null()) {
		return p_settings.default_absorption;
	}
	if (watch && !material->is_connected("changed", p_settings.on_changed)) {
		material->connect("changed", p_settings.on_changed);
	}
	return material->get_absorption();
}

// ---- Per tick ----

int GeometryBaker::sync_dynamic(PhononGeometry &r_geometry) {
	int moved = 0;
	std::vector<PhononGeometry::DynamicBody> &bodies = r_geometry.get_dynamic_bodies();
	for (std::size_t i = 0; i < bodies.size(); i++) {
		// The body may have been freed since the bake; it is dropped at the next rebake (node_removed marks dirty)
		Node2D *body = Object::cast_to<Node2D>(ObjectDB::get_instance(bodies[i].object_id));
		if (body == nullptr) {
			continue;
		}
		const Transform2D global = body->get_global_transform(); // the one engine call per moving body
		if (global != bodies[i].last_transform) {
			r_geometry.update_dynamic_body(i, global);
			moved++;
		}
	}
	return moved;
}
