#include "resources/phonon_sensor_condition.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

void PhononSensorCondition::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_id", "id"), &PhononSensorCondition::set_id);
	ClassDB::bind_method(D_METHOD("get_id"), &PhononSensorCondition::get_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "id"), "set_id", "get_id");

	ClassDB::bind_method(D_METHOD("set_rules", "rules"), &PhononSensorCondition::set_rules);
	ClassDB::bind_method(D_METHOD("get_rules"), &PhononSensorCondition::get_rules);
	// Typed array hint: "Add Element" then the slot's dropdown lists every PhononRule subclass
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules", PROPERTY_HINT_TYPE_STRING,
						 String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":PhononRule"),
			"set_rules", "get_rules");

	// Timing: collapsed group, all off by default
	ADD_GROUP("Timing", "");
	ClassDB::bind_method(D_METHOD("set_one_shot", "one_shot"), &PhononSensorCondition::set_one_shot);
	ClassDB::bind_method(D_METHOD("get_one_shot"), &PhononSensorCondition::get_one_shot);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "one_shot"), "set_one_shot", "get_one_shot");

	ClassDB::bind_method(D_METHOD("set_cooldown", "seconds"), &PhononSensorCondition::set_cooldown);
	ClassDB::bind_method(D_METHOD("get_cooldown"), &PhononSensorCondition::get_cooldown);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cooldown", PROPERTY_HINT_RANGE, "0,60,0.01,or_greater,suffix:s"), "set_cooldown", "get_cooldown");

	ClassDB::bind_method(D_METHOD("set_release_ratio", "ratio"), &PhononSensorCondition::set_release_ratio);
	ClassDB::bind_method(D_METHOD("get_release_ratio"), &PhononSensorCondition::get_release_ratio);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "release_ratio", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_release_ratio", "get_release_ratio");
	ADD_GROUP("", "");
}

// ---- Logic ----

bool PhononSensorCondition::accepts(const PhononHit &p_hit) const {
	for (const Ref<PhononRule> &rule : get_rule_cache()) {
		if (rule.is_valid() && !rule->accepts(p_hit)) {
			return false;
		}
	}
	return true;
}

// Needs at least one accepted hit (silence never triggers), then every threshold rule must be met.
// Filter rules return true from is_met, so they don't block it.
bool PhononSensorCondition::should_trigger(float p_energy, int p_packets) const {
	if (p_packets == 0) {
		return false;
	}
	for (const Ref<PhononRule> &rule : get_rule_cache()) {
		if (rule.is_valid() && !rule->is_met(p_energy, p_packets)) {
			return false;
		}
	}
	return true;
}

// Trigger needs EVERY threshold, so release when ANY falls below its release line.
// An empty window always releases, so a condition with no thresholds re-arms once the sound is gone.
bool PhononSensorCondition::should_release(float p_energy, int p_packets) const {
	if (p_packets == 0) {
		return true;
	}
	for (const Ref<PhononRule> &rule : get_rule_cache()) {
		if (rule.is_valid() && rule->is_released(p_energy, p_packets, release_ratio)) {
			return true;
		}
	}
	return false;
}

const std::vector<Ref<PhononRule>> &PhononSensorCondition::get_rule_cache() const {
	if (rule_cache.size() != static_cast<size_t>(rules.size())) {
		rebuild_rule_cache();
	}
	return rule_cache;
}

void PhononSensorCondition::rebuild_rule_cache() const {
	rule_cache.clear();
	for (int i = 0; i < rules.size(); i++) {
		rule_cache.push_back(rules[i]);
	}
}

// ---- Config ----
// emit_changed(): Resource convention, tells anything using this condition that it was edited

void PhononSensorCondition::set_id(const StringName &p_id) { id = p_id; emit_changed(); }
StringName PhononSensorCondition::get_id() const { return id; }

void PhononSensorCondition::set_rules(const TypedArray<PhononRule> &p_rules) {
	rules = p_rules;
	rebuild_rule_cache();
	emit_changed();
}
TypedArray<PhononRule> PhononSensorCondition::get_rules() const { return rules; }

void PhononSensorCondition::set_release_ratio(float p_ratio) { release_ratio = CLAMP(p_ratio, 0.0f, 1.0f); emit_changed(); }
float PhononSensorCondition::get_release_ratio() const { return release_ratio; }

void PhononSensorCondition::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; emit_changed(); }
bool PhononSensorCondition::get_one_shot() const { return one_shot; }

void PhononSensorCondition::set_cooldown(float p_seconds) { cooldown = MAX(p_seconds, 0.0f); emit_changed(); }
float PhononSensorCondition::get_cooldown() const { return cooldown; }
