#pragma once

#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>

// Sound is split into frequency bands: each packet carries an energy value per band.
// 0 = low (bass), 1 = mid (voice), 2 = high (hiss)
static constexpr int BAND_COUNT = 3;

// A single phonon (one discrete packet of sound energy)
struct PhononPacket {
    godot::Vector2 position = godot::Vector2(0, 0);
    godot::Vector2 direction = godot::Vector2(0, 0);
    float energy[BAND_COUNT] = {0.0f, 0.0f, 0.0f}; // Fraction of each band still surviving. Starts as the emitter's band mix (sums to 1)
    float distance_travelled = 0.0f;
    int bounce_count = 0;
    bool active = false;

    // Per-emission data: identical for every packet of one emit_phonons() call, copied in at spawn and never changed.
    // Kept on the packet for simplicity. See performance.md (emission table) for moving it out if the tick loop needs to be leaner.
    uint32_t emission_id = 0;    // which sound this belongs to ("sound #42"), so receivers can group hits. 0 = none
    float weight = 0.0f;         // how much of the emitter's loudness this packet represents (loudness / packet_count)
    float pitch = 0.0f;          // Hz, gameplay label for sensor conditions, not used by the physics
    godot::StringName tag;       // emitter's tag, e.g. "footstep"
    godot::ObjectID source;      // the emitter node (an ID, not a pointer: the node may be freed while packets fly)

    // Sum of all band energies
    float total_energy() const {
        float total = 0.0f;
        for (int b = 0; b < BAND_COUNT; b++) {
            total += energy[b];
        }
        return total;
    }
};
