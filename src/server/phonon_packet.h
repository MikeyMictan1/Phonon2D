#pragma once

#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>

// A single phonon (one discrete packet of sound energy)
struct PhononPacket {
    godot::Vector2 position = godot::Vector2(0, 0);
    godot::Vector2 direction = godot::Vector2(0, 0);
    float energy = 0.0f; // How loud the packet still is. Starts at the emitter's loudness; walls and air reduce it; culled below cull_threshold
    float distance_travelled = 0.0f;
    int bounce_count = 0;
    bool active = false;

    // Per-emission data: identical for every packet of one emitter play() call, copied in at spawn and never changed.
    // Kept on the packet for simplicity. See performance.md (emission table) for moving it out if the tick loop needs to be leaner.
    uint32_t emission_id = 0;    // which sound this belongs to ("sound #42"), so receivers can group hits. 0 = none
    float share = 0.0f;          // 1 / packet_count: the part of the wavefront this packet stands for. A hit delivers energy * share
    float frequency = 0.0f;      // Hz, gameplay label for sensor conditions, not used by the physics (yet)
    godot::StringName tag;       // emitter's tag, e.g. "footstep"
    godot::ObjectID source;      // the emitter node (an ID, not a pointer: the node may be freed while packets fly)
};
