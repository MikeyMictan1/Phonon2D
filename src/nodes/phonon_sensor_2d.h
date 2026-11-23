#pragma once

#include "nodes/phonon_receiver.h"
#include "resources/phonon_sensor_condition.h"

#include <godot_cpp/variant/typed_array.hpp>

#include <vector>

using namespace godot;

/**
 * Gameplay node. Watches its rolling window of hits and fires signals when its conditions are met,
 * e.g. "enough 880 Hz sound arrived in the last 0.5 s -> shatter the glass".
 *
 * Each tick (in end_tick, after the server has delivered every hit):
 *   for each condition: add up the energy of the hits it accepts, divide by diameter, then
 *   - not triggered and above min_energy (and min_packets)  -> triggered(condition, energy)
 *   - triggered and below min_energy * release_ratio         -> released(condition)
 * Edge-triggered: signals fire on the change, not every tick while it stays loud.
 */
class PhononSensor2D : public PhononReceiver {
	GDCLASS(PhononSensor2D, PhononReceiver)

public:
	void _exit_tree() override;
	void end_tick(float p_now) override;

	// Queries (bound to GDScript) -----------------------------------------------------------------
	// Current window reading for one condition: energy of the hits it accepts, divided by diameter
	float get_condition_energy(const Ref<PhononSensorCondition> &p_condition) const;
	bool is_triggered(const Ref<PhononSensorCondition> &p_condition) const;

	// Re-arms everything: clears triggered flags, cooldowns and one_shot "already fired" flags
	void reset_conditions();

	// Config ---------------------------------------------------------------------------------------
	void set_conditions(const TypedArray<PhononSensorCondition> &p_conditions);
	TypedArray<PhononSensorCondition> get_conditions() const;

protected:
	static void _bind_methods();

private:
	// Runtime state per condition, same index as `conditions`. Lives here, not in the resource,
	// because one condition resource can be shared by several sensors.
	struct ConditionState {
		bool triggered = false;
		bool has_fired = false;      // for one_shot
		float cooldown_until = 0.0f; // sim time before which it can't trigger again
	};

	TypedArray<PhononSensorCondition> conditions;
	std::vector<ConditionState> states;

	// Sum of accepted hits' energy (divided by diameter) and how many hits were accepted
	void measure(const PhononSensorCondition &p_condition, float &r_energy, int &r_packets) const;
	int find_condition(const Ref<PhononSensorCondition> &p_condition) const;
};
