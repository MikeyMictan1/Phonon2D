# Phonon2D Canon (source of truth)

Living record of the project for future AI sessions (and Mikey). If this file disagrees with other docs or old chat, this file wins. Update it after every piece of significant work, in the same session, before wrapping up.

**Rules for editing**
- Append new entries to the Log (newest at the bottom). Don't rewrite history; if something turns out wrong, add a correction entry and fix the Current State section.
- Every log entry has a date (YYYY-MM-DD, add HH:MM if known) and says what changed, why, and what is verified vs not.
- Keep "Current State" and "Decisions" accurate; they are the quick read.
- Record decisions with their reasoning. Mikey must be able to defend them in the viva.

## Related docs (all in `docs/`)

- `current-context.md`: background brief (who Mikey is, full library design, deliverables, setup problems already solved, sandbox history). Read for depth; this canon wins on anything current.
- `running.md`: how to compile and run the project.
- `gdextension-guide.md`: GDExtension cheat sheet for Mikey (class anatomy, registration, binding, callbacks, memory, singletons, gotchas).
- `gd-extension-context.md`: stock godot-cpp-template README, reference only.

## Project summary

Phonon2D: C++ GDExtension for Godot 4.7 that simulates 2D sound propagation as discrete energy packets (phonons) reflecting, absorbing and scattering off materials, exposed as a gameplay mechanic. Final-year dissertation project, University of Warwick (Mikey, BSc Computer Science). Planned classes: PhononPacket, PhononEmitter2D, PhononListener2D, PhononSensor2D, PhononReceiver, PhononMaterial, PhononServer. See `current-context.md` for detail.

Deadlines (confirm against module page): Spec 16 Oct 2026, Progress Report 22 Jan 2027, Final Report Plan 13 Mar 2027, Project Mgmt Report and Final Report 9 Apr 2027, Viva 5 to 15 May 2027.

Academic integrity: Mikey must understand and defend all Phonon2D code and declare AI use per Warwick rules. Prefer explaining and guiding; setup/tooling help is fine to do directly.

## Codename: Echo

Mikey calls the Claude that works on Phonon2D **Echo**. It helps him keep this project's AI separate from other Claude work. If you are an AI starting a session in this repo, you are Echo: Mikey will address you by that name, and you can refer to yourself that way. Treat this canon as your persistent memory, since you won't remember earlier sessions otherwise.

## Working agreement

- Casual, short, plain writing. No em dashes, no filler.
- One step at a time; say what a command does before asking him to run it.
- Honest pushback on overengineering or things that won't survive a viva.
- Phonon2D code is assessed: explain and guide, build it with Mikey step by step rather than dumping large chunks. Tooling/setup can be done directly.
- Mikey uses GitHub Desktop for commits and PRs. Don't commit or push unless asked.
- Ask before starting long-running commands (a full godot-cpp rebuild is ~25 min).

## Git workflow

- `main` is the stable branch. One branch per milestone off `main` (e.g. `phonon-emitter`), merged by PR.
- CI (`.github/workflows/ci.yml`, from the template) builds on Linux, Windows, macOS, Android and Web on every push and PR. Build-only, no tests. Check the Actions tab after pushing; merge only when green. `make_build.yml` is a manual all-platform release build.
- Never edit or commit inside `godot-cpp/`. Exclude it from repo-wide find/replace.

## Environment

- Windows 11, PowerShell. Repo: `C:\MyStuff\Projects\Phonon2D`, default branch `main`.
- Godot 4.7 standard. MinGW-w64 g++ 16.2.0 (MSYS2 UCRT64, `C:\msys64\ucrt64\bin`, must be first on PATH). Python 3.12.10, SCons 4.11.1. No MSVC.
- Build: `scons use_mingw=yes` (see `running.md`).
- godot-cpp is a git submodule (branch 4.3); `SConstruct` passes `api_version: 4.3`.

## Current state

_Last updated: 2026-10-28_

**Setup complete, ready for development.** Setup PR from `GDExtension-Setup` into `main` opened 2026-10-28 (CI running at time of writing).

