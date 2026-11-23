#include "resources/phonon_sensor_condition.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <cmath>

void PhononSensorCondition::_bind_methods() {
	BIND_ENUM_CONSTANT(BAND_ALL);
	BIND_ENUM_CONSTANT(BAND_LOW);
	BIND_ENUM_CONSTANT(BAND_MID);
	BIND_ENUM_CONSTANT(BAND_HIGH);

	ClassDB::bind_method(D_METHOD("set_id", "id"), &PhononSensorCondition::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &PhononSensorCondition::get_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "id"), "set_id", "get_id");

	// Thresholds
	ADD_GROUP("Threshold", "");
	ClassDB::bind_method(D_METHOD("set_band", "band"), &PhononSensorCondition::set_band);
	ClassDB::bind_method(D_METHOD("get_band"), &PhononSensorCondition::get_band);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "band", PROPERTY_HINT_ENUM, "All,Low,Mid,High"), "set_band", "get_band");

	ClassDB::bind_method(D_METHOD("set_min_energy", "energy"), &PhononSensorCondition::set_min_energy);
	ClassDB::bind_method(D_METHOD("get_min_energy"), &PhononSensorCondition::get_min_energy);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_energy", PROPERTY_HINT_RANGE, "0,1,0.0001,or_greater"), "set_min_energy", "get_min_energy");

	ClassDB::bind_method(D_METHOD("set_min_packets", "packets"), &PhononSensorCondition::set_min_packets);
	ClassDB::bind_method(D_METHOD("get_min_packets"), &PhononSensorCondition::get_min_packets);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_packets", PROPERTY_HINT_RANGE, "0,1000,1,or_greater"), "set_min_packets", "get_min_packets");

	ClassDB::bind_method(D_METHOD("set_release_ratio", "ratio"), &PhononSensorCondition::set_release_ratio);
	ClassDB::bind_method(D_METHOD("get_release_ratio"), &PhononSensorCondition::get_release_ratio);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "release_ratio", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_release_ratio", "get_release_ratio");

	// Filters
	ADD_GROUP("Filter", "");
	ClassDB::bind_method(D_METHOD("set_pitch", "pitch"), &PhononSensorCondition::set_pitch);
	ClassDB::bind_method(D_METHOD("get_pitch"), &PhononSensorCondition::get_pitch);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "0,20000,1,suffix:Hz"), "set_pitch", "get_pitch");

	ClassDB::bind_method(D_METHOD("set_pitch_tolerance", "tolerance"), &PhononSensorCondition::set_pitch_tolerance);
	ClassDB::bind_method(D_METHOD("get_pitch_tolerance"), &PhononSensorCondition::get_pitch_tolerance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_tolerance", PROPERTY_HINT_RANGE, "0,5000,1,suffix:Hz"), "set_pitch_tolerance", "get_pitch_tolerance");

	ClassDB::bind_method(D_METHOD("set_source_tag", "tag"), &PhononSensorCondition::set_source_tag);
	ClassDB::bind_method(D_METHOD("get_source_tag"), &PhononSensorCondition::get_source_tag);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "source_tag"), "set_source_tag", "get_source_tag");

	ClassDB::bind_method(D_METHOD("set_require_direct", "require"), &PhononSensorCondition::set_require_direct);
	ClassDB::bind_method(D_METHOD("get_require_direct"), &PhononSensorCondition::get_require_direct);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "require_direct"), "set_require_direct", "get_require_direct");

	// Timing
	ADD_GROUP("Timing", "");
	ClassDB::bind_method(D_METHOD("set_one_shot", "one_shot"), &PhononSensorCondition::set_one_shot);
	ClassDB::bind_method(D_METHOD("get_one_shot"), &PhononSensorCondition::get_one_shot);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "one_shot"), "set_one_shot", "get_one_shot");

	ClassDB::bind_method(D_METHOD("set_cooldown", "seconds"), &PhononSensorCondition::set_cooldown);
	ClassDB::bind_method(D_METHOD("get_cooldown"), &PhononSensorCondition::get_cooldown);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cooldown", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_cooldown", "get_cooldown");
	ADD_GROUP("", "");
}

// ---- Logic ----

bool PhononSensorCondition::accepts(const PhononHit &p_hit) const {
	if (require_direct && p_hit.bounces > 0) {
		return false;
	}
	if (source_tag != StringName() && p_hit.tag != source_tag) {
		return false;
	}
	if (pitch > 0.0f && std::fabs(p_hit.pitch - pitch) > pitch_tolerance) {
		return false;
	}
	return true;
}

float PhononSensorCondition::get_hit_energy(const PhononHit &p_hit) const {
	switch (band) {
		case BAND_LOW:
			return p_hit.energy[0];
		case BAND_MID:
			return p_hit.energy[1];
		case BAND_HIGH:
			return p_hit.energy[2];
		case BAND_ALL:
		default:
			return p_hit.total_energy();
	}
}

bool PhononSensorCondition::should_trigger(float p_energy, int p_packets) const {
	return p_energy >= min_energy && p_packets >= min_packets;
}

bool PhononSensorCondition::should_release(float p_energy) const {
	return p_energy < min_energy * release_ratio;
}

// ---- Config ----
// emit_changed(): Resource convention, tells anything using this condition that it was edited

void PhononSensorCondition::set_id(const StringName &p_id) { id = p_id; emit_changed(); }
StringName PhononSensorCondition::get_id() const { return id; }

void PhononSensorCondition::set_band(Band p_band) { band = p_band; emit_changed(); }
PhononSensorCondition::Band PhononSensorCondition::get_band() const { return band; }

void PhononSensorCondition::set_min_energy(float p_energy) { min_energy = MAX(p_energy, 0.0f); emit_changed(); }
float PhononSensorCondition::get_min_energy() const { return min_energy; }

void PhononSensorCondition::set_min_packets(int p_packets) { min_packets = MAX(p_packets, 0); emit_changed(); }
int PhononSensorCondition::get_min_packets() const { return min_packets; }

void PhononSensorCondition::set_pitch(float p_pitch) { pitch = MAX(p_pitch, 0.0f); emit_changed(); }
float PhononSensorCondition::get_pitch() const { return pitch; }

void PhononSensorCondition::set_pitch_tolerance(float p_tolerance) { pitch_tolerance = MAX(p_tolerance, 0.0f); emit_changed(); }
float PhononSensorCondition::get_pitch_tolerance() const { return pitch_tolerance; }

void PhononSensorCondition::set_source_tag(const StringName &p_tag) { source_tag = p_tag; emit_changed(); }
StringName PhononSensorCondition::get_source_tag() const { return source_tag; }

void PhononSensorCondition::set_require_direct(bool p_require) { require_direct = p_require; emit_changed(); }
bool PhononSensorCondition::get_require_direct() const { return require_direct; }

void PhononSensorCondition::set_release_ratio(float p_ratio) { release_ratio = CLAMP(p_ratio, 0.0f, 1.0f); emit_changed(); }
float PhononSensorCondition::get_release_ratio() const { return release_ratio; }

void PhononSensorCondition::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; emit_changed(); }
bool PhononSensorCondition::get_one_shot() const { return one_shot; }

void PhononSensorCondition::set_cooldown(float p_seconds) { cooldown = MAX(p_seconds, 0.0f); emit_changed(); }
float PhononSensorCondition::get_cooldown() const { return cooldown; }
