# Phonon2D Canon (source of truth)

Living record of the project for future AI sessions (and Mikey). If this file disagrees with other docs or old chat, this file wins. Update it after every piece of significant work, in the same session, before wrapping up.

**Rules for editing**
- Append new entries to the Log (newest at the bottom). Don't rewrite history; if something turns out wrong, add a correction entry and fix the Current State section.
- Every log entry has a date (YYYY-MM-DD, add HH:MM if known) and says what changed, why, and what is verified vs not.
- Keep "Current State" and "Decisions" accurate; they are the quick read.
- Record decisions with their reasoning. Mikey must be able to defend them in the viva.

## Related docs (all in `docs/`)

- `current-context.md`: onboarding brief carried over from the GDExtension sandbox (who Mikey is, design of the library, deliverable dates, setup problems already solved). Partly historical; sandbox framing is outdated.
- `running.md`: how to compile and run the project.
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

## Environment

- Windows 11, PowerShell. Repo: `C:\MyStuff\Projects\Phonon2D`, branch `GDExtension-Setup` (main is `main`).
- Godot 4.7 standard. MinGW-w64 g++ 16.2.0 (MSYS2 UCRT64, `C:\msys64\ucrt64\bin`, must be first on PATH). Python 3.12.10, SCons 4.11.1. No MSVC.
- Build: `scons use_mingw=yes` (see `running.md`).
- godot-cpp is a git submodule (branch 4.3); `SConstruct` passes `api_version: 4.3`.

## Current state

_Last updated: 2026-10-28_

- Repo contains the template scaffolding copied from the sandbox: `src/` (register_types, ExampleClass, PinkCircle), `project/` test project, `doc_classes/`, build config.
- `PinkCircle` (Node2D, exported `radius`, pulsing draw) is the only custom node. Verified 2026-10-28: `scons use_mingw=yes` builds in this repo and PinkCircle works in Godot.
- Library still uses template placeholder names (`EXTENSION-NAME`, `example_library_init`, `example.gdextension`). Works as is.
- `ExampleClass` still present.
- No Phonon2D classes written yet.
- Initial project setup is being committed by Mikey (2026-10-28) as the baseline before cleanup.

## Decisions

- Material variation handled with data (coefficients, enum presets), not interface hierarchies. Marker interfaces cut. (Pre-existing, from design work.)
- PhononPacket stores direction, not velocity (speed is a medium property); wavelength derivable so not stored.
- Packets are pooled and recycled below a cull threshold.
- Listener and sensor share a `PhononReceiver` abstraction so the server dispatches to one list.
- Using MinGW rather than MSVC. Fallback if it becomes a time sink: VS Build Tools + plain `scons`.

## Known issues / gotchas

- g++ fails silently from PowerShell unless `C:\msys64\ucrt64\bin` is first on PATH (DLL shadowing; offending DLL never identified).
- First godot-cpp build in a new folder takes about 20 minutes; Windows Defender exclusion helps.
- GDExtension `_process` runs in the editor too; guard simulation with `Engine::get_singleton()->is_editor_hint()`.
- Godot locks the DLL while open; close it before rebuilding if the copy fails.

## Roadmap

1. ~~Verify build and PinkCircle in this repo.~~ Done 2026-10-28.
2. (next) Rename placeholders to phonon2d; remove ExampleClass.
3. Milestone: `PhononEmitter2D` spawning N points moving outward in a circle, drawn. No collision or audio.
4. Editor setup: `scons compiledb=yes`, gdb debug config.
5. Project Specification.

## Log

### 2026-10-28
- Sandbox work (earlier, dates not recorded): built the template with MinGW, solved PATH/DLL and slow-link problems, added `PinkCircle`, copied files to this repo. Details in `current-context.md`.
- Read all context docs, confirmed repo state (migration copied, rename pending). Wrote `running.md` with build/run steps. Renamed `context/` to `docs/` (Mikey). Established this canon as the source of truth and filled in the initial state above. Mikey named the Phonon2D AI "Echo" (see Codename section).
- Mikey ran the first build in this repo: it succeeded and PinkCircle runs in Godot. Committing this as the initial project setup baseline. Next: cleanup of template leftovers (rename placeholders, remove ExampleClass, check stale build output isn't tracked).