- Layout: `src/` (register_types, PinkCircle), `project/` Godot test project, `doc_classes/` (empty, `.gitkeep`), `docs/`, build config (SConstruct, CMakeLists, methods.py), `.github/workflows/`.
- Library `Phonon2D`, entry symbol `phonon2d_library_init`, extension file `project/bin/phonon2d.gdextension`. Built DLL: `project/bin/windows/Phonon2D.windows.template_debug.x86_64.dll`.
- `PinkCircle` (Node2D, exported `radius`, pulsing draw) is the only custom node: a placeholder from the sandbox proving registration, properties, `_process` and `_draw` work. Remove or replace once real nodes exist. `pink_circle.tscn` is the main scene.
- Build and PinkCircle verified in Godot 4.7. Incremental build ~16s.
- Build output (dll, a, o) is gitignored and untracked.
- Class skeletons exist (empty, compile, registered) on `development`, layout below. Attributes/methods not added yet.

```
src/
  register_types.*            registration
  pink_circle.*               setup placeholder, delete later
  server/   phonon_server (Object, singleton later), phonon_packet (plain struct), phonon_packet_factory (plain C++, owned by server)
  nodes/    phonon_emitter_2d (AudioStreamPlayer2D), phonon_receiver (abstract Node2D), phonon_listener_2d, phonon_sensor_2d
  resources/ phonon_material (Resource, MaterialType enum inside the class)
```
- SConstruct globs `src/*.cpp` + `src/*/*.cpp` (excluding generated `src/gen/`). CMakeLists lists files explicitly: add new files there too.

## Decisions

- Material variation handled with data (coefficients, enum presets), not interface hierarchies. Marker interfaces cut. (Pre-existing, from design work.)
- PhononPacket stores direction, not velocity (speed is a medium property); wavelength derivable so not stored.
- Packets are pooled and recycled below a cull threshold.
- Listener and sensor share a `PhononReceiver` abstraction so the server dispatches to one list.
- PhononPacket fields (Mikey, 2026-10-28): `position`, `direction` (Vector2, unit), `intensity`, `frequency`, `distance_travelled` (float), `bounce_count` (int), `active` (bool), all defaulted so `packet = PhononPacket();` resets on recycle. float not double: Vector2 is float (real_t), and packets are numerous so smaller is more cache-friendly. Wavelength not stored (derived from speed/frequency). No per-packet distance falloff: geometric spreading (1/r in 2D) emerges from packet density; packets lose energy only to air absorption and materials.
- PhononPacket and PhononPacketFactory are plain C++ (no GDCLASS): pooled internals never exposed to GDScript, so no Object overhead. (2026-10-28)
- PhononPacketFactory renamed to **PhononPacketPool** (2026-10-28): it never chooses between concrete types (the core of GoF Factory), its job is reuse, so it is an Object Pool (cite Nystrom, Game Programming Patterns). Diagram/report should say "Object Pool". Benefits: no runtime allocation, bounded memory (pool size = performance budget), stable pointers, cache-friendly iteration. Costs: memory held when idle, hard cap, forgotten recycle drains the pool, stale pointers after recycle.
- Pool design (2026-10-28): `std::vector<PhononPacket>` sized once in the constructor initializer list, never grown (growth would invalidate handed-out pointers). Size from `explicit PhononPacketPool(std::size_t p_pool_size = POOL_SIZE_DEFAULT)`, public `static constexpr std::size_t POOL_SIZE_DEFAULT = 4096` (memory is trivial, ~150 KB; real limit is per-tick CPU; tune with a peak-active debug counter). No stored pool_size (use `.size()`). `request_packet()` returns `PhononPacket*`, `nullptr` when full (pool-full policy: drop the new packet). Linear O(n) scan for a free slot for now; next-fit cursor or free-index stack if profiling shows it matters. `recycle_packet(PhononPacket&)` resets to defaults; `clear_pool()` deactivates all (must not `vector::clear()`). Packet death (cull threshold, max bounces, max range) is decided by the server.
- Open: how the server iterates active packets each tick without being able to resize the vector. Decide when writing the server tick.
- PhononReceiver is an abstract Node2D base (`GDREGISTER_ABSTRACT_CLASS`), not a pure C++ interface: GDCLASS doesn't mix well with multiple inheritance, and listener/sensor are scene nodes anyway. (2026-10-28)
- Material type enum lives inside PhononMaterial (`PhononMaterial.MaterialType` in GDScript), Godot convention. (2026-10-28)
- Source folders grouped by Godot role (server/nodes/resources), mirroring the engine's own layout. (2026-10-28)
- Using MinGW rather than MSVC. Fallback if it becomes a time sink: VS Build Tools + plain `scons`.

## Known issues / gotchas

