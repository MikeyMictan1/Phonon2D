#pragma once

#include <godot_cpp/classes/collision_shape2d.hpp>

#include "resources/phonon_material.h"

using namespace godot;

/**
 * A CollisionShape2D that also carries a PhononMaterial. Use it instead of a normal
 * CollisionShape2D on anything phonons should bounce off with specific acoustics
 * (StaticBody2D, RigidBody2D, CharacterBody2D, Area2D, ...). The body still needs to be on
 * the phonon collision layer.
 *
 * The material lives on the shape (not the body) because the shape is the surface: one body
 * can mix materials, e.g. a glass window shape and a concrete wall shape.
 * No material set, or a plain CollisionShape2D: the server uses its default_absorption.
 */
class PhononCollisionShape2D : public CollisionShape2D {
	GDCLASS(PhononCollisionShape2D, CollisionShape2D)

public:
	// Named phonon_material, not material: CanvasItem already has a "material" property (for shaders)
	void set_phonon_material(const Ref<PhononMaterial> &p_material);
	Ref<PhononMaterial> get_phonon_material() const;

protected:
	static void _bind_methods();

private:
	Ref<PhononMaterial> phonon_material;
};
