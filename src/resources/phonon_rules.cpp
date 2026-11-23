#include "resources/phonon_rules.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

#include <cmath>

// emit_changed() in setters: Resource convention, tells the condition/sensor using this rule that it was edited

// ---- PhononMinPackets ----

void PhononMinPackets::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_count", "count"), &PhononMinPackets::set_count);
	ClassDB::bind_method(D_METHOD("get_count"), &PhononMinPackets::get_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "count", PROPERTY_HINT_RANGE, "1,10000,1,or_greater"), "set_count", "get_count");
}

bool PhononMinPackets::is_met(float p_energy, int p_packets) const {
	return p_packets >= count;
}

bool PhononMinPackets::is_released(float p_energy, int p_packets, float p_release_ratio) const {
	return static_cast<float>(p_packets) < static_cast<float>(count) * p_release_ratio;
}

void PhononMinPackets::set_count(int p_count) { count = MAX(p_count, 1); emit_changed(); }
int PhononMinPackets::get_count() const { return count; }

// ---- PhononMinEnergy ----

void PhononMinEnergy::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_energy", "energy"), &PhononMinEnergy::set_energy);
	ClassDB::bind_method(D_METHOD("get_energy"), &PhononMinEnergy::get_energy);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "energy", PROPERTY_HINT_RANGE, "0,1,0.00001,or_greater"), "set_energy", "get_energy");
}

bool PhononMinEnergy::is_met(float p_energy, int p_packets) const {
	return p_energy >= energy;
}

bool PhononMinEnergy::is_released(float p_energy, int p_packets, float p_release_ratio) const {
	return p_energy < energy * p_release_ratio;
}

void PhononMinEnergy::set_energy(float p_energy) { energy = MAX(p_energy, 0.0f); emit_changed(); }
float PhononMinEnergy::get_energy() const { return energy; }

// ---- PhononFrequency ----

void PhononFrequency::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &PhononFrequency::set_frequency);
	ClassDB::bind_method(D_METHOD("get_frequency"), &PhononFrequency::get_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency", PROPERTY_HINT_RANGE, "20,20000,1,suffix:Hz"), "set_frequency", "get_frequency");

	ClassDB::bind_method(D_METHOD("set_tolerance", "tolerance"), &PhononFrequency::set_tolerance);
	ClassDB::bind_method(D_METHOD("get_tolerance"), &PhononFrequency::get_tolerance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tolerance", PROPERTY_HINT_RANGE, "0,5000,1,suffix:Hz"), "set_tolerance", "get_tolerance");
}

bool PhononFrequency::accepts(const PhononHit &p_hit) const {
	return std::fabs(p_hit.frequency - frequency) <= tolerance;
}

void PhononFrequency::set_frequency(float p_frequency) { frequency = MAX(p_frequency, 0.0f); emit_changed(); }
float PhononFrequency::get_frequency() const { return frequency; }

void PhononFrequency::set_tolerance(float p_tolerance) { tolerance = MAX(p_tolerance, 0.0f); emit_changed(); }
float PhononFrequency::get_tolerance() const { return tolerance; }

// ---- PhononSourceTag ----

void PhononSourceTag::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &PhononSourceTag::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &PhononSourceTag::get_tag);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");
}

bool PhononSourceTag::accepts(const PhononHit &p_hit) const {
	return p_hit.tag == tag;
}

void PhononSourceTag::set_tag(const StringName &p_tag) { tag = p_tag; emit_changed(); }
StringName PhononSourceTag::get_tag() const { return tag; }

// ---- PhononDirectOnly ----

bool PhononDirectOnly::accepts(const PhononHit &p_hit) const {
	return p_hit.bounces == 0;
}
