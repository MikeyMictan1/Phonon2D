#include "server/phonon_server.h"
#include "nodes/phonon_receiver.h"
#include "nodes/phonon_collision_shape_2d.h"
#include "resources/phonon_material.h"
#include "debug/ScopeTimer.h"

#include <godot_cpp/classes/collision_object2d.hpp>
#include <godot_cpp/classes/physics_direct_space_state2d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <algorithm>
#include <cmath>

// Static members need exactly one definition in one .cpp
PhononServer *PhononServer::singleton = nullptr;

void PhononServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_stats"), &PhononServer::get_stats);
	ClassDB::bind_method(D_METHOD("set_profiling_enabled", "enabled"), &PhononServer::set_profiling_enabled);
	ClassDB::bind_method(D_METHOD("is_profiling_enabled"), &PhononServer::is_profiling_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "profiling_enabled"), "set_profiling_enabled", "is_profiling_enabled");

	ClassDB::bind_method(D_METHOD("set_speed_of_sound", "speed"), &PhononServer::set_speed_of_sound);
	ClassDB::bind_method(D_METHOD("get_speed_of_sound"), &PhononServer::get_speed_of_sound);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_of_sound"), "set_speed_of_sound", "get_speed_of_sound");

	ClassDB::bind_method(D_METHOD("set_cull_threshold", "threshold"), &PhononServer::set_cull_threshold);
	ClassDB::bind_method(D_METHOD("get_cull_threshold"), &PhononServer::get_cull_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cull_threshold"), "set_cull_threshold", "get_cull_threshold");

	ClassDB::bind_method(D_METHOD("set_max_bounces", "max_bounces"), &PhononServer::set_max_bounces);
	ClassDB::bind_method(D_METHOD("get_max_bounces"), &PhononServer::get_max_bounces);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_bounces"), "set_max_bounces", "get_max_bounces");

	ClassDB::bind_method(D_METHOD("set_max_distance", "max_distance"), &PhononServer::set_max_distance);
	ClassDB::bind_method(D_METHOD("get_max_distance"), &PhononServer::get_max_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance"), "set_max_distance", "get_max_distance");

	ClassDB::bind_method(D_METHOD("set_collision_mask", "mask"), &PhononServer::set_collision_mask);
	ClassDB::bind_method(D_METHOD("get_collision_mask"), &PhononServer::get_collision_mask);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_2D_PHYSICS), "set_collision_mask", "get_collision_mask");

	ClassDB::bind_method(D_METHOD("set_default_absorption", "absorption"), &PhononServer::set_default_absorption);
	ClassDB::bind_method(D_METHOD("get_default_absorption"), &PhononServer::get_default_absorption);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "default_absorption", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_default_absorption", "get_default_absorption");

	ClassDB::bind_method(D_METHOD("get_active_packet_count"), &PhononServer::get_active_packet_count);
	ClassDB::bind_method(D_METHOD("clear_packets"), &PhononServer::clear_packets);
	ClassDB::bind_method(D_METHOD("get_sim_time"), &PhononServer::get_sim_time);

	ClassDB::bind_method(D_METHOD("set_air_absorption", "absorption"), &PhononServer::set_air_absorption);
	ClassDB::bind_method(D_METHOD("get_air_absorption"), &PhononServer::get_air_absorption);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_absorption", PROPERTY_HINT_RANGE, "0,0.01,0.0001"), "set_air_absorption", "get_air_absorption");
}

// ---- Singleton ----

PhononServer *PhononServer::get_singleton() {
	return singleton;
}

PhononServer::PhononServer() {
	ERR_FAIL_COND_MSG(singleton != nullptr, "PhononServer already exists; only one instance is allowed.");
	singleton = this;
}

PhononServer::~PhononServer() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

// ---- Emitter API ----

uint32_t PhononServer::new_emission_id() {
	const uint32_t id = next_emission_id++;
	if (next_emission_id == 0) {
		next_emission_id = 1; // wrapped after ~4 billion emissions: skip 0, it means "none"
	}
	return id;
}

PhononPacket *PhononServer::spawn_packet(const Vector2 &p_position, const Vector2 &p_direction, const EmissionInfo &p_emission) {
	PhononPacket *packet = packet_pool.request_packet();
	if (packet == nullptr) {
		return nullptr; // pool full: drop the packet (pool already logged it)
	}

	packet->position = p_position;
	packet->direction = p_direction.normalized(); // direction is always a unit vector
	packet->energy = p_emission.loudness; // starts as loud as the emitter, walls and air drain it
	packet->emission_id = p_emission.id;
	packet->share = p_emission.share;
	packet->frequency = p_emission.frequency;
	packet->tag = p_emission.tag;
	packet->source = p_emission.source;
	return packet;
}

double PhononServer::get_sim_time() const { return sim_time; }

