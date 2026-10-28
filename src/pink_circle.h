#pragma once

#include <godot_cpp/classes/node2d.hpp>

using namespace godot;

class PinkCircle : public Node2D {
	GDCLASS(PinkCircle, Node2D)

	float radius = 50.0f;
	double time = 0.0;

protected:
	static void _bind_methods();

public:
	void set_radius(float p_radius);
	float get_radius() const;

	void _process(double delta) override;
	void _draw() override;
};
