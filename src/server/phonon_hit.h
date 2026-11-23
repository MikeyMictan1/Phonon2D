#pragma once

#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>

// One packet arriving at a receiver: a snapshot of what was delivered, taken by the server at the moment of entry.
// Receivers keep these in a PhononHitBuffer and read them back as a rolling window.
struct PhononHit {
    float time = 0.0f;                               // server sim time when it arrived (seconds)
    float energy = 0.0f;                             // actual energy delivered: packet.energy * packet.share (not yet divided by diameter)
    float frequency = 0.0f;                          // Hz, gameplay label from the emitter
    godot::Vector2 direction = godot::Vector2(0, 0); // packet's travel direction on arrival (negate it to point back at the sound)
    int bounces = 0;                                 // 0 = arrived directly from the emitter
    uint32_t emission_id = 0;                        // which emit() call it belongs to ("sound #42")
    godot::StringName tag;                           // emitter's tag (e.g. "footstep")
};
