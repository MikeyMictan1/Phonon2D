#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include "server/phonon_packet_pool.h"
#include "debug/phonon_stats.h"
#include "raycast/phonon_geometry.h"
#include "raycast/ray_hit.h"

#include <cstdint>
#include <utility>
#include <vector>

using namespace godot;

namespace godot {
class PhysicsDirectSpaceState2D; // forward declaration: tick only takes a pointer to it
class Node;
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
	PhononPacket *spawn_packet(const Vector2 &p_position, const Vector2 &p_direction, float p_intensity, float p_frequency);

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

	// Geometry (custom raycaster) ---------------------------------------------------------------
	// Which part of the scene to bake walls from (PhononWorld2D passes the tree root). Marks geometry dirty.
	void set_geometry_root(Node *p_root);
	// Rebake before the next tick. Connected to scene/material change signals; also bound to GDScript.
	void mark_geometry_dirty();
	void clear_geometry();
	const PhononGeometry &get_geometry() const { return geometry; } // for debug drawing

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

	// true: our own PhononGeometry raycaster. false: Godot's intersect_ray (kept for comparison)
	void set_use_custom_raycast(bool p_enabled);
	bool is_using_custom_raycast() const;

	void set_geometry_cell_size(float p_size);
	float get_geometry_cell_size() const;

private:
	static PhononServer *singleton;

	// Debugging
	PhononStats stats;
	bool profiling_enabled = false;

	// How far off the wall a packet is placed after a bounce, so next tick's ray doesn't start on the surface
	static constexpr float BOUNCE_OFFSET = 0.01f;

	PhononPacketPool packet_pool;

	// Custom raycaster
	PhononGeometry geometry;
	bool use_custom_raycast = true;
	bool geometry_dirty = true;
	uint64_t geometry_root_id = 0; // ObjectID of the bake root (an ID, not a pointer, so a freed root is detected)
	float geometry_cell_size = PhononGeometry::DEFAULT_CELL_SIZE;
	int64_t last_bake_ns = 0; // kept across ticks (stats is reset every tick)
	int bake_count = 0;
	std::vector<PhononReceiver *> receivers; // non-owning: receivers are scene nodes, Godot owns them

	float speed_of_sound = 343.0f; // units per second (decide: pixels or metres?)
	float cull_threshold = 0.01f;  // packets below this intensity are recycled
	int max_bounces = 8;
	float max_distance = 2000.0f;
	uint32_t collision_mask = 1u << 4; // physics layer 5 ("phonon"): only these colliders affect sound
	float default_absorption = 0.1f;
	float air_absorption = 0.001f;

	// tick helpers
	void advance_packet(PhononPacket &p_packet, float p_distance);
	void apply_air_absorption(PhononPacket &p_packet, float p_distance);
	void bounce_packet(PhononPacket &p_packet, const RayHit &p_hit);
	RayHit hit_from_dictionary(const Dictionary &p_hit, const Vector2 &p_from) const; // Godot path -> same struct
	void rebuild_geometry();
	float get_surface_absorption(Object *p_collider, int p_shape_index) const;
	bool should_cull(const PhononPacket &p_packet) const;
};
