#include "server/phonon_server.h"
#include "nodes/phonon_receiver.h"
#include "nodes/phonon_collision_shape_2d.h"
#include "resources/phonon_material.h"
#include "debug/ScopeTimer.h"
#include "raycast/geometry_baker.h"

#include <godot_cpp/classes/collision_object2d.hpp>
#include <godot_cpp/classes/node.hpp>
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

	ClassDB::bind_method(D_METHOD("set_air_absorption", "absorption"), &PhononServer::set_air_absorption);
	ClassDB::bind_method(D_METHOD("get_air_absorption"), &PhononServer::get_air_absorption);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "air_absorption", PROPERTY_HINT_RANGE, "0,0.01,0.0001"), "set_air_absorption", "get_air_absorption");

	ClassDB::bind_method(D_METHOD("set_use_custom_raycast", "enabled"), &PhononServer::set_use_custom_raycast);
	ClassDB::bind_method(D_METHOD("is_using_custom_raycast"), &PhononServer::is_using_custom_raycast);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_custom_raycast"), "set_use_custom_raycast", "is_using_custom_raycast");

	ClassDB::bind_method(D_METHOD("set_geometry_cell_size", "size"), &PhononServer::set_geometry_cell_size);
	ClassDB::bind_method(D_METHOD("get_geometry_cell_size"), &PhononServer::get_geometry_cell_size);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "geometry_cell_size", PROPERTY_HINT_RANGE, "8,512,1"), "set_geometry_cell_size", "get_geometry_cell_size");

	ClassDB::bind_method(D_METHOD("mark_geometry_dirty"), &PhononServer::mark_geometry_dirty);
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

PhononPacket *PhononServer::spawn_packet(const Vector2 &p_position, const Vector2 &p_direction, float p_intensity, float p_frequency) {
	PhononPacket *packet = packet_pool.request_packet();
	if (packet == nullptr) {
		return nullptr; // pool full: drop the packet (pool already logged it)
	}

	packet->position = p_position;
	packet->direction = p_direction.normalized(); // direction is always a unit vector
	packet->intensity = p_intensity;
	packet->frequency = p_frequency;
	return packet;
}

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

// ---- Geometry ----

void PhononServer::set_geometry_root(Node *p_root) {
	geometry_root_id = p_root != nullptr ? p_root->get_instance_id() : 0;
	geometry_dirty = true;
}

void PhononServer::mark_geometry_dirty() { geometry_dirty = true; }

void PhononServer::clear_geometry() {
	geometry.clear();
	geometry_root_id = 0;
	geometry_dirty = true;
}

// Turn the scene's colliders into wall segments. All the engine calls of the custom raycaster happen here.
void PhononServer::rebuild_geometry() {
	geometry_dirty = false;
	Node *root = Object::cast_to<Node>(ObjectDB::get_instance(geometry_root_id));
	if (root == nullptr) {
		geometry.clear();
		return;
	}

	GeometryBaker::Settings settings;
	settings.collision_mask = collision_mask;
	settings.default_absorption = default_absorption;
	settings.cell_size = geometry_cell_size;
	settings.on_changed = Callable(this, "mark_geometry_dirty");

	last_bake_ns = 0;
	ScopeTimer bake_timer(last_bake_ns);
	GeometryBaker::bake(root, settings, geometry);
	bake_count++;
}

// ---- Simulation ----

void PhononServer::tick(double p_delta, PhysicsDirectSpaceState2D *p_space) {
	// Debugging/stats setup
	stats = PhononStats{};
	ScopeTimer tick_timer(stats.tick_ns);

	// How far every packet tries to move this tick (all packets share one speed: it's a property of the medium)
	const float step = speed_of_sound * static_cast<float>(p_delta);

<<<<<<< Updated upstream
	// One query object reused for every ray this tick. Creating one per packet would mean
	// thousands of allocations per frame.
=======
	// Custom raycaster: rebake if something changed, then move dynamic walls to where their bodies are now.
	// Both cost per wall / moving body, not per packet.
	if (use_custom_raycast) {
		if (geometry_dirty) {
			rebuild_geometry();
		}
		ScopeTimer sync_timer(stats.dynamic_sync_ns);
		stats.dynamic_bodies_moved = GeometryBaker::sync_dynamic(geometry);
	}

	// Godot raycaster: one query object reused for every ray this tick. Creating one per packet would mean
	// thousands of allocations per frame.
	const bool use_godot_raycast = !use_custom_raycast && p_space != nullptr;
>>>>>>> Stashed changes
	Ref<PhysicsRayQueryParameters2D> query;
	if (use_godot_raycast) {
		query.instantiate();
		query->set_collision_mask(collision_mask);
	}

	packet_pool.for_each_active([&](PhononPacket &packet) {
		stats.active_packets++; // always counted: one add per packet, effectively free

<<<<<<< Updated upstream
		// Cast a ray along the path the packet is about to take. Raycasting the whole step
		// (instead of checking the end point) stops fast packets tunnelling through thin walls.
		Dictionary hit;
		if (p_space != nullptr) {
=======
		// Raycast along the path the packet is about to take. Raycasting the whole step (instead of the end point) stops fast packets going through thin walls
		RayHit hit;
		bool did_hit = false;

		if (use_custom_raycast) {
			// Our geometry: plain arrays, no engine calls, result straight into a struct
			if (profiling_enabled) {
				ScopeTimer ray_timer(stats.raycast_ns);
				stats.raycast_count++;
				did_hit = geometry.cast(packet.position, packet.direction, step, hit);
			} else {
				did_hit = geometry.cast(packet.position, packet.direction, step, hit);
			}
		} else if (use_godot_raycast) {
>>>>>>> Stashed changes
			query->set_from(packet.position);
			query->set_to(packet.position + packet.direction * step);

			Dictionary result;
			if (profiling_enabled) {
				ScopeTimer ray_timer(stats.raycast_ns);
				stats.raycast_count++;
				result = p_space->intersect_ray(query);
			} else {
				result = p_space->intersect_ray(query);
			}
			if (!result.is_empty()) {
				hit = hit_from_dictionary(result, packet.position);
				did_hit = true;
			}
<<<<<<< Updated upstream
			else { hit = p_space->intersect_ray(query); }
			
		}

		if (hit.is_empty()) {
			advance_packet(packet, step);
		} else {
			bounce_packet(packet, hit);
		}
=======
		}

		// If we arent gonna hit any walls, then just advance the packet along its path. Otherwise we are, so bounce it against that surface
		if (did_hit) { bounce_packet(packet, hit); }
		else { advance_packet(packet, step); }
>>>>>>> Stashed changes

		if (should_cull(packet)) {
			packet_pool.recycle_packet(packet);
		}
	});
}

