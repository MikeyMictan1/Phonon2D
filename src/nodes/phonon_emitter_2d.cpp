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

	ClassDB::bind_method(D_METHOD("set_loudness", "loudness"), &PhononEmitter2D::set_loudness);
	ClassDB::bind_method(D_METHOD("get_loudness"), &PhononEmitter2D::get_loudness);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "loudness", PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"), "set_loudness", "get_loudness");

	// Band mix, grouped in the inspector under "Band" (properties named band_*)
	ADD_GROUP("Band", "band_");
	ClassDB::bind_method(D_METHOD("set_band_low", "value"), &PhononEmitter2D::set_band_low);
	ClassDB::bind_method(D_METHOD("get_band_low"), &PhononEmitter2D::get_band_low);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "band_low", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_band_low", "get_band_low");

	ClassDB::bind_method(D_METHOD("set_band_mid", "value"), &PhononEmitter2D::set_band_mid);
	ClassDB::bind_method(D_METHOD("get_band_mid"), &PhononEmitter2D::get_band_mid);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "band_mid", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_band_mid", "get_band_mid");

	ClassDB::bind_method(D_METHOD("set_band_high", "value"), &PhononEmitter2D::set_band_high);
	ClassDB::bind_method(D_METHOD("get_band_high"), &PhononEmitter2D::get_band_high);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "band_high", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_band_high", "get_band_high");
	ADD_GROUP("", ""); // end the group so later properties aren't put in it

	ClassDB::bind_method(D_METHOD("set_pitch", "pitch"), &PhononEmitter2D::set_pitch);
	ClassDB::bind_method(D_METHOD("get_pitch"), &PhononEmitter2D::get_pitch);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch", PROPERTY_HINT_RANGE, "20,20000,1,suffix:Hz"), "set_pitch", "get_pitch");

	ClassDB::bind_method(D_METHOD("set_tag", "tag"), &PhononEmitter2D::set_tag);
	ClassDB::bind_method(D_METHOD("get_tag"), &PhononEmitter2D::get_tag);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "tag"), "set_tag", "get_tag");

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

	// Normalise the band mix so it sums to 1: the mix only says how energy is spread, loudness says how much
	const float mix_total = band_low + band_mid + band_high;
	ERR_FAIL_COND_MSG(mix_total <= 0.0f, "PhononEmitter2D: band_low, band_mid and band_high are all 0, so there is no sound to emit.");
	const float band_mix[BAND_COUNT] = { band_low / mix_total, band_mid / mix_total, band_high / mix_total };

	// What every packet of this sound shares. Each packet carries an equal share of the loudness (weight).
	PhononServer::EmissionInfo emission;
	emission.id = server->new_emission_id();
	emission.weight = loudness / static_cast<float>(packet_count);
	emission.pitch = pitch;
	emission.tag = tag;
	emission.source = ObjectID(get_instance_id());

	const Vector2 origin = get_global_position();
	const float angle_step = Math_TAU / static_cast<float>(packet_count);

	for (int i = 0; i < packet_count; i++) {
		const float angle = angle_step * static_cast<float>(i);
		const Vector2 direction(Math::cos(angle), Math::sin(angle)); // already unit length

		if (server->spawn_packet(origin, direction, band_mix, emission) == nullptr) {
			return; // pool full: the pool has already logged it, no point trying the rest
		}
	}
}

void PhononEmitter2D::set_packet_count(int p_count) { packet_count = CLAMP(p_count, 1, 4096); }
int PhononEmitter2D::get_packet_count() const { return packet_count; }

void PhononEmitter2D::set_loudness(float p_loudness) { loudness = MAX(p_loudness, 0.0f); }
float PhononEmitter2D::get_loudness() const { return loudness; }

void PhononEmitter2D::set_band_low(float p_value) { band_low = CLAMP(p_value, 0.0f, 1.0f); }
float PhononEmitter2D::get_band_low() const { return band_low; }

void PhononEmitter2D::set_band_mid(float p_value) { band_mid = CLAMP(p_value, 0.0f, 1.0f); }
float PhononEmitter2D::get_band_mid() const { return band_mid; }

void PhononEmitter2D::set_band_high(float p_value) { band_high = CLAMP(p_value, 0.0f, 1.0f); }
float PhononEmitter2D::get_band_high() const { return band_high; }

void PhononEmitter2D::set_pitch(float p_pitch) { pitch = MAX(p_pitch, 0.0f); }
float PhononEmitter2D::get_pitch() const { return pitch; }

void PhononEmitter2D::set_tag(const StringName &p_tag) { tag = p_tag; }
StringName PhononEmitter2D::get_tag() const { return tag; }

void PhononEmitter2D::set_emit_on_ready(bool p_enabled) { emit_on_ready = p_enabled; }
bool PhononEmitter2D::get_emit_on_ready() const { return emit_on_ready; }