// ---- Receiver API ----

void PhononServer::register_receiver(PhononReceiver *p_receiver) {
	ERR_FAIL_NULL(p_receiver);
	if (std::find(receivers.begin(), receivers.end(), p_receiver) == receivers.end()) {
		receivers.push_back(p_receiver);
	}
}

void PhononServer::unregister_receiver(PhononReceiver *p_receiver) {
	receivers.erase(std::remove(receivers.begin(), receivers.end(), p_receiver), receivers.end());
}

// ---- Simulation ----

void PhononServer::tick(double p_delta, PhysicsDirectSpaceState2D *p_space) {
	// Debugging/stats setup
	stats = PhononStats{};
	ScopeTimer tick_timer(stats.tick_ns);

	// Advance the clock first, so hits in this tick are stamped with the time at the end of the tick
	sim_time += p_delta;

	// How far every packet tries to move this tick (all packets share one speed: it's a property of the medium)
	const float step = speed_of_sound * static_cast<float>(p_delta);

	// Snapshot receiver circles once per tick (positions can change between ticks, not during one)
	receiver_circles.clear();
	for (PhononReceiver *receiver : receivers) {
		const float r = receiver->get_radius();
		receiver_circles.push_back({ receiver, receiver->get_global_position(), r * r });
	}

	// One query object reused for every ray this tick. Creating one per packet would mean thousands of allocations per frame.
	Ref<PhysicsRayQueryParameters2D> query;
	if (p_space != nullptr) {
		query.instantiate();
		query->set_collision_mask(collision_mask);
	}

	// Loop through every active packet in the pool
	packet_pool.for_each_active([&](PhononPacket &packet) {
		stats.active_packets++;

		// Raycast along the path the packet is about to take. Raycasting the whole step (instead of the end point) stops fast packets going through thin walls
		Dictionary hit;
		if (p_space != nullptr) {
			query->set_from(packet.position);
			query->set_to(packet.position + packet.direction * step);

			// Benchmarking
			if (profiling_enabled) { 
				ScopeTimer ray_timer(stats.raycast_ns); 
				stats.raycast_count++;
				hit = p_space->intersect_ray(query);
			}
			else { hit = p_space->intersect_ray(query); }
		}

		// Snapshot before moving: detection needs where it started and its energy before the wall/air took a cut
		StepStart start;
		if (!receiver_circles.empty()) {
			start.position = packet.position;
			start.direction = packet.direction;
			start.energy = packet.energy;
			start.bounce_count = packet.bounce_count;
		}

		// If we arent gonna hit any walls, then just advance the packet along its path. OTherwise we are, so bounce it against that surface
		if (hit.is_empty()) { advance_packet(packet, step); }
		else { bounce_packet(packet, hit); }

		// Did this step's path cross into any receiver?
		if (!receiver_circles.empty()) { detect_receivers(packet, start); }

		// Recylcing packets that need to die
		if (should_cull(packet)) { packet_pool.recycle_packet(packet); }
	});

	// Every hit for this tick is in: let receivers drop old hits and (later) check conditions / set volumes.
	// Loops the snapshot, not `receivers`: end_tick may emit signals whose handlers add/remove receivers,
	// which would reshuffle `receivers` mid-loop. (queue_free() is deferred, so snapshot pointers stay valid.)
	const float now = static_cast<float>(sim_time);
	for (const ReceiverCircle &circle : receiver_circles) {
		circle.receiver->end_tick(now);
	}
}

// Entry-only test: a hit counts when the packet starts the step OUTSIDE a circle and its path reaches inside.
// A packet sitting inside the circle for several ticks is therefore counted once, not once per tick.
void PhononServer::detect_receivers(const PhononPacket &p_packet, const StepStart &p_start) {
	const Vector2 start = p_start.position;
	const Vector2 path = p_packet.position - start;
	const float path_len_sq = path.length_squared();

	for (const ReceiverCircle &circle : receiver_circles) {
		const Vector2 to_centre = circle.centre - start;
		if (to_centre.length_squared() <= circle.radius_sq) {
			continue; // started inside: already counted when it entered
		}

		// Closest point on the segment to the centre: project onto the path, clamp to the segment's ends
		float t = 0.0f;
		if (path_len_sq > 0.0f) {
			t = CLAMP(to_centre.dot(path) / path_len_sq, 0.0f, 1.0f);
		}
		const Vector2 closest = start + path * t;
		if (closest.distance_squared_to(circle.centre) > circle.radius_sq) {
			continue; // passed by without touching
		}

		PhononHit hit;
		hit.time = static_cast<float>(sim_time);
		hit.energy = p_start.energy * p_packet.share; // energy left, split across the wavefront's packets
		hit.frequency = p_packet.frequency;
		hit.direction = p_start.direction;
		hit.bounces = p_start.bounce_count;
		hit.emission_id = p_packet.emission_id;
		hit.tag = p_packet.tag;
	hit.source = p_packet.source;
		circle.receiver->add_hit(hit);
	}
}

