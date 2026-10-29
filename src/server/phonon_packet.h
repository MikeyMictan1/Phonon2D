#pragma once

#include <godot_cpp/variant/vector2.hpp>

// A single phonon (one discrete packet of sound energy)
struct PhononPacket {
    godot::Vector2 position = godot::Vector2(0, 0);
    godot::Vector2 directio = godot::Vector2(0, 0);
    float intensity = 0.0f;
    float frequency = 0.0f;
    float distance_travelled = 0.0f;
    int bounce_count = 0;
    bool active = false;
};