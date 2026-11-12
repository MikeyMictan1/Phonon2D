#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "server/phonon_packet_pool.h"

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
	// Metadata key a collider uses to point at its PhononMaterial: wall.set_meta("phonon_material", mat)
	static constexpr const char *MATERIAL_META_KEY = "phonon_material";

	// Singleton created and destroyed in register_types.cpp (memnew / memdelete),
	static PhononServer *get_singleton();

	PhononServer();
	~PhononServer();

	// Emitter API: Pulls a packet from the pool and initializes it. Returns nullptr if the pool is full (packet dropped).
	PhononPacket *spawn_packet(const Vector2 &p_position, const Vector2 &p_direction, float p_intensity, float p_frequency);

	// Receiver API: Receivers add themselves when they enter the tree and remove themselves when they leave.
	void register_receiver(PhononReceiver *p_receiver);
	void unregister_receiver(PhononReceiver *p_receiver);

	// Simulation: advances every active packet by one step, bouncing off colliders in p_space.
	// Called once per physics frame by PhononWorld2D. p_space may be null (packets then fly through everything).
	void tick(double p_delta, PhysicsDirectSpaceState2D *p_space);

	// Read-only access to active packets, for drawing/debugging. Same idea as the pool's for_each_active.
	template <typename Func>
	void for_each_active_packet(Func &&p_func) const {
		packet_pool.for_each_active(std::forward<Func>(p_func));
	}

	int get_active_packet_count() const; // debug, bound to GDScript

	// Config (bound to GDScript as properties)
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

private:
	static PhononServer *singleton;

	// How far off the wall a packet is placed after a bounce, so next tick's ray doesn't start on the surface
	static constexpr float BOUNCE_OFFSET = 0.01f;

	PhononPacketPool packet_pool;
	std::vector<PhononReceiver *> receivers; // non-owning: receivers are scene nodes, Godot owns them

	float speed_of_sound = 343.0f; // units per second (decide: pixels or metres?)
	float cull_threshold = 0.01f;  // packets below this intensity are recycled
	int max_bounces = 8;
	float max_distance = 2000.0f;
	uint32_t collision_mask = 1u << 4; // physics layer 5 ("phonon"): only these colliders affect sound
	float default_absorption = 0.1f;   // used when a collider has no PhononMaterial (hard, reflective surface)

	// tick helpers
	void advance_packet(PhononPacket &p_packet, float p_distance);
	void bounce_packet(PhononPacket &p_packet, const Dictionary &p_hit);
	float get_surface_absorption(Object *p_collider) const;
	bool should_cull(const PhononPacket &p_packet) const;
};
