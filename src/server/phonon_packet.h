#pragma once

#include <godot_cpp/variant/vector2.hpp>

// Sound is split into frequency bands: each packet carries an energy value per band.
// 0 = low (bass), 1 = mid (voice), 2 = high (hiss)
static constexpr int BAND_COUNT = 3;

// A single phonon (one discrete packet of sound energy)
struct PhononPacket {
    godot::Vector2 position = godot::Vector2(0, 0);
    godot::Vector2 direction = godot::Vector2(0, 0);
    float energy[BAND_COUNT] = {0.0f, 0.0f, 0.0f}; // Fraction of each band still surviving. Starts as the emitter's band mix (sums to 1)
    float weight = 0.0f; // How much of the emitters loudness this packet represents (loudness / packet_count emitted)
    float pitch = 0.0f; // Hz for sensor conditions like "only react to 880 Hz", not for physics
    float distance_travelled = 0.0f;
    int bounce_count = 0;
    bool active = false;

    // Sum of all band energies
    float total_energy() const {
        float total = 0.0f;
        for (int b = 0; b < BAND_COUNT; b++) {
            total += energy[b];
        }
        return total;
    }
};