- g++ fails silently from PowerShell unless `C:\msys64\ucrt64\bin` is first on PATH (DLL shadowing; offending DLL never identified).
- First godot-cpp build in a new folder takes about 20 minutes; Windows Defender exclusion helps.
- GDExtension `_process` runs in the editor too; guard simulation with `Engine::get_singleton()->is_editor_hint()`.
- Godot locks the DLL while open; close it before rebuilding if the copy fails.
- An interrupted godot-cpp archive step leaves a 0-byte temp file in `godot-cpp/bin/`; delete it and rerun.
- godot-cpp once fully rebuilt (~25 min) for no identified reason. If it happens again, investigate.
- Repo-wide find/replace once edited files inside `godot-cpp/test/` (submodule showed as modified). Reverted. Exclude `godot-cpp/`.

## Roadmap

1. ~~Verify build and PinkCircle in this repo.~~ Done 2026-10-28.
2. ~~Rename placeholders to phonon2d; remove ExampleClass.~~ Done and verified 2026-10-28.
3. Merge the setup PR once CI is green; branch `phonon-emitter` off `main`.
4. (next) Milestone: `PhononEmitter2D` spawning N points moving outward in a circle, drawn. No collision or audio. Guard simulation with `is_editor_hint()`.
5. Editor setup: `scons compiledb=yes`, gdb debug config.
6. Project Specification (deadline was 16 Oct 2026 per the dates given; confirm its status with Mikey).

## Log

### 2026-10-28
- Sandbox work (earlier, dates not recorded): built the template with MinGW, solved PATH/DLL and slow-link problems, added `PinkCircle`, copied files to this repo. Details in `current-context.md`.
- Read all context docs, confirmed repo state (migration copied, rename pending). Wrote `running.md` with build/run steps. Renamed `context/` to `docs/` (Mikey). Established this canon as the source of truth and filled in the initial state above. Mikey named the Phonon2D AI "Echo" (see Codename section).
- Mikey ran the first build in this repo: it succeeded and PinkCircle runs in Godot. Committing this as the initial project setup baseline. Next: cleanup of template leftovers (rename placeholders, remove ExampleClass, check stale build output isn't tracked).
- Cleanup pass (Echo): untracked compiled binaries (they had been committed despite .gitignore; added ignore rules for dll/a/so/dylib). Renamed library to `Phonon2D`, entry symbol to `phonon2d_library_init`, `.gdextension` to `phonon2d.gdextension` with all library paths updated. Removed ExampleClass (src, doc XML, example.gd/tscn); CMake source list now pink_circle. Set project name and main scene. First rebuild after the rename was interrupted at the godot-cpp archive step (stray 0-byte temp file in `godot-cpp/bin/`); rerun started. Verification of the rebuilt DLL in Godot pending.
- 17:36: rebuild finished (`Phonon2D.windows.template_debug.x86_64.dll`). It had recompiled all of godot-cpp (970 objects, ~25 min incl. archive) for reasons not identified; the previous build's objects were not reused. A follow-up no-change rebuild took 16s and reported godot-cpp up to date, so incremental builds work. If a full godot-cpp rebuild happens again unexpectedly, investigate (suspects: SCons signature cache, submodule state).
- Mikey verified in Godot: PinkCircle runs with the renamed `Phonon2D` DLL, no errors. Setup and cleanup complete; ready to start Phonon2D development (next: PhononEmitter2D milestone).
- Stray edits inside `godot-cpp/test/` (from a repo-wide find/replace of the entry symbol) found and reverted by Mikey. Committed "Build Ready" (`175cce3`), pushed, opened setup PR into `main`. Explained the template CI. Tidied all docs for a fresh session; next session starts on the PhononEmitter2D milestone.

### 2026-10-28 (skeletons)
- Mikey shared an architecture diagram (Factory / Mediator singleton / Observers / Material resource). Echo created empty class skeletons for it in `src/server`, `src/nodes`, `src/resources`, registered them, and updated SConstruct (subfolder glob, excludes `src/gen/` which otherwise linked twice) and CMakeLists. Builds clean with MinGW; not yet checked in the Godot editor.
- The diagram still shows marker interfaces `IPhononPacketFactory` and `IPhononMaterial`. Not created, per the existing "marker interfaces cut" decision. Flagged to Mikey.
- Next: prototype "phonons emitting from an emitter" (emitter -> server -> factory -> packets, drawn). Planning first, then implement with Mikey step by step.
- Mikey wrote `PhononPacket` (fields above, with defaults, `godot::Vector2`). Next: PhononPacketFactory (pool).
- Mikey designed and wrote the `PhononPacketPool` header (renamed from factory). Next: `.cpp` (`request_packet` linear scan, recycle, clear), then server tick.
