#include "nodes/phonon_world_2d.h"
#include "server/phonon_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/world2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void PhononWorld2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_debug_draw", "enabled"), &PhononWorld2D::set_debug_draw);
	ClassDB::bind_method(D_METHOD("get_debug_draw"), &PhononWorld2D::get_debug_draw);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_draw"), "set_debug_draw", "get_debug_draw");

	ClassDB::bind_method(D_METHOD("set_packet_color", "color"), &PhononWorld2D::set_packet_color);
	ClassDB::bind_method(D_METHOD("get_packet_color"), &PhononWorld2D::get_packet_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "packet_color"), "set_packet_color", "get_packet_color");

	ClassDB::bind_method(D_METHOD("set_dot_radius", "radius"), &PhononWorld2D::set_dot_radius);
	ClassDB::bind_method(D_METHOD("get_dot_radius"), &PhononWorld2D::get_dot_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dot_radius", PROPERTY_HINT_RANGE, "0.5,10,0.5"), "set_dot_radius", "get_dot_radius");

	ClassDB::bind_method(D_METHOD("set_trail_length", "length"), &PhononWorld2D::set_trail_length);
	ClassDB::bind_method(D_METHOD("get_trail_length"), &PhononWorld2D::get_trail_length);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "trail_length", PROPERTY_HINT_RANGE, "0,100,1"), "set_trail_length", "get_trail_length");

	ClassDB::bind_method(D_METHOD("set_min_alpha", "alpha"), &PhononWorld2D::set_min_alpha);
	ClassDB::bind_method(D_METHOD("get_min_alpha"), &PhononWorld2D::get_min_alpha);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_alpha", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_min_alpha", "get_min_alpha");
}

// ---- Lifecycle ----

void PhononWorld2D::_enter_tree() {
	add_to_group(GROUP_NAME);
}

// Leaving the tree (scene change, node deleted): this scene's packets shouldn't carry over into the next one
void PhononWorld2D::_exit_tree() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	PhononServer *server = PhononServer::get_singleton();
	if (server != nullptr) {
		server->clear_packets();
	}
}

// Once per physics frame: run the simulation against this scene's walls, then ask to be redrawn
void PhononWorld2D::_physics_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return; // GDExtension nodes also run in the editor; don't simulate while editing
	}

	PhononServer *server = PhononServer::get_singleton();
	if (server == nullptr) {
		return;
	}

	// The physics space only exists for nodes inside a viewport, which is why this node (not the server) fetches it
	PhysicsDirectSpaceState2D *space = get_world_2d()->get_direct_space_state();
	server->tick(p_delta, space);

	if (debug_draw) {
		queue_redraw();
	}
}

void PhononWorld2D::_draw() {
	if (!debug_draw) {
		return;
	}
	const PhononServer *server = PhononServer::get_singleton();
	if (server == nullptr) {
		return;
	}

	// Packet positions are in global (world) coordinates. Undo this node's own transform once,
	// so the dots land in the right place even if the node has been moved, rotated or scaled.
	draw_set_transform_matrix(get_global_transform().affine_inverse());

	const int count = server->get_active_packet_count();
	if (count == 0) {
		return;
	}

	// Batched drawing: fill arrays in plain C++, then hand them to the engine in two calls,
	// instead of one draw_line + draw_circle per packet (thousands of separate draw commands).
	// Two points (one segment) per packet, one colour per segment.
	PackedVector2Array trail_points;
	PackedVector2Array dot_points;
	PackedColorArray colors;
	trail_points.resize(count * 2);
	dot_points.resize(count * 2);
	colors.resize(count);

	// Write through raw pointers: skips the per-element checks of set()
	Vector2 *trail = trail_points.ptrw();
	Vector2 *dot = dot_points.ptrw();
	Color *col = colors.ptrw();

	int i = 0;
	server->for_each_active_packet([&](const PhononPacket &packet) {
		if (i >= count) {
			return; // safety: never write past the arrays
		}
		Color color = packet_color;
		color.a = CLAMP(packet.energy, min_alpha, 1.0f); // fade as the packet loses energy; quiet sounds look faint
		col[i] = color;

		// Trail: segment from behind the packet up to it
		trail[i * 2] = packet.position - packet.direction * trail_length;
		trail[i * 2 + 1] = packet.position;

		// Dot: a short segment as long as it is wide, i.e. a small square facing the direction of travel
		dot[i * 2] = packet.position - packet.direction * dot_radius;
		dot[i * 2 + 1] = packet.position + packet.direction * dot_radius;

		i++;
	});

	if (trail_length > 0.0f) {
		draw_multiline_colors(trail_points, colors, 2.0f);
	}
	draw_multiline_colors(dot_points, colors, dot_radius * 2.0f);
}

// Yellow warning triangle in the editor if a scene has more than one PhononWorld2D
// (each one ticks the server, so two would run the simulation at double speed)
PackedStringArray PhononWorld2D::_get_configuration_warnings() const {
	PackedStringArray warnings;
	if (is_inside_tree() && get_tree()->get_nodes_in_group(GROUP_NAME).size() > 1) {
		warnings.push_back("Only one PhononWorld2D should exist per scene. Each one ticks the PhononServer, so extras speed up the simulation.");
	}
	return warnings;
}

// ---- Drawing settings ----

void PhononWorld2D::set_debug_draw(bool p_enabled) {
	debug_draw = p_enabled;
	queue_redraw(); // clears the old drawing immediately when turned off
}
bool PhononWorld2D::get_debug_draw() const { return debug_draw; }

void PhononWorld2D::set_packet_color(const Color &p_color) { packet_color = p_color; }
Color PhononWorld2D::get_packet_color() const { return packet_color; }

void PhononWorld2D::set_dot_radius(float p_radius) { dot_radius = p_radius; }
float PhononWorld2D::get_dot_radius() const { return dot_radius; }

void PhononWorld2D::set_trail_length(float p_length) { trail_length = p_length; }
float PhononWorld2D::get_trail_length() const { return trail_length; }

void PhononWorld2D::set_min_alpha(float p_alpha) { min_alpha = CLAMP(p_alpha, 0.0f, 1.0f); }
float PhononWorld2D::get_min_alpha() const { return min_alpha; }