// No collision: move the full step
void PhononServer::advance_packet(PhononPacket &p_packet, float p_distance) {
	p_packet.position += p_packet.direction * p_distance;
	p_packet.distance_travelled += p_distance;
	apply_air_absorption(p_packet, p_distance);
}

// Air absorption, slightly reduce energy based on distance travelled (Beer-Lambert formula applied to sound).
void PhononServer::apply_air_absorption(PhononPacket &p_packet, float p_distance) {
	p_packet.energy *= std::exp(-air_absorption * p_distance);
}

// Collision: move to the hit point, reflect off the surface, lose energy to the material.
void PhononServer::bounce_packet(PhononPacket &p_packet, const Dictionary &p_hit) {
	const Vector2 hit_position = p_hit["position"];
	const Vector2 normal = p_hit["normal"]; // Normal of the wall/surface collided with
	Object *collider = p_hit["collider"];
	const int shape_index = p_hit["shape"]; // which of the collider's shapes the ray hit
	const float distance =  p_packet.position.distance_to(hit_position);

	p_packet.distance_travelled += distance;
	p_packet.position = hit_position + normal * BOUNCE_OFFSET; // nudge off the wall
	p_packet.direction = p_packet.direction.bounce(normal);    // d' = d - 2(d.n)n
	p_packet.energy *= 1.0f - get_surface_absorption(collider, shape_index); // the material keeps its share
	apply_air_absorption(p_packet, distance);
	p_packet.bounce_count++;
}

// Absorption of the exact shape that was hit: its PhononMaterial if it's a PhononCollisionShape2D
// with one set, otherwise the default (hard, reflective surface).
float PhononServer::get_surface_absorption(Object *p_collider, int p_shape_index) const {
	// The ray reports the body plus a shape index; map the index back to the shape node that owns it
	const CollisionObject2D *body = Object::cast_to<CollisionObject2D>(p_collider);
	if (body == nullptr) {
		return default_absorption; // not a CollisionObject2D (e.g. a TileMapLayer): no shape nodes to check
	}

	const uint32_t owner_id = body->shape_find_owner(p_shape_index);
	const PhononCollisionShape2D *shape = Object::cast_to<PhononCollisionShape2D>(body->shape_owner_get_owner(owner_id));
	if (shape == nullptr || shape->get_phonon_material().is_null()) {
		return default_absorption; // plain CollisionShape2D / polygon, or no material assigned
	}
	return shape->get_phonon_material()->get_absorption();
}

bool PhononServer::should_cull(const PhononPacket &p_packet) const {
	return p_packet.energy < cull_threshold
		|| p_packet.bounce_count > max_bounces
		|| p_packet.distance_travelled >= max_distance;
}

// ---- Config ----

void PhononServer::set_speed_of_sound(float p_speed) { speed_of_sound = p_speed; }
float PhononServer::get_speed_of_sound() const { return speed_of_sound; }

void PhononServer::set_cull_threshold(float p_threshold) { cull_threshold = p_threshold; }
float PhononServer::get_cull_threshold() const { return cull_threshold; }

void PhononServer::set_max_bounces(int p_max_bounces) { max_bounces = p_max_bounces; }
int PhononServer::get_max_bounces() const { return max_bounces; }

void PhononServer::set_max_distance(float p_max_distance) { max_distance = p_max_distance; }
float PhononServer::get_max_distance() const { return max_distance; }

void PhononServer::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }
uint32_t PhononServer::get_collision_mask() const { return collision_mask; }

void PhononServer::set_default_absorption(float p_absorption) { default_absorption = CLAMP(p_absorption, 0.0f, 1.0f); }
float PhononServer::get_default_absorption() const { return default_absorption; }

void PhononServer::set_air_absorption(float p_absorption) { air_absorption = MAX(p_absorption, 0.0f); } // negative would add energy
float PhononServer::get_air_absorption() const { return air_absorption; }

// ---- Debug ----

int PhononServer::get_active_packet_count() const {
	return static_cast<int>(packet_pool.get_active_count());
}

void PhononServer::clear_packets() {
	packet_pool.clear_pool();
}

// Converted to milliseconds only here, for display
Dictionary PhononServer::get_stats() const {
	Dictionary d;
	d["tick_ms"] = stats.tick_ns / 1.0e6;
	d["raycast_ms"] = stats.raycast_ns / 1.0e6;
	d["raycast_count"] = stats.raycast_count;
	d["active_packets"] = stats.active_packets;
	return d;
}

void PhononServer::set_profiling_enabled(bool p_enabled) { profiling_enabled = p_enabled; }
bool PhononServer::is_profiling_enabled() const { return profiling_enabled; }
