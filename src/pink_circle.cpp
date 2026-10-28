#include "pink_circle.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void PinkCircle::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &PinkCircle::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &PinkCircle::get_radius);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius"), "set_radius", "get_radius");
}

void PinkCircle::set_radius(float p_radius) {
	radius = p_radius;
	queue_redraw();
}

float PinkCircle::get_radius() const {
	return radius;
}

void PinkCircle::_process(double delta) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	time += delta;
	queue_redraw();
}

void PinkCircle::_draw() {
	float r = radius * (1.0f + 0.2f * (float)Math::sin(time * 3.0));
	draw_circle(Vector2(), r, Color(1.0f, 0.4f, 0.7f));
}