// No collision: move the full step
void PhononServer::advance_packet(PhononPacket &p_packet, float p_distance) {
	p_packet.position += p_packet.direction * p_distance;
	p_packet.distance_travelled += p_distance;
	apply_air_absorption(p_packet, p_distance);
}

// Air absorption, slightly reduce intensity based on distance travelled (Beer-Lambert formula applied to sound)
void PhononServer::apply_air_absorption(PhononPacket &p_packet, float p_distance) {
	p_packet.intensity *= std::exp(-air_absorption * p_distance);
}

// Collision: move to the hit point, reflect off the surface, lose energy to the material.
// Same code for both raycasters: each produces a RayHit.
void PhononServer::bounce_packet(PhononPacket &p_packet, const RayHit &p_hit) {
	p_packet.distance_travelled += p_hit.distance;
	p_packet.position = p_hit.position + p_hit.normal * BOUNCE_OFFSET; // nudge off the wall
	p_packet.direction = p_packet.direction.bounce(p_hit.normal);      // d' = d - 2(d.n)n
	p_packet.intensity *= 1.0f - p_hit.absorption;
	apply_air_absorption(p_packet, p_hit.distance);
	p_packet.bounce_count++;
}

// Godot raycaster only: read the Dictionary intersect_ray returns into a RayHit
RayHit PhononServer::hit_from_dictionary(const Dictionary &p_hit, const Vector2 &p_from) const {
	RayHit hit;
	hit.position = p_hit["position"];
	hit.normal = p_hit["normal"]; // Normal of the wall/surface collided with
	hit.distance = p_from.distance_to(hit.position);
	Object *collider = p_hit["collider"];
	const int shape_index = p_hit["shape"]; // which of the collider's shapes the ray hit
	hit.absorption = get_surface_absorption(collider, shape_index);
	return hit;
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
	return p_packet.intensity < cull_threshold
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

void PhononServer::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	geometry_dirty = true; // which bodies count as walls changed
}
uint32_t PhononServer::get_collision_mask() const { return collision_mask; }

void PhononServer::set_default_absorption(float p_absorption) {
	default_absorption = CLAMP(p_absorption, 0.0f, 1.0f);
	geometry_dirty = true; // baked into the segments
}
float PhononServer::get_default_absorption() const { return default_absorption; }

void PhononServer::set_air_absorption(float p_absorption) { air_absorption = MAX(p_absorption, 0.0f); } // negative would add energy
float PhononServer::get_air_absorption() const { return air_absorption; }

void PhononServer::set_use_custom_raycast(bool p_enabled) {
	use_custom_raycast = p_enabled;
	geometry_dirty = true; // walls may have changed while the Godot path was in use
}
bool PhononServer::is_using_custom_raycast() const { return use_custom_raycast; }

void PhononServer::set_geometry_cell_size(float p_size) {
	geometry_cell_size = MAX(p_size, 1.0f);
	geometry_dirty = true;
}
float PhononServer::get_geometry_cell_size() const { return geometry_cell_size; }

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
	d["raycaster"] = use_custom_raycast ? "custom" : "godot";
	d["dynamic_sync_ms"] = stats.dynamic_sync_ns / 1.0e6;
	d["dynamic_bodies_moved"] = stats.dynamic_bodies_moved;
	d["static_segments"] = static_cast<int64_t>(geometry.get_static_count());
	d["dynamic_segments"] = static_cast<int64_t>(geometry.get_dynamic_count());
	d["last_bake_ms"] = last_bake_ns / 1.0e6;
	d["bake_count"] = bake_count;
	return d;
}

void PhononServer::set_profiling_enabled(bool p_enabled) { profiling_enabled = p_enabled; }
bool PhononServer::is_profiling_enabled() const { return profiling_enabled; }
