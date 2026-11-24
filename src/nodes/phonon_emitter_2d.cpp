#include "nodes/phonon_emitter_2d.h"
#include "server/phonon_server.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>

void PhononEmitter2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("play"), &PhononEmitter2D::play);

	ClassDB::bind_method(D_METHOD("set_stream", "stream"), &PhononEmitter2D::set_stream);
	ClassDB::bind_method(D_METHOD("get_stream"), &PhononEmitter2D::get_stream);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream", "get_stream");

	ClassDB::bind_method(D_METHOD("set_packet_count", "count"), &PhononEmitter2D::set_packet_count);
	ClassDB::bind_method(D_METHOD("get_packet_count"), &PhononEmitter2D::get_packet_count);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "packet_count", PROPERTY_HINT_RANGE, "1,4096,1"), "set_packet_count", "get_packet_count");

	ClassDB::bind_method(D_METHOD("set_loudness", "loudness"), &PhononEmitter2D::set_loudness);
	ClassDB::bind_method(D_METHOD("get_loudness"), &PhononEmitter2D::get_loudness);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loudness", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_loudness", "get_loudness");

	ClassDB::bind_method(D_METHOD("set_frequency", "frequency"), &PhononEmitter2D::set_frequency);
	ClassDB::bind_method(D_METHOD("get_frequency"), &PhononEmitter2D::get_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frequency", PROPERTY_HINT_RANGE, "20,20000,1,suffix:Hz"), "set_frequency", "get_frequency");

	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &PhononEmitter2D::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &PhononEmitter2D::get_tag);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");

	ClassDB::bind_method(D_METHOD("set_autoplay", "enabled"), &PhononEmitter2D::set_autoplay);
	ClassDB::bind_method(D_METHOD("get_autoplay"), &PhononEmitter2D::get_autoplay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "get_autoplay");
}

void PhononEmitter2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return; // don't spawn packets while editing the scene
	}
	if (autoplay) {
		play();
	}
}

void PhononEmitter2D::play() {
	PhononServer *server = PhononServer::get_singleton();
	ERR_FAIL_NULL_MSG(server, "PhononServer singleton missing; is the extension loaded?");

	// What every packet of this sound shares. Loudness is each packet's starting energy (so louder lives longer);
	// each packet stands for an equal share of the wavefront when it hits a receiver.
	PhononServer::EmissionInfo emission;
	emission.id = server->new_emission_id();
	emission.loudness = loudness;
	emission.share = 1.0f / static_cast<float>(packet_count);
	emission.frequency = frequency;
	emission.tag = tag;
	emission.source = ObjectID(get_instance_id());

	const Vector2 origin = get_global_position();
	const float angle_step = Math_TAU / static_cast<float>(packet_count);

	for (int i = 0; i < packet_count; i++) {
		const float angle = angle_step * static_cast<float>(i);
		const Vector2 direction(Math::cos(angle), Math::sin(angle)); // already unit length

		if (server->spawn_packet(origin, direction, emission) == nullptr) {
			return; // pool full: the pool has already logged it, no point trying the rest
		}
	}
}

void PhononEmitter2D::set_stream(const Ref<AudioStream> &p_stream) { stream = p_stream; }
Ref<AudioStream> PhononEmitter2D::get_stream() const { return stream; }

void PhononEmitter2D::set_packet_count(int p_count) { packet_count = CLAMP(p_count, 1, 4096); }
int PhononEmitter2D::get_packet_count() const { return packet_count; }

void PhononEmitter2D::set_loudness(float p_loudness) { loudness = MAX(p_loudness, 0.0f); }
float PhononEmitter2D::get_loudness() const { return loudness; }

void PhononEmitter2D::set_frequency(float p_frequency) { frequency = MAX(p_frequency, 0.0f); }
float PhononEmitter2D::get_frequency() const { return frequency; }

void PhononEmitter2D::set_tag(const StringName &p_tag) { tag = p_tag; }
StringName PhononEmitter2D::get_tag() const { return tag; }

void PhononEmitter2D::set_autoplay(bool p_enabled) { autoplay = p_enabled; }
bool PhononEmitter2D::get_autoplay() const { return autoplay; }
