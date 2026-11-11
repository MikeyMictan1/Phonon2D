#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "server/phonon_packet_pool.h"

#include <vector>

using namespace godot;

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

	// Emitter API: Pulls a packet from the pool and initializes it. Returns nullptr if the pool is full (packet dropped).
	PhononPacket *spawn_packet(const Vector2 &p_position, const Vector2 &p_direction, float p_intensity, float p_frequency);

	// Receiver API: Receivers add themselves when they enter the tree and remove themselves when they leave.
	void register_receiver(PhononReceiver *p_receiver);
	void unregister_receiver(PhononReceiver *p_receiver);

	// Simulation: Advances every active packet by one step. Who calls this each frame is still to be decided.
	void tick(double p_delta);

	// Config (bound to GDScript as properties)
	void set_speed_of_sound(float p_speed);
	float get_speed_of_sound() const;

	void set_cull_threshold(float p_threshold);
	float get_cull_threshold() const;

	void set_max_bounces(int p_max_bounces);
	int get_max_bounces() const;

	void set_max_distance(float p_max_distance);
	float get_max_distance() const;

private:
	static PhononServer *singleton;

	PhononPacketPool packet_pool;
	std::vector<PhononReceiver *> receivers; // non-owning: receivers are scene nodes, Godot owns them

	float speed_of_sound = 343.0f; // units per second (decide: pixels or metres?)
	float cull_threshold = 0.01f;  // packets below this intensity are recycled
	int max_bounces = 8;
	float max_distance = 2000.0f;

	// tick helpers
	void advance_packet(PhononPacket &p_packet, float p_delta);
	bool should_cull(const PhononPacket &p_packet) const;
};
