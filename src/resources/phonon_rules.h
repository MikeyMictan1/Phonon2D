#pragma once

#include "server/phonon_hit.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

/**
 * One rule inside a PhononSensorCondition. In the inspector: Rules -> Add Element -> pick a rule type.
 * Only the rules you add are checked, so a condition shows exactly what it tests.
 *
 * Two kinds (Strategy pattern: the condition just loops over rules through this one interface):
 *   - Filters decide WHICH hits count (override accepts). Every filter must pass.
 *   - Thresholds decide WHEN the condition fires (override is_met / is_released). Every threshold must be met;
 *     the condition releases when any threshold falls below its value * release_ratio.
 *
 * Settings only, never runtime state (rules are shared like any Resource).
 * p_energy is the window's accepted energy divided by the sensor's diameter; p_packets is the accepted hit count.
 */
class PhononRule : public Resource {
	GDCLASS(PhononRule, Resource)

public:
	virtual bool accepts(const PhononHit &p_hit) const { return true; }
	virtual bool is_met(float p_energy, int p_packets) const { return true; }
	virtual bool is_released(float p_energy, int p_packets, float p_release_ratio) const { return false; }

protected:
	static void _bind_methods() {}
};

// ---- Thresholds ------------------------------------------------------------------------------------

// Fires once at least `count` accepted packets are in the window
class PhononMinPackets : public PhononRule {
	GDCLASS(PhononMinPackets, PhononRule)

public:
	bool is_met(float p_energy, int p_packets) const override;
	bool is_released(float p_energy, int p_packets, float p_release_ratio) const override;

	void set_count(int p_count);
	int get_count() const;

protected:
	static void _bind_methods();

private:
	int count = 10;
};

// Fires once the window's accepted energy (per unit of diameter) reaches `energy`
class PhononMinEnergy : public PhononRule {
	GDCLASS(PhononMinEnergy, PhononRule)

public:
	bool is_met(float p_energy, int p_packets) const override;
	bool is_released(float p_energy, int p_packets, float p_release_ratio) const override;

	void set_energy(float p_energy);
	float get_energy() const;

protected:
	static void _bind_methods();

private:
	float energy = 0.001f;
};

// ---- Filters ---------------------------------------------------------------------------------------

// Only counts hits whose emitter frequency is within `tolerance` Hz of `frequency`
class PhononFrequency : public PhononRule {
	GDCLASS(PhononFrequency, PhononRule)

public:
	bool accepts(const PhononHit &p_hit) const override;

	void set_frequency(float p_frequency);
	float get_frequency() const;

	void set_tolerance(float p_tolerance);
	float get_tolerance() const;

protected:
	static void _bind_methods();

private:
	float frequency = 440.0f;
	float tolerance = 30.0f;
};

// Only counts hits from emitters with this tag (e.g. "footstep")
class PhononSourceTag : public PhononRule {
	GDCLASS(PhononSourceTag, PhononRule)

public:
	bool accepts(const PhononHit &p_hit) const override;

	void set_tag(const StringName &p_tag);
	StringName get_tag() const;

protected:
	static void _bind_methods();

private:
	StringName tag;
};

// Only counts hits that arrived without bouncing (line of sight to the emitter)
class PhononDirectOnly : public PhononRule {
	GDCLASS(PhononDirectOnly, PhononRule)

public:
	bool accepts(const PhononHit &p_hit) const override;

protected:
	static void _bind_methods() {}
};
