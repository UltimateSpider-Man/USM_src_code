# OpenUSM build — `dist/` output

This directory contains a ready-to-deploy build of the OpenUSM project for
*Ultimate Spider-Man (2005)* PC.

## Files

| File           | What it is                                                         |
|----------------|--------------------------------------------------------------------|
| `USM.exe`      | The original game executable, with a custom icon injected into its `.rsrc` section. Imports and code unchanged. |
| `binkw32.dll`  | The OpenUSM DLL. Hooks `USM.exe` at runtime to install all patches and load mods. Re-exports every Bink function as a forwarder so video playback keeps working. |
| `mods/`        | Drop-in mod folder. Anything placed here can override game assets (textures, meshes, animations, audio, entities). |

## Installing into your game directory

1. Copy `USM.exe` and `binkw32.dll` into your *Ultimate Spider-Man* install
   directory, replacing the originals.
2. **Important — preserve Bink video playback:** rename the original
   `binkw32.dll` (the one shipped with the game) to **`binkw32_.dll`** before
   replacing it. Our `binkw32.dll` re-exports every Bink function as a
   forwarder pointing at `binkw32_.dll`, so the actual video decoder still
   gets called for intros and cutscenes.

   If you skip this rename, intros simply won't play — the game will still
   start and run.

3. Copy the `mods/` folder next to `USM.exe`. Drop assets into it using paths
   like:
   - `mods/data/...` — direct overrides (engine resolves filename-keyed)
   - `mods/characters/<name>.fbx` / `.dds` / `.tga` — typed-resource overrides

   See `enumerate_mods()` in `main.cpp` for the full key resolution rules.

## Command-line flags supported by the DLL

`USM.exe` itself takes no flags, but our DLL parses these from the command line
in `DllMain`:

| Flag                              | Effect                                                 |
|-----------------------------------|--------------------------------------------------------|
| `-console`                        | Allocates a console window for `sp_log()` output.      |
| `-windowed`                       | Forces windowed mode.                                  |
| `-noloadscreen`                   | Skips load screen.                                     |
| `-enablefpuexceptionhandling`     | Toggles FPU exception handling (off by default here).  |

Then the DLL reads `debug_menu.ini` from the working directory if present,
and feeds it through `os_developer_options::instance` for runtime tuning.

## Verifying the icon

After installing, right-click `USM.exe` in Explorer or look at the title-bar /
taskbar icon when running — you should see the round red "USM" mark instead of
the original game icon.

To verify programmatically:

```bash
i686-w64-mingw32-objdump -p USM.exe | grep -i "Resource Directory" -A 3
```
