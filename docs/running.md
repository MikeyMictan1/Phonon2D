# Running Phonon2D

How to compile the extension and see it in Godot. Windows 11, PowerShell, from the repo root.

## One-time setup (already done on this machine)

- MSYS2 with MinGW-w64 g++ (UCRT64) at `C:\msys64\ucrt64\bin`
- Python 3.12 + SCons (`pip install scons`)
- Godot 4.7 (standard build)
- godot-cpp submodule fetched: `git submodule update --init --recursive`

## 1. Open a terminal and fix PATH

g++ fails silently (exit code 1, no error text) if another DLL on PATH shadows MinGW's. Put MinGW first in every new terminal:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;" + $env:PATH
```

Check it works: `g++ --version` should print 16.x.

(To make this permanent, add `C:\msys64\ucrt64\bin` to the top of the System Path variable.)

## 2. Compile

```powershell
scons use_mingw=yes
```

What it does:
- builds the godot-cpp bindings into a static library (first time in a folder only; around 20 minutes, a lot faster with the folder excluded from Windows Defender)
- compiles everything in `src/*.cpp` into a DLL
- copies the DLL into `project/bin/windows/`

Success looks like `scons: done building targets`. Later builds only recompile the files in `src/` you changed.

Useful variants:

```powershell
scons use_mingw=yes target=template_release   # release build instead of debug
scons use_mingw=yes compiledb=yes             # also writes compile_commands.json for editor autocomplete
scons -c                                      # clean build output
```

## 3. Open the project in Godot

1. Open Godot 4.7, click Import, pick `project/project.godot`.
2. If the project was already open when you rebuilt: Project > Reload Current Project. Godot locks the DLL while it is open, so if the build fails with a permission error, close Godot first, build, then reopen.
3. Check the extension loaded: there should be no red errors in the Output panel on startup.

## 4. Check the PinkCircle works

1. Open `project/pink_circle.tscn` (double click in the FileSystem dock).
2. The pink circle should be visible, pulsing in size. The `radius` property shows in the Inspector.
3. To add one yourself: Scene > add node (Ctrl+A), search `PinkCircle`.
4. Press F6 to run the open scene. The pink circle should appear in the game window.

If `PinkCircle` is missing from the Create New Node dialog, the library did not load. See troubleshooting.

## 5. The edit/build/test loop

1. Edit files in `src/`
2. If you add a new class: include its header and call `GDREGISTER_CLASS(YourClass);` in `src/register_types.cpp`
3. Close Godot (or rely on the reload step above), run `scons use_mingw=yes`
4. Reopen / reload the project, test

Note: `_process` in a GDExtension node also runs inside the editor. Guard simulation code with `Engine::get_singleton()->is_editor_hint()`.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Every file fails instantly, empty `=====` blocks in SCons output | PATH problem, redo step 1 |
| SCons looks for MSVC / cl.exe | Forgot `use_mingw=yes` |
| `Permission denied` writing the DLL | Godot has it open. Close Godot, rebuild |
| Extension not loading, `Can't open dynamic library` | DLL name doesn't match the paths in `project/bin/phonon2d.gdextension`. The DLL should be `project/bin/windows/Phonon2D.windows.template_debug.x86_64.dll`, matching `project/bin/phonon2d.gdextension` |
| `godot-cpp is not available` | Run `git submodule update --init --recursive` |
| Class missing after editing code | Forgot `GDREGISTER_CLASS` in `register_types.cpp`, or did not rebuild/reload |

## Notes

- Library name is `Phonon2D`, entry symbol `phonon2d_library_init`, extension file `project/bin/phonon2d.gdextension`.
- `SConstruct` passes `api_version: 4.3` while the project is on Godot 4.7. It builds fine, so leave it unless something breaks.
- Compiled output (`.dll`, `.a`, `.o`) is gitignored. Never commit it.
- If a build is interrupted during "Linking Static Library", delete the stray temp file in `godot-cpp/bin/` (random 8-char name, 0 bytes) and rerun.
