#include "nodes/phonon_sensor_2d.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void PhononSensor2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_condition_energy", "condition"), &PhononSensor2D::get_condition_energy);
	ClassDB::bind_method(D_METHOD("is_triggered", "condition"), &PhononSensor2D::is_triggered);
	ClassDB::bind_method(D_METHOD("reset_conditions"), &PhononSensor2D::reset_conditions);

	ClassDB::bind_method(D_METHOD("set_conditions", "conditions"), &PhononSensor2D::set_conditions);
	ClassDB::bind_method(D_METHOD("get_conditions"), &PhononSensor2D::get_conditions);
	// Typed array hint: the inspector shows "Array[PhononSensorCondition]" and only lets you add those
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "conditions", PROPERTY_HINT_TYPE_STRING,
						 String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) + ":PhononSensorCondition"),
			"set_conditions", "get_conditions");

	ADD_SIGNAL(MethodInfo("triggered",
			PropertyInfo(Variant::OBJECT, "condition", PROPERTY_HINT_RESOURCE_TYPE, "PhononSensorCondition"),
			PropertyInfo(Variant::FLOAT, "energy")));
	ADD_SIGNAL(MethodInfo("released",
			PropertyInfo(Variant::OBJECT, "condition", PROPERTY_HINT_RESOURCE_TYPE, "PhononSensorCondition")));
}

// ---- Lifecycle ----

void PhononSensor2D::_exit_tree() {
	PhononReceiver::_exit_tree(); // unregisters and clears hits
	for (ConditionState &state : states) {
		state.triggered = false; // hits are gone, so nothing is "currently loud". one_shot memory is kept
	}
}

// ---- Called by the server, once per tick after every hit is delivered ----

void PhononSensor2D::end_tick(float p_now) {
	PhononReceiver::end_tick(p_now); // drop expired hits first, so the window is up to date

	// GDScript can append to the array without going through set_conditions: grow/shrink state to match
	if (states.size() != static_cast<size_t>(conditions.size())) {
		states.resize(static_cast<size_t>(conditions.size()));
	}

	// Index loop + size re-check: a signal handler could edit `conditions` (which resizes `states`)
	for (int i = 0; i < conditions.size() && i < static_cast<int>(states.size()); i++) {
		const Ref<PhononSensorCondition> condition = conditions[i]; // Ref keeps it alive through the emit
		if (condition.is_null()) {
			continue; // empty slot in the inspector array
		}

		float energy = 0.0f;
		int packets = 0;
		measure(**condition, energy, packets);

		ConditionState &state = states[i];
		if (!state.triggered) {
			if (condition->get_one_shot() && state.has_fired) {
				continue; // already used up
			}
			if (p_now < state.cooldown_until) {
				continue;
			}
			if (condition->should_trigger(energy, packets)) {
				state.triggered = true;
				state.has_fired = true;
				state.cooldown_until = p_now + condition->get_cooldown();
				emit_signal("triggered", condition, energy);
			}
		} else if (condition->should_release(energy, packets)) {
			state.triggered = false;
			emit_signal("released", condition);
		}
	}
}

void PhononSensor2D::measure(const PhononSensorCondition &p_condition, float &r_energy, int &r_packets) const {
	float total = 0.0f;
	int packets = 0;
	hits.for_each([&](const PhononHit &hit) {
		if (p_condition.accepts(hit)) {
			total += hit.energy;
			packets++;
		}
	});
	r_energy = total / get_diameter(); // size-independent reading (rain gauge)
	r_packets = packets;
}

// ---- Queries ----

int PhononSensor2D::find_condition(const Ref<PhononSensorCondition> &p_condition) const {
	for (int i = 0; i < conditions.size(); i++) {
		if (Ref<PhononSensorCondition>(conditions[i]) == p_condition) {
			return i;
		}
	}
	return -1;
}

float PhononSensor2D::get_condition_energy(const Ref<PhononSensorCondition> &p_condition) const {
	ERR_FAIL_COND_V(p_condition.is_null(), 0.0f);
	float energy = 0.0f;
	int packets = 0;
	measure(**p_condition, energy, packets); // works for any condition, even one not in the list
	return energy;
}

bool PhononSensor2D::is_triggered(const Ref<PhononSensorCondition> &p_condition) const {
	const int i = find_condition(p_condition);
	return i >= 0 && i < static_cast<int>(states.size()) && states[i].triggered;
}

void PhononSensor2D::reset_conditions() {
	states.assign(states.size(), ConditionState());
}

// ---- Config ----

void PhononSensor2D::set_conditions(const TypedArray<PhononSensorCondition> &p_conditions) {
	conditions = p_conditions;
	states.assign(static_cast<size_t>(conditions.size()), ConditionState()); // fresh state per condition
}

TypedArray<PhononSensorCondition> PhononSensor2D::get_conditions() const { return conditions; }
