#pragma once

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

using namespace godot;

/**
 * Scene-side adapter for PhononServer. Put exactly one in any scene that uses phonons.
 * Each physics frame it ticks the server (handing it this scene's physics space to raycast
 * against), then draws the active packets as dots with short trails.
 * The server never calls this node; all calls go node -> server.
 */
class PhononWorld2D : public Node2D {
	GDCLASS(PhononWorld2D, Node2D)

public:
	// Group every PhononWorld2D joins, so the editor can warn when a scene has more than one
	static constexpr const char *GROUP_NAME = "phonon_world_2d";

	void _enter_tree() override;
	void _exit_tree() override;
	void _physics_process(double p_delta) override;
	void _draw() override;
	PackedStringArray _get_configuration_warnings() const override;

	// Drawing settings (exported to the inspector)
	void set_debug_draw(bool p_enabled);
	bool get_debug_draw() const;

	void set_packet_color(const Color &p_color);
	Color get_packet_color() const;

	void set_dot_radius(float p_radius);
	float get_dot_radius() const;

	void set_trail_length(float p_length);
	float get_trail_length() const;

	void set_min_alpha(float p_alpha);
	float get_min_alpha() const;

protected:
	static void _bind_methods();

private:
	bool debug_draw = true;
	Color packet_color = Color(0.11f, 0.62f, 0.46f); // teal
	float dot_radius = 2.5f;
	float trail_length = 14.0f;
	float min_alpha = 0.15f; // weakest packets still stay faintly visible
};
