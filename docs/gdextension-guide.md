# GDExtension cheat sheet (Phonon2D)

Everything you need day to day, using this project's setup. Build steps are in `running.md`.

## 1. What GDExtension is

- You write C++ classes against **godot-cpp** (C++ wrappers for Godot's API).
- They compile into a DLL (`Phonon2D.windows.template_debug.x86_64.dll`).
- Godot loads the DLL at startup using `project/bin/phonon2d.gdextension`, which says where the DLL is and what the entry function is called.
- Once loaded, your classes act like built-in ones: they show up in Create New Node, the Inspector, and GDScript.
- No engine recompile needed. You only rebuild your DLL.

## 2. How Godot finds your classes (the load chain)

```
Godot starts
  -> reads project/bin/phonon2d.gdextension
  -> loads the DLL, calls phonon2d_library_init()        (register_types.cpp)
  -> calls initialize_gdextension_types()
       -> GDREGISTER_CLASS(X) for each class
            -> X::_bind_methods() runs once
```

If any link is missing, the class won't show up.

## 3. Anatomy of a class

```cpp
// phonon_thing.h
#pragma once
#include <godot_cpp/classes/node2d.hpp>   // header for the parent class

using namespace godot;

class PhononThing : public Node2D {
	GDCLASS(PhononThing, Node2D)          // (this class, parent). Required.

	float speed = 100.0f;                 // members: plain C++

protected:
	static void _bind_methods();          // required, even if empty

public:
	void set_speed(float p_speed);
	float get_speed() const;

	void _ready() override;               // engine callbacks
	void _process(double delta) override;
};
```

```cpp
// phonon_thing.cpp
#include "nodes/phonon_thing.h"
#include <godot_cpp/core/class_db.hpp>

void PhononThing::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &PhononThing::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &PhononThing::get_speed);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed"), "set_speed", "get_speed");
}
```

Then in `register_types.cpp`: `#include "nodes/phonon_thing.h"` and `GDREGISTER_CLASS(PhononThing);`.

Header names are the class name in snake_case: `AudioStreamPlayer2D` is `<godot_cpp/classes/audio_stream_player2d.hpp>`. Browse `godot-cpp/gen/include/godot_cpp/classes/` if unsure (it exists after the first build).

## 4. Adding a new class: checklist

1. `.h` + `.cpp` in the right folder (`server/`, `nodes/`, `resources/`)
2. `GDCLASS(Name, Parent)` + `static void _bind_methods();`
3. Define `_bind_methods()` in the `.cpp` (empty is fine)
4. `#include` + `GDREGISTER_CLASS(Name);` in `register_types.cpp`
5. Add both files to `CMakeLists.txt` (SCons finds them automatically)
6. Rebuild, reload Godot

Plain C++ classes Godot never needs to see (like `PhononPacket`): skip steps 2 to 4.

## 5. Registration macros (register_types.cpp)

| Macro | Use |
|---|---|
| `GDREGISTER_CLASS(X)` | Normal class, can be added to scenes / `.new()` |
| `GDREGISTER_ABSTRACT_CLASS(X)` | Base type only, can't be instantiated (`PhononReceiver`) |
| `GDREGISTER_VIRTUAL_CLASS(X)` | Base meant to be extended in GDScript |
| `GDREGISTER_RUNTIME_CLASS(X)` | Like CLASS, but its callbacks don't run in the editor |

Register parents before children.

## 6. `_bind_methods()`: exposing things to Godot

Only what's bound here is visible to the editor and GDScript. C++ can still use everything.

| You want | Write |
|---|---|
| Method callable from GDScript | `ClassDB::bind_method(D_METHOD("name", "arg1"), &X::name);` |
| Static method | `ClassDB::bind_static_method("X", D_METHOD("name"), &X::name);` |
| Inspector property | bind setter + getter, then `ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed"), "set_speed", "get_speed");` |
| Property with slider | `PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "0,1000,1")` |
| Enum dropdown property | `PropertyInfo(Variant::INT, "type", PROPERTY_HINT_ENUM, "None,Foam,Velvet")` |
| Resource slot in Inspector | `PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "PhononMaterial")` |
| Group properties in Inspector | `ADD_GROUP("Emission", "emission_");` before the properties |
| Signal | `ADD_SIGNAL(MethodInfo("sound_detected", PropertyInfo(Variant::FLOAT, "intensity")));` |
| Enum values | `BIND_ENUM_CONSTANT(FOAM);` + `VARIANT_ENUM_CAST(X::MyEnum);` after the class in the `.h` |

Overrides like `_ready`, `_process`, `_draw` are **not** bound. Godot calls them because you override them.

Common `Variant` types: `BOOL`, `INT`, `FLOAT`, `STRING`, `VECTOR2`, `COLOR`, `OBJECT`, `ARRAY`.

## 7. Engine callbacks you'll use

| Callback | When |
|---|---|
| `_ready()` | Node entered the tree, children ready |
| `_process(double delta)` | Every frame. Use for drawing / visuals |
| `_physics_process(double delta)` | Fixed tick (60/s default). Use for simulation |
| `_draw()` | When redrawn. Trigger with `queue_redraw()` |
| `_notification(int what)` | Catch-all (enter/exit tree etc.), not virtual: declare `void _notification(int p_what);` |

**Editor gotcha:** these run in the editor too. Guard simulation:

```cpp
#include <godot_cpp/classes/engine.hpp>
if (Engine::get_singleton()->is_editor_hint()) return;
```

## 8. Common tasks in C++

```cpp
// signals
emit_signal("sound_detected", 0.8f);

// print to Godot's Output panel
#include <godot_cpp/variant/utility_functions.hpp>
UtilityFunctions::print("intensity: ", intensity);

// errors that show in the Debugger without crashing
ERR_FAIL_NULL(ptr);
ERR_FAIL_COND_MSG(count < 0, "count must be >= 0");

// nodes
Node *n = get_node<Node>(NodePath("Child"));
Node2D *n2 = Object::cast_to<Node2D>(n);     // safe cast, nullptr if wrong type

// drawing (inside _draw)
draw_circle(Vector2(), 4.0f, Color(1, 1, 1));
draw_line(a, b, Color(1, 0, 0), 2.0f);

// math
#include <godot_cpp/core/math.hpp>
Math::sin(x); Math::deg_to_rad(90.0);
Vector2 dir = Vector2(1, 0).rotated(angle);
```

## 9. Memory: who owns what

| Kind | How to create | How it dies |
|---|---|---|
| `Node` subclasses | `memnew(X)` then `add_child()` | Freed with the tree, or `queue_free()` |
| `RefCounted` / `Resource` | `Ref<X> r; r.instantiate();` | Automatically when last `Ref` goes |
| Plain `Object` (e.g. a singleton) | `memnew(X)` | You call `memdelete(ptr)` |
| Plain C++ (`PhononPacket`) | normal C++ (`std::vector`, values) | normal C++ |

Use `memnew`/`memdelete` for Godot objects, never `new`/`delete`. Hold Resources in `Ref<>`, not raw pointers.

## 10. Godot types vs std types

Godot's API takes Godot types: `String`, `Array`, `Dictionary`, `TypedArray<T>`, `PackedVector2Array`, `Vector2`. Use them for anything bound or passed to the engine. For purely internal data (the packet pool) `std::vector` is fine and faster.

## 11. Singletons (for PhononServer)

Usual pattern:

```cpp
// phonon_server.h
static PhononServer *singleton;
static PhononServer *get_singleton() { return singleton; }

// register_types.cpp, on init (after GDREGISTER_CLASS)
server = memnew(PhononServer);
Engine::get_singleton()->register_singleton("PhononServer", server);

// on uninit
Engine::get_singleton()->unregister_singleton("PhononServer");
memdelete(server);
```

GDScript can then call `PhononServer.whatever()`. A plain `Object` doesn't get `_physics_process`, so something has to tick it (an emitter/node calling into it, or hooking the main loop). Decide when we implement it.

## 12. Documentation (optional, nice for the report)

XML files in `doc_classes/` show up as in-editor help (F1). Generate stubs from the project folder:

```powershell
godot --doctool .. --gdextension-docs
```

## 13. Gotchas

- Forgot `GDREGISTER_CLASS` = class missing in Godot, no error.
- Forgot to bind setter/getter = property won't show / `ADD_PROPERTY` errors at load.
- Name strings in `D_METHOD`/`ADD_PROPERTY` must match exactly; typos fail at load time, check the Output panel.
- Godot locks the DLL while open; close or reload before rebuilding.
- `_process` runs in the editor (see section 7).
- Changing a header recompiles every file that includes it; that's normal.
- Don't edit anything in `godot-cpp/`.

## 14. Where to look things up

- Godot class reference (same names/methods in C++): docs.godotengine.org/en/stable/classes/
- godot-cpp headers for exact C++ signatures: `godot-cpp/gen/include/godot_cpp/classes/`
- GDExtension C++ tutorial: docs.godotengine.org, "GDExtension C++ example"
- Real-world example code: the godot-cpp `test/` folder (read only)
