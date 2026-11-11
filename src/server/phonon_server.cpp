#include "server/phonon_server.h"
#include "nodes/phonon_receiver.h"

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

void PhononServer::tick(double p_delta) {
	// TODO: loop over active packets once you've decided how the server reaches them
	// (pool method / friend / pool runs the update). For each active packet:
	//   advance_packet(packet, dt);
	//   if (should_cull(packet)) packet_pool.recycle_packet(packet);
	(void)p_delta; // silences the unused-parameter warning until then
}

void PhononServer::advance_packet(PhononPacket &p_packet, float p_delta) {
	const float step = speed_of_sound * p_delta;
	p_packet.position += p_packet.direction * step;
	p_packet.distance_travelled += step;
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
