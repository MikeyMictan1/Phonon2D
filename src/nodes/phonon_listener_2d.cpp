#include "nodes/phonon_listener_2d.h"
#include "nodes/phonon_emitter_2d.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>

#include <cmath>

PhononListener2D *PhononListener2D::active = nullptr;

void PhononListener2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_playing_voice_count"), &PhononListener2D::get_playing_voice_count);

	ClassDB::bind_method(D_METHOD("set_reference_distance", "distance"), &PhononListener2D::set_reference_distance);
	ClassDB::bind_method(D_METHOD("get_reference_distance"), &PhononListener2D::get_reference_distance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reference_distance", PROPERTY_HINT_RANGE, "1,2000,1,or_greater,suffix:px"), "set_reference_distance", "get_reference_distance");

	ClassDB::bind_method(D_METHOD("set_volume_db", "db"), &PhononListener2D::set_volume_db);
	ClassDB::bind_method(D_METHOD("get_volume_db"), &PhononListener2D::get_volume_db);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.1,suffix:dB"), "set_volume_db", "get_volume_db");

	ClassDB::bind_method(D_METHOD("set_max_voices", "count"), &PhononListener2D::set_max_voices);
	ClassDB::bind_method(D_METHOD("get_max_voices"), &PhononListener2D::get_max_voices);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_voices", PROPERTY_HINT_RANGE, "1,256,1"), "set_max_voices", "get_max_voices");
}

// ---- Lifecycle ----

void PhononListener2D::_enter_tree() {
	PhononReceiver::_enter_tree();
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (active != nullptr && active != this) {
		WARN_PRINT("A PhononListener2D is already active; this one stays silent. Use one listener per scene.");
		return;
	}
	active = this;
}

void PhononListener2D::_exit_tree() {
	if (active == this) {
		active = nullptr;
	}
	stop_all_voices();
	arrivals.clear();
	PhononReceiver::_exit_tree();
}

// Voices are created here, not in the constructor: they're child nodes, so they need the node to be in the tree
void PhononListener2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) {
		return; // nothing plays in the editor
	}
	rebuild_voices();
}

// ---- Called by the server ----

// Runs once per packet that enters the circle, so it only tallies. Starting players happens once per tick in end_tick.
void PhononListener2D::add_hit(const PhononHit &p_hit) {
	PhononReceiver::add_hit(p_hit);

	for (Arrival &arrival : arrivals) { // a handful of emissions per tick at most: linear search is fine
		if (arrival.emission_id == p_hit.emission_id) {
			arrival.energy += p_hit.energy;
			return;
		}
	}
	arrivals.push_back({ p_hit.emission_id, p_hit.source, p_hit.energy });
}

void PhononListener2D::end_tick(float p_now) {
	PhononReceiver::end_tick(p_now);

	if (active != this || voices.empty()) {
		arrivals.clear(); // inactive listener: hear nothing
		return;
	}

	// Free voices whose clip has ended
	for (Voice &voice : voices) {
		if (voice.emission_id != 0 && !voice.player->is_playing()) {
			release_voice(voice);
		}
	}

	for (const Arrival &arrival : arrivals) {
		Voice *voice = find_voice(arrival.emission_id);
		bool start = false;

		if (voice == nullptr) {
			if (is_finished(arrival.emission_id)) {
				continue; // clip already played out: a late bounce doesn't replay it
			}
			// First packet of this sound: look up what the emitter plays. Gone (freed) or no stream set -> silent.
			const Ref<AudioStream> stream = get_source_stream(arrival.source);
			if (stream.is_null()) {
				continue;
			}
			voice = claim_voice();
			voice->emission_id = arrival.emission_id;
			voice->energy = 0.0f;
			voice->player->set_stream(stream);
			start = true;
		}

		voice->energy += arrival.energy;
		voice->player->set_volume_db(energy_to_db(voice->energy)); // set before play() so the first frame isn't at the old volume
		if (start) {
			voice->player->play();
		}
	}
	arrivals.clear();
}

// ---- Queries ----

