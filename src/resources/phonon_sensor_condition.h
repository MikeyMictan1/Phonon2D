#pragma once

#include "resources/phonon_rules.h"
#include "server/phonon_hit.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <vector>

using namespace godot;

/**
 * One thing a PhononSensor2D watches for, e.g. "10+ packets of 880 Hz sound" -> shatter the glass.
 * Made of rules you add in the inspector (Rules -> Add Element -> pick a type), see phonon_rules.h.
 * Rules combine with AND. No threshold rules = fires on the first accepted hit.
 *
 * Holds only settings, never runtime state: one condition resource can be shared by many sensors,
 * so "is it currently triggered" and cooldown timers live in the sensor, one per condition.
 */
class PhononSensorCondition : public Resource {
	GDCLASS(PhononSensorCondition, Resource)

public:
	// Logic used by the sensor (C++ only) -------------------------------------------------------
	// Does this hit pass every filter rule?
	bool accepts(const PhononHit &p_hit) const;

	// p_energy is already normalised by the sensor's diameter
	bool should_trigger(float p_energy, int p_packets) const;
	bool should_release(float p_energy, int p_packets) const;

	// Config (inspector properties) --------------------------------------------------------------
	void set_id(const StringName &p_id);
	StringName get_id() const;

	void set_rules(const TypedArray<PhononRule> &p_rules);
	TypedArray<PhononRule> get_rules() const;

	void set_release_ratio(float p_ratio);
	float get_release_ratio() const;

	void set_one_shot(bool p_one_shot);
	bool get_one_shot() const;

	void set_cooldown(float p_seconds);
	float get_cooldown() const;

protected:
	static void _bind_methods();

private:
	StringName id;               // name for scripts to check, e.g. "shatter" ("name" is taken by Resource)
	TypedArray<PhononRule> rules;
	float release_ratio = 0.5f;  // hysteresis: a threshold releases below its value * this (stops flicker)
	bool one_shot = false;       // fire once, then never again (until the sensor's reset_conditions())
	float cooldown = 0.0f;       // seconds after triggering before it can trigger again

	// Plain C++ copy of `rules`: the sensor checks every hit against every rule each tick, and reading
	// a TypedArray element goes through Variant. Rebuilt in set_rules, and lazily if the sizes differ
	// (GDScript can append to the array without calling the setter).
	mutable std::vector<Ref<PhononRule>> rule_cache;
	const std::vector<Ref<PhononRule>> &get_rule_cache() const;
	void rebuild_rule_cache() const;
};
