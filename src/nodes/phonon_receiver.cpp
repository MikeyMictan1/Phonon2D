#include "nodes/phonon_receiver.h"
#include "server/phonon_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void PhononReceiver::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_diameter"), &PhononReceiver::get_diameter);
	ClassDB::bind_method(D_METHOD("get_window_energy"), &PhononReceiver::get_window_energy);
	ClassDB::bind_method(D_METHOD("get_hit_count"), &PhononReceiver::get_hit_count);

	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &PhononReceiver::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &PhononReceiver::get_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "1,1000,0.5,or_greater,suffix:px"), "set_radius", "get_radius");

	ClassDB::bind_method(D_METHOD("set_window_length", "seconds"), &PhononReceiver::set_window_length);
	ClassDB::bind_method(D_METHOD("get_window_length"), &PhononReceiver::get_window_length);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "window_length", PROPERTY_HINT_RANGE, "0.01,10,0.01,or_greater,suffix:s"), "set_window_length", "get_window_length");

	ClassDB::bind_method(D_METHOD("set_buffer_capacity", "capacity"), &PhononReceiver::set_buffer_capacity);
	ClassDB::bind_method(D_METHOD("get_buffer_capacity"), &PhononReceiver::get_buffer_capacity);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "buffer_capacity", PROPERTY_HINT_RANGE, "16,65536,1"), "set_buffer_capacity", "get_buffer_capacity");

	ADD_GROUP("Debug", "debug_");
	ClassDB::bind_method(D_METHOD("set_debug_draw", "enabled"), &PhononReceiver::set_debug_draw);
	ClassDB::bind_method(D_METHOD("get_debug_draw"), &PhononReceiver::get_debug_draw);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug_draw"), "set_debug_draw", "get_debug_draw");

	ClassDB::bind_method(D_METHOD("set_debug_color", "color"), &PhononReceiver::set_debug_color);
	ClassDB::bind_method(D_METHOD("get_debug_color"), &PhononReceiver::get_debug_color);
	ADD_PROPERTY(PropertyInfo(Variant::COLOR, "debug_color"), "set_debug_color", "get_debug_color");
	ADD_GROUP("", "");
}

// ---- Lifecycle ----

// Register with the server so tick() tests packets against this circle. Not in the editor: nothing is simulated there.
void PhononReceiver::_enter_tree() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	PhononServer *server = PhononServer::get_singleton();
	if (server != nullptr) {
		server->register_receiver(this);
	}
}

// Must unregister before the node is freed, or the server would hold a dangling pointer
void PhononReceiver::_exit_tree() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	PhononServer *server = PhononServer::get_singleton();
	if (server != nullptr) {
		server->unregister_receiver(this);
	}
	hits.clear(); // stale hits would carry over if the node is re-added later
}

void PhononReceiver::_draw() {
	if (debug_draw) {
		draw_arc(Vector2(0, 0), radius, 0.0f, Math_TAU, 48, debug_color, 1.0f);
	}
}

// ---- Called by the server ----

void PhononReceiver::add_hit(const PhononHit &p_hit) {
	hits.push(p_hit);
}

void PhononReceiver::end_tick(float p_now) {
	hits.drop_older_than(p_now - window_length);
}

// ---- Queries ----

float PhononReceiver::get_diameter() const {
	return radius * 2.0f;
}

float PhononReceiver::get_window_energy() const {
	float total = 0.0f;
	hits.for_each([&](const PhononHit &hit) { total += hit.total_energy(); });
	return total / get_diameter();
}

int PhononReceiver::get_hit_count() const {
	return static_cast<int>(hits.size());
}

// ---- Config ----

void PhononReceiver::set_radius(float p_radius) {
	radius = MAX(p_radius, 1.0f); // diameter is a divisor, so never 0
	queue_redraw();
}
float PhononReceiver::get_radius() const { return radius; }

void PhononReceiver::set_window_length(float p_seconds) { window_length = MAX(p_seconds, 0.01f); }
float PhononReceiver::get_window_length() const { return window_length; }

void PhononReceiver::set_buffer_capacity(int p_capacity) { hits.set_capacity(static_cast<std::size_t>(CLAMP(p_capacity, 16, 65536))); }
int PhononReceiver::get_buffer_capacity() const { return static_cast<int>(hits.capacity()); }

void PhononReceiver::set_debug_draw(bool p_enabled) {
	debug_draw = p_enabled;
	queue_redraw();
}
bool PhononReceiver::get_debug_draw() const { return debug_draw; }

void PhononReceiver::set_debug_color(const Color &p_color) {
	debug_color = p_color;
	queue_redraw();
}
Color PhononReceiver::get_debug_color() const { return debug_color; }
