#include "resources/phonon_material.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void PhononMaterial::_bind_methods() {
	BIND_ENUM_CONSTANT(NONE);

	ClassDB::bind_method(D_METHOD("set_absorption", "absorption"), &PhononMaterial::set_absorption);
	ClassDB::bind_method(D_METHOD("get_absorption"), &PhononMaterial::get_absorption);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "absorption", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_absorption", "get_absorption");
}

void PhononMaterial::set_absorption(float p_absorption) {
	absorption = CLAMP(p_absorption, 0.0f, 1.0f);
	emit_changed(); // Resource convention: tell anything using this material that it changed
}

float PhononMaterial::get_absorption() const {
	return absorption;
}
