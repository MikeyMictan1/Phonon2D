#include "nodes/phonon_collision_shape_2d.h"

#include <godot_cpp/core/class_db.hpp>

void PhononCollisionShape2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_phonon_material", "material"), &PhononCollisionShape2D::set_phonon_material);
	ClassDB::bind_method(D_METHOD("get_phonon_material"), &PhononCollisionShape2D::get_phonon_material);
	// RESOURCE_TYPE hint: the inspector slot only offers PhononMaterial (New / Load / drag-drop)
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "phonon_material", PROPERTY_HINT_RESOURCE_TYPE, "PhononMaterial"),
			"set_phonon_material", "get_phonon_material");

	ADD_SIGNAL(MethodInfo(MATERIAL_CHANGED_SIGNAL));
}

void PhononCollisionShape2D::set_phonon_material(const Ref<PhononMaterial> &p_material) {
	if (phonon_material == p_material) {
		return;
	}
	phonon_material = p_material;
	emit_signal(MATERIAL_CHANGED_SIGNAL);
}

Ref<PhononMaterial> PhononCollisionShape2D::get_phonon_material() const {
	return phonon_material;
}
