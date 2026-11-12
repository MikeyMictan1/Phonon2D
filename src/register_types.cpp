#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "nodes/phonon_collision_shape_2d.h"
#include "nodes/phonon_emitter_2d.h"
#include "nodes/phonon_listener_2d.h"
#include "nodes/phonon_receiver.h"
#include "nodes/phonon_sensor_2d.h"
#include "nodes/phonon_world_2d.h"
#include "resources/phonon_material.h"
#include "server/phonon_server.h"

using namespace godot;

// The one PhononServer instance, owned by the extension for its whole lifetime
static PhononServer *phonon_server = nullptr;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Abstract: GDScript can't call PhononServer.new(); only this file creates it
	GDREGISTER_ABSTRACT_CLASS(PhononServer);
	GDREGISTER_CLASS(PhononMaterial);
	GDREGISTER_CLASS(PhononEmitter2D);
	GDREGISTER_ABSTRACT_CLASS(PhononReceiver);
	GDREGISTER_CLASS(PhononListener2D);
	GDREGISTER_CLASS(PhononSensor2D);
	GDREGISTER_CLASS(PhononWorld2D);
	GDREGISTER_CLASS(PhononCollisionShape2D);

	// Create the server after its class is registered, then expose it to GDScript by name
	phonon_server = memnew(PhononServer);
	Engine::get_singleton()->register_singleton("PhononServer", phonon_server);
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Reverse order of creation: hide it from GDScript first, then free it
	Engine::get_singleton()->unregister_singleton("PhononServer");
	memdelete(phonon_server);
	phonon_server = nullptr;
}

extern "C"
{
	// Initialization
	GDExtensionBool GDE_EXPORT phonon2d_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}