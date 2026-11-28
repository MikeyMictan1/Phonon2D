#pragma once

#include "nodes/phonon_receiver.h"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_listener2d.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>

#include <array>
#include <cstdint>
#include <vector>

using namespace godot;

/**
 * Turns arriving phonons into the audio the player hears. One active per scene (the first one to enter the tree;
 * any other stays silent and warns).
 *
 * MVP playback, one voice per emission (one emitter play() call):
 *   - The first packet of an emission to enter the circle starts its emitter's `stream` from 0:00. That gives
 *     travel delay for free: the clip starts when the wavefront arrives.
 *   - The clip then plays to the end. Only the volume moves: it comes from the TOTAL energy that emission has
 *     delivered so far (not a rolling window, because the ring passes in a fraction of a second while the clip
 *     lasts much longer). So it rises as the rest of the ring and bounces arrive, and never goes down.
 *   - Once a voice finishes, later packets of that emission are ignored (no replay; echoes are future work).
 *
 * Direction: each voice also sums energy * arrival direction over its hits. Reversed, that is where the sound seems
 * to come from: the emitter in open space, the doorway when the direct path is blocked. Voices are AudioStreamPlayer2Ds
 * with Godot's distance fade off (our energy already sets loudness), placed each tick one screen-width from the
 * listener in that direction; the listener owns an AudioListener2D made current, so Godot pans relative to it.
 * Stereo only: Godot 2D panning is left/right, so a sound straight above or below sounds centred.
 */
class PhononListener2D : public PhononReceiver {
	GDCLASS(PhononListener2D, PhononReceiver)

public:
	void _ready() override;
	void _enter_tree() override;
	void _exit_tree() override;

	// Called by the server -----------------------------------------------------------------------
	void add_hit(const PhononHit &p_hit) override;
	void end_tick(float p_now) override;

	// Queries (bound to GDScript) -----------------------------------------------------------------
	int get_playing_voice_count() const; // debug: how many emissions are audible right now

	// Config (inspector properties) ----------------------------------------------------------------
	// Distance at which a loudness-1 sound plays at volume_db (0 dB by default). Closer is louder, further is quieter.
	void set_reference_distance(float p_distance);
	float get_reference_distance() const;

	// Overall volume trim for everything this listener plays
	void set_volume_db(float p_db);
	float get_volume_db() const;

	// How many sounds can play at once. When full, a new sound replaces the quietest one.
	void set_max_voices(int p_count);
	int get_max_voices() const;

	// How far sounds pan left/right: 0 = always centred, 1 = full. Same meaning as AudioStreamPlayer2D.panning_strength
	void set_panning_strength(float p_strength);
	float get_panning_strength() const;

protected:
	static void _bind_methods();

private:
	static PhononListener2D *active; // the listener that plays audio; null if none in the tree

	struct Voice {
		uint32_t emission_id = 0; // 0 = free
		float energy = 0.0f; // total energy delivered by this emission so far
		Vector2 direction_sum; // sum of energy * travel direction of its hits; reversed = where it seems to come from
		AudioStreamPlayer2D *player = nullptr; // internal child, owned by the scene tree
	};
	std::vector<Voice> voices;

	// Energy that arrived this tick, summed per emission. Filled by add_hit, consumed by end_tick.
	struct Arrival {
		uint32_t emission_id = 0;
		ObjectID source;
		float energy = 0.0f;
		Vector2 direction_sum;
	};
	std::vector<Arrival> arrivals;

	// Emissions whose voice already finished (or was stolen), so their late packets don't restart the clip
	std::array<uint32_t, 64> finished_ids{};
	std::size_t finished_next = 0;

	float reference_distance = 100.0f;
	float volume_db = 0.0f;
	int max_voices = 32;
	float panning_strength = 1.0f;

	AudioListener2D *audio_listener = nullptr; // internal child: Godot pans our voices relative to this

	void rebuild_voices();
	void place_voice(Voice &p_voice, float p_offset);
	void stop_all_voices();
	Voice *find_voice(uint32_t p_emission_id);
	Voice *claim_voice(); // free voice, or steals the quietest
	void release_voice(Voice &p_voice);
	bool is_finished(uint32_t p_emission_id) const;
	float energy_to_db(float p_energy) const;
	static Ref<AudioStream> get_source_stream(ObjectID p_source);
};
