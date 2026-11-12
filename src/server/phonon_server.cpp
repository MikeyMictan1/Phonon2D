#include "server/phonon_server.h"
#include "nodes/phonon_receiver.h"
#include "nodes/phonon_collision_shape_2d.h"
#include "resources/phonon_material.h"

#include <godot_cpp/classes/collision_object2d.hpp>
#include <godot_cpp/classes/physics_direct_space_state2d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <algorithm>

// Static members need exactly one definition in one .cpp
PhononServer *PhononServer::singleton = nullptr;

void PhononServer::_bind_methods() {
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

// ---- Simulation ----

void PhononServer::tick(double p_delta, PhysicsDirectSpaceState2D *p_space) {
	// How far every packet tries to move this tick (all packets share one speed: it's a property of the medium)
	const float step = speed_of_sound * static_cast<float>(p_delta);

	// One query object reused for every ray this tick. Creating one per packet would mean
	// thousands of allocations per frame.
	Ref<PhysicsRayQueryParameters2D> query;
	if (p_space != nullptr) {
		query.instantiate();
		query->set_collision_mask(collision_mask);
	}

	packet_pool.for_each_active([&](PhononPacket &packet) {
		// Cast a ray along the path the packet is about to take. Raycasting the whole step
		// (instead of checking the end point) stops fast packets tunnelling through thin walls.
		Dictionary hit;
		if (p_space != nullptr) {
			query->set_from(packet.position);
			query->set_to(packet.position + packet.direction * step);
			hit = p_space->intersect_ray(query);
		}

		if (hit.is_empty()) {
			advance_packet(packet, step);
		} else {
			bounce_packet(packet, hit);
		}

		if (should_cull(packet)) {
			packet_pool.recycle_packet(packet);
		}
	});
}

// No collision: move the full step
void PhononServer::advance_packet(PhononPacket &p_packet, float p_distance) {
	p_packet.position += p_packet.direction * p_distance;
	p_packet.distance_travelled += p_distance;
}

// Collision: move to the hit point, reflect off the surface, lose energy to the material.
void PhononServer::bounce_packet(PhononPacket &p_packet, const Dictionary &p_hit) {
	const Vector2 hit_position = p_hit["position"];
	const Vector2 normal = p_hit["normal"]; // Normal of the wall/surface collided with
	Object *collider = p_hit["collider"];
	const int shape_index = p_hit["shape"]; // which of the collider's shapes the ray hit

	p_packet.distance_travelled += p_packet.position.distance_to(hit_position);
	p_packet.position = hit_position + normal * BOUNCE_OFFSET; // nudge off the wall
	p_packet.direction = p_packet.direction.bounce(normal);    // d' = d - 2(d.n)n
	p_packet.intensity *= 1.0f - get_surface_absorption(collider, shape_index);
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

void PhononServer::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }
uint32_t PhononServer::get_collision_mask() const { return collision_mask; }

void PhononServer::set_default_absorption(float p_absorption) { default_absorption = CLAMP(p_absorption, 0.0f, 1.0f); }
float PhononServer::get_default_absorption() const { return default_absorption; }

// ---- Debug ----

int PhononServer::get_active_packet_count() const {
	return static_cast<int>(packet_pool.get_active_count());
}

void PhononServer::clear_packets() {
	packet_pool.clear_pool();
}