int PhononListener2D::get_playing_voice_count() const {
	int count = 0;
	for (const Voice &voice : voices) {
		if (voice.emission_id != 0) {
			count++;
		}
	}
	return count;
}

// ---- Voices ----

void PhononListener2D::rebuild_voices() {
	for (Voice &voice : voices) {
		if (voice.player != nullptr) {
			voice.player->queue_free();
		}
	}
	voices.assign(static_cast<std::size_t>(max_voices), Voice());
	for (Voice &voice : voices) {
		voice.player = memnew(AudioStreamPlayer);
		add_child(voice.player, false, INTERNAL_MODE_BACK); // internal: hidden from the scene dock and never saved
	}
}

void PhononListener2D::stop_all_voices() {
	for (Voice &voice : voices) {
		if (voice.emission_id != 0) {
			voice.player->stop();
			release_voice(voice);
		}
	}
}

PhononListener2D::Voice *PhononListener2D::find_voice(uint32_t p_emission_id) {
	for (Voice &voice : voices) {
		if (voice.emission_id == p_emission_id) {
			return &voice;
		}
	}
	return nullptr;
}

PhononListener2D::Voice *PhononListener2D::claim_voice() {
	Voice *quietest = &voices[0];
	for (Voice &voice : voices) {
		if (voice.emission_id == 0) {
			return &voice;
		}
		if (voice.energy < quietest->energy) {
			quietest = &voice;
		}
	}
	// All busy: cut off the quietest sound
	quietest->player->stop();
	release_voice(*quietest);
	return quietest;
}

void PhononListener2D::release_voice(Voice &p_voice) {
	finished_ids[finished_next] = p_voice.emission_id;
	finished_next = (finished_next + 1) % finished_ids.size();
	p_voice.emission_id = 0;
	p_voice.energy = 0.0f;
}

bool PhononListener2D::is_finished(uint32_t p_emission_id) const {
	for (uint32_t id : finished_ids) {
		if (id == p_emission_id) {
			return true;
		}
	}
	return false;
}

// Volume from delivered energy, calibrated so a loudness-1 sound heard reference_distance away plays at volume_db.
// A ring of N packets at distance d is spread over a circumference of 2*pi*d, so a circle of diameter D catches about
// D / (2*pi*d) of them, each carrying loudness / N. Total energy ~ loudness * D / (2*pi*d). Divided by D (to be
// size-independent, like get_window_energy) that is loudness / (2*pi*d). Multiplying by 2*pi*reference_distance
// makes it 1 (0 dB) at the reference distance. 10*log10 because energy is a power, not an amplitude.
// Result: -3 dB per doubling of distance (2D spreading), minus whatever walls and air absorbed.
float PhononListener2D::energy_to_db(float p_energy) const {
	const float level = (p_energy / get_diameter()) * static_cast<float>(Math_TAU) * reference_distance;
	if (level <= 0.0f) {
		return -80.0f;
	}
	return CLAMP(10.0f * std::log10(level) + volume_db, -80.0f, 24.0f);
}

Ref<AudioStream> PhononListener2D::get_source_stream(ObjectID p_source) {
	const PhononEmitter2D *emitter = Object::cast_to<PhononEmitter2D>(ObjectDB::get_instance(p_source));
	if (emitter == nullptr) {
		return Ref<AudioStream>();
	}
	return emitter->get_stream();
}

// ---- Config ----

void PhononListener2D::set_reference_distance(float p_distance) { reference_distance = MAX(p_distance, 1.0f); }
float PhononListener2D::get_reference_distance() const { return reference_distance; }

void PhononListener2D::set_volume_db(float p_db) { volume_db = p_db; }
float PhononListener2D::get_volume_db() const { return volume_db; }

void PhononListener2D::set_max_voices(int p_count) {
	max_voices = CLAMP(p_count, 1, 256);
	if (is_node_ready() && !Engine::get_singleton()->is_editor_hint()) {
		stop_all_voices();
		rebuild_voices();
	}
}
int PhononListener2D::get_max_voices() const { return max_voices; }
