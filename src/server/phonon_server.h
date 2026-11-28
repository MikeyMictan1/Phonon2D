#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "server/phonon_packet_pool.h"
#include "debug/phonon_stats.h"

#include <cstdint>
#include <utility>
#include <vector>

using namespace godot;

namespace godot {
class PhysicsDirectSpaceState2D; // forward declaration: tick only takes a pointer to it
}
class PhononReceiver; // forward declaration: the server only stores pointers, so it doesn't need the full class

/**
 * Mediator singleton server. Emitters hand it new phonons; it advances packets each
 * tick, resolves collisions against materials and dispatches to receivers.
 */
class PhononServer : public Object {
	GDCLASS(PhononServer, Object)

protected:
	static void _bind_methods();

public:
	// Singleton created and destroyed in register_types.cpp (memnew / memdelete),
	static PhononServer *get_singleton();

	PhononServer();
	~PhononServer();

	// Benchmarking/Debugging -------------------------------------------------------------
	Dictionary get_stats() const;
	void set_profiling_enabled(bool p_enabled);
	bool is_profiling_enabled() const;

	// Emitter API -----------------------------------------------------------------------
	// Everything shared by every packet of one emitter play() call
	struct EmissionInfo {
		uint32_t id = 0;     // from new_emission_id()
		float loudness = 0.0f;  // the emitter's loudness: each packet's starting energy
		float share = 0.0f;     // 1 / packet_count
		float frequency = 0.0f; // gameplay label in Hz
		StringName tag;
		ObjectID source;
	};

	// Call once per emit, before spawning its packets. Unique id so receivers can group hits by sound.
	uint32_t new_emission_id();

	// p_emission is copied into the packet
	PhononPacket *spawn_packet(const Vector2 &p_position, const Vector2 &p_direction, const EmissionInfo &p_emission);

	// Seconds simulated so far (sum of every tick's delta). Hits are timestamped with this. Bound to GDScript.
	double get_sim_time() const;

	// Receiver API -----------------------------------------------------------------------
	void register_receiver(PhononReceiver *p_receiver);
	void unregister_receiver(PhononReceiver *p_receiver);

	// Simulation -----------------------------------------------------------------------
	// Called once per physics frame by PhononWorld2D. p_space may be null (packets then fly through everything).
	void tick(double p_delta, PhysicsDirectSpaceState2D *p_space);

	// Read-only access to active packets, for drawing/debugging. Same idea as the pool's for_each_active.
	template <typename Func>
	void for_each_active_packet(Func &&p_func) const {packet_pool.for_each_active(std::forward<Func>(p_func));}

	int get_active_packet_count() const; // debug, bound to GDScript

	// Deactivates every packet (e.g. when a scene's PhononWorld2D leaves the tree). Bound to GDScript.
	void clear_packets();

	// Config (bound to GDScript as properties) -----------------------------------------------------------------------
	void set_speed_of_sound(float p_speed);
	float get_speed_of_sound() const;

	void set_cull_threshold(float p_threshold);
	float get_cull_threshold() const;

	void set_max_bounces(int p_max_bounces);
	int get_max_bounces() const;

	void set_max_distance(float p_max_distance);
	float get_max_distance() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_default_absorption(float p_absorption);
	float get_default_absorption() const;

	void set_air_absorption(float p_absorption);
	float get_air_absorption() const;

private:
	static PhononServer *singleton;

	// Debugging
	PhononStats stats;
	bool profiling_enabled = false;

	// How far off the wall a packet is placed after a bounce, so next tick's ray doesn't start on the surface
	static constexpr float BOUNCE_OFFSET = 0.01f;

	PhononPacketPool packet_pool;
	std::vector<PhononReceiver *> receivers; // non-owning: receivers are scene nodes, Godot owns them

	uint32_t next_emission_id = 1; // 0 means "no emission"

	double sim_time = 0.0; // double: a float loses millisecond precision after a few hours

	// Per-tick snapshot of each receiver's circle, so the packet loop doesn't call into Godot for every packet
	struct ReceiverCircle {
		PhononReceiver *receiver;
		Vector2 centre;
		float radius_sq;
	};
	std::vector<ReceiverCircle> receiver_circles; // member, not a local: reused every tick, no allocation

	float speed_of_sound = 343.0f; // units per second (decide: pixels or metres?)
	float cull_threshold = 0.01f;  // packets whose energy drops below this are recycled
	int max_bounces = 8;
	float max_distance = 2000.0f;
	uint32_t collision_mask = 1u << 4; // physics layer 5 ("phonon"): only these colliders affect sound
	float default_absorption = 0.1f;
	float air_absorption = 0.001f;

	// tick helpers
	void advance_packet(PhononPacket &p_packet, float p_distance);
	void apply_air_absorption(PhononPacket &p_packet, float p_distance);
	void bounce_packet(PhononPacket &p_packet, const Dictionary &p_hit);
	float get_surface_absorption(Object *p_collider, int p_shape_index) const;
	bool should_cull(const PhononPacket &p_packet) const;

	// What detection needs from a packet as it was at the START of this tick (before moving/absorbing).
	// Plain numbers only: copying the whole packet every tick would also copy its StringName tag (a refcount update).
	struct StepStart {
		Vector2 position;
		Vector2 direction;
		float energy;
		int bounce_count;
	};
	// p_packet: the packet after moving (for its per-emission data). p_start: snapshot from before the move.
	void detect_receivers(const PhononPacket &p_packet, const StepStart &p_start);
};
