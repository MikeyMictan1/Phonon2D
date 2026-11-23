#pragma once

#include "server/phonon_hit.h"
#include "server/phonon_hit_buffer.h"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

/**
 * Shared base for anything that receives phonons (listener, sensor), so the server dispatches to one list.
 * Registered abstract: can't be added to a scene on its own.
 *
 * A receiver is a circle. Each tick the server tests every packet's step against it and, when a packet
 * enters the circle, hands it a PhononHit. The receiver keeps recent hits in a ring buffer (the rolling
 * window). At the end of the tick the server calls end_tick(), which drops expired hits; subclasses
 * extend end_tick() to do their own work (sensors check conditions, the listener sets volumes).
 */
class PhononReceiver : public Node2D {
	GDCLASS(PhononReceiver, Node2D)

public:
	void _enter_tree() override;
	void _exit_tree() override;
	void _draw() override;

	// Called by the server -----------------------------------------------------------------------
	// A packet entered this receiver's circle this tick
	void add_hit(const PhononHit &p_hit);

	// After every packet has been dispatched for this tick. Base version drops hits older than the window.
	// Subclasses override it and call PhononReceiver::end_tick(p_now) first.
	virtual void end_tick(float p_now);

	// Queries (bound to GDScript) -----------------------------------------------------------------
	// Diameter: the receiver's cross-section facing the sound, used to normalise readings (rain gauge idea)
	float get_diameter() const;

	// Total energy of all hits in the window, divided by diameter: "how loud is it here" (size-independent)
	float get_window_energy() const;

	int get_hit_count() const;

	// Config (inspector properties) ----------------------------------------------------------------
	void set_radius(float p_radius);
	float get_radius() const;

	void set_window_length(float p_seconds);
	float get_window_length() const;

	void set_buffer_capacity(int p_capacity);
	int get_buffer_capacity() const;

	void set_debug_draw(bool p_enabled);
	bool get_debug_draw() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

protected:
	static void _bind_methods();

	PhononHitBuffer hits; // protected: sensor and listener read it directly

private:
	float radius = 32.0f;
	float window_length = 0.5f; // seconds of hits kept
	bool debug_draw = true;     // draw the circle outline (editor and game)
	Color debug_color = Color(0.85f, 0.35f, 0.19f, 0.8f); // coral
};
