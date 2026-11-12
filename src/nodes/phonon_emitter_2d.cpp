#include "nodes/phonon_emitter_2d.h"
#include "server/phonon_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

void PhononEmitter2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("emit_phonons"), &PhononEmitter2D::emit_phonons);

	ClassDB::bind_method(D_METHOD("set_packet_count", "count"), &PhononEmitter2D::set_packet_count);
	ClassDB::bind_method(D_METHOD("get_packet_count"), &PhononEmitter2D::get_packet_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "packet_count", PROPERTY_HINT_RANGE, "1,4096,1"), "set_packet_count", "get_packet_count");

	ClassDB::bind_method(D_METHOD("set_intensity", "intensity"), &PhononEmitter2D::set_intensity);
	ClassDB::bind_method(D_METHOD("get_intensity"), &PhononEmitter2D::get_intensity);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "intensity", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_intensity", "get_intensity");

	ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &PhononEmitter2D::set_frequency);
	ClassDB::bind_method(D_METHOD("get_frequency"), &PhononEmitter2D::get_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency", PROPERTY_HINT_RANGE, "20,20000,1,suffix:Hz"), "set_frequency", "get_frequency");

	ClassDB::bind_method(D_METHOD("set_emit_on_ready", "enabled"), &PhononEmitter2D::set_emit_on_ready);
	ClassDB::bind_method(D_METHOD("get_emit_on_ready"), &PhononEmitter2D::get_emit_on_ready);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "emit_on_ready"), "set_emit_on_ready", "get_emit_on_ready");
}

void PhononEmitter2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return; // don't spawn packets while editing the scene
	}
	if (emit_on_ready) {
		emit_phonons();
	}
}

void PhononEmitter2D::emit_phonons() {
	PhononServer *server = PhononServer::get_singleton();
	ERR_FAIL_NULL_MSG(server, "PhononServer singleton missing; is the extension loaded?");

	const Vector2 origin = get_global_position();
	const float angle_step = Math_TAU / static_cast<float>(packet_count);

	for (int i = 0; i < packet_count; i++) {
		const float angle = angle_step * static_cast<float>(i);
		const Vector2 direction(Math::cos(angle), Math::sin(angle)); // already unit length

		if (server->spawn_packet(origin, direction, intensity, frequency) == nullptr) {
			return; // pool full: the pool has already logged it, no point trying the rest
		}
	}
}

void PhononEmitter2D::set_packet_count(int p_count) { packet_count = CLAMP(p_count, 1, 4096); }
int PhononEmitter2D::get_packet_count() const { return packet_count; }

void PhononEmitter2D::set_intensity(float p_intensity) { intensity = MAX(p_intensity, 0.0f); }
float PhononEmitter2D::get_intensity() const { return intensity; }

void PhononEmitter2D::set_frequency(float p_frequency) { frequency = MAX(p_frequency, 0.0f); }
float PhononEmitter2D::get_frequency() const { return frequency; }

void PhononEmitter2D::set_emit_on_ready(bool p_enabled) { emit_on_ready = p_enabled; }
bool PhononEmitter2D::get_emit_on_ready() const { return emit_on_ready; }
