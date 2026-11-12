#pragma once

#include <godot_cpp/classes/resource.hpp>

using namespace godot;

// Acoustic properties of a surface (absorption, scattering), applied to
// collision bodies, sprites, tilemaps. Presets come from PhononMaterialType.
class PhononMaterial : public Resource {
	GDCLASS(PhononMaterial, Resource)

public:
	// Lives inside the class so Godot shows it as PhononMaterial.MaterialType.
	// Values added during implementation (NONE, FOAM, VELVET, ...).
	enum MaterialType {
		NONE,
	};

	// Fraction of a packet's intensity lost per bounce: 0 = perfect mirror, 1 = absorbs everything
	void set_absorption(float p_absorption);
	float get_absorption() const;

protected:
	static void _bind_methods();

private:
	float absorption = 0.1f;
};

VARIANT_ENUM_CAST(PhononMaterial::MaterialType);
