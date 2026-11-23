#pragma once

#include "server/phonon_hit.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string_name.hpp>

using namespace godot;

/**
 * One rule a PhononSensor2D watches, e.g. "lots of high-pitched sound at 880 Hz arrived directly".
 * A Resource so it's edited in the inspector and can be saved/shared between sensors.
 *
 * Holds only settings, never runtime state: one condition resource can be shared by many sensors,
 * so "is it currently triggered" and cooldown timers live in the sensor, one per condition.
 *
 * Two parts:
 *   - Filters (accepts): which hits count at all (pitch, tag, direct only).
 *   - Thresholds: how much of those hits' energy must be in the window to trigger, and when to release.
 */
class PhononSensorCondition : public Resource {
	GDCLASS(PhononSensorCondition, Resource)

public:
	// Which energy to add up. Lives inside the class: PhononSensorCondition.BAND_HIGH in GDScript.
	enum Band {
		BAND_ALL,  // all bands summed
		BAND_LOW,
		BAND_MID,
		BAND_HIGH,
	};

	// Logic used by the sensor (C++ only) -------------------------------------------------------
	// Does this hit pass the filters?
	bool accepts(const PhononHit &p_hit) const;

	// The part of a hit's energy this condition cares about (selected band, or all)
	float get_hit_energy(const PhononHit &p_hit) const;

	// p_energy is already normalised by the sensor's diameter
	bool should_trigger(float p_energy, int p_packets) const;
	bool should_release(float p_energy) const;

	// Config (inspector properties) --------------------------------------------------------------
	void set_id(const StringName &p_id);
	StringName get_id() const;

	void set_band(Band p_band);
	Band get_band() const;

	void set_min_energy(float p_energy);
	float get_min_energy() const;

	void set_min_packets(int p_packets);
	int get_min_packets() const;

	void set_pitch(float p_pitch);
	float get_pitch() const;

	void set_pitch_tolerance(float p_tolerance);
	float get_pitch_tolerance() const;

	void set_source_tag(const StringName &p_tag);
	StringName get_source_tag() const;

	void set_require_direct(bool p_require);
	bool get_require_direct() const;

	void set_release_ratio(float p_ratio);
	float get_release_ratio() const;

	void set_one_shot(bool p_one_shot);
	bool get_one_shot() const;

	void set_cooldown(float p_seconds);
	float get_cooldown() const;

protected:
	static void _bind_methods();

private:
	StringName id;                 // name for scripts to check, e.g. "shatter" ("name" is taken by Resource)
	Band band = BAND_ALL;
	float min_energy = 0.01f;      // trigger threshold (energy / diameter in the window)
	int min_packets = 0;           // 0 = off. Extra check on hit count; depends on packet_count, so use sparingly
	float pitch = 0.0f;            // Hz, 0 = any pitch
	float pitch_tolerance = 30.0f; // Hz either side of pitch
	StringName source_tag;         // empty = any emitter
	bool require_direct = false;   // only count hits that never bounced
	float release_ratio = 0.5f;    // hysteresis: released when energy < min_energy * this (stops flicker)
	bool one_shot = false;         // fire once, then never again
	float cooldown = 0.0f;         // seconds after triggering before it can trigger again
};

VARIANT_ENUM_CAST(PhononSensorCondition::Band);
