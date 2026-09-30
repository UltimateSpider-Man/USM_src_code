# External OBJ / FBX / GLB meshes for USM_06_09_2026

This source update integrates external model import into the existing Ultimate Spider-Man PC mesh replacement path. It does not require Blender at game runtime. It is source code, not a compiled or in-game-validated loader.

## Start here

Place the model files under `extra` beside the game executable, and launch with the game directory as the working directory. Subdirectories are searched. The existing `mods` root remains supported.

A model named after an existing mesh resource can be selected directly:

```text
Ultimate Spider-Man/
  extra/
    ULTIMATE_SPIDERMAN.glb
    ULTIMATE_SPIDERMAN.glb.ini
```

For a differently named custom model, put a mapping beside it:

```text
Ultimate Spider-Man/
  extra/
    custom_hero/
      CustomHero.glb
      CustomHero.glb.ini
      mesh_swaps.ini
```

`mesh_swaps.ini`:

```ini
ULTIMATE_SPIDERMAN = CustomHero.glb
```

`CustomHero.glb.ini`, for an unrigged model or deliberate native weight transfer:

```ini
adapt=toolkit
anim=0
```

This preset requests a custom full-scene import, fits its bounding box to the current native donor on each axis, and transfers that donor's skin weights. It intentionally does not preserve the model's authored skin weights. Native gameplay animations remain in charge. Export a compatible rest pose: bounding-box fitting cannot repair arbitrary anatomy or a posed character by itself.

The filename sidecar is **`CustomHero.glb.ini`**, not `CustomHero.ini`. Restart the game after changing models, mappings, sidecars, or external texture/buffer dependencies; existing caches are not a live-reload service.

## Names and selection

There is no new character-name whitelist. `.obj`, `.fbx`, `.glb`, and `.gltf` extensions are recognized case-insensitively, and their source filenames can be arbitrary. `string_hash_dictionary.txt` explains known hashes but is not required for discovery or an admission list.

Mappings address an existing resource file or an embedded mesh object. For example:

```ini
VENOM = CustomVenom.fbx
VENOM000 = CustomBody.glb
```

The first is a file/family replacement; the second selects one embedded object. An exact object override takes priority over its containing file's override, without becoming the next sibling object's override. Object lookup uses the engine's numeric hash directly, including when its printable name is unavailable.

A numeric hash is also allowed, for example `0x12345678 = CustomProp.obj` or `extra/0x12345678.obj`. That number is only a syntax example: replace it with the actual target's engine hash. It is not a known game address or a universal target.

Right-hand source names in `mesh_swaps.ini` must be local basenames. Put each mapping file in the same folder as its model. Configured mappings win over same-name automatic discovery. Existing registered mod/root priority is retained; avoid multiple automatic candidates for the same target, since enumeration order is not a public conflict-resolution guarantee.

**A new filename does not create a new actor, entity, resource request, or script entry.** This is a replacement pipeline for meshes the game loads. It still needs a compatible native mesh/section and, for a skinned character, the target's native skeleton/donor information. Existing loose `.pcmesh` support is preserved; no new arbitrary-resource scaffold or entity-spawning system was added.

## Native PCMESH files

Both the final (`USM`) and release loaders discover native PC `.pcmesh` files
under `extra` and its subfolders, with case-insensitive extensions. For example,
`extra/characters/VENOM.PCMESH` is available by `venom`, `venom.pcmesh`, and its
relative path with either slash style. `extra/0x12345678.PCMESH` binds directly
to that resource hash; use the actual target hash.

Loose PCMESH files are explicit replacements for requested mesh resources,
including resources also present in PCPACK. They retain the original `mods`
before `extra` root priority. A same-stem FBX and native PCMESH may coexist as
different resource types. A new file alone does not create an actor or script.
Only pristine native `PCM ` version `0x601` images are accepted; invalid,
truncated, or already rebased images are skipped. The loader keeps original
bytes and supplies a private writable parse copy for each mesh load.

The registration regression check compiles each loader's actual recursive
traversal, PCMESH branch, and native image detector without starting the game:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File test/verify_extra_pcmesh_registration.ps1
```

## Adaptation controls

| Sidecar setting | Effect |
| --- | --- |
| `adapt=toolkit` | Sets `custom=1`, `skin=transfer`, `fit=1`, and `fit_axes=1`. |
| `adapt=uniform` | Chooses the pre-existing uniform fitting path. Does not force custom mode or change the skin choice. |
| `adapt=none` | Disables fitting; preserve exported dimensions and placement. |
| `adapt=auto` | Leaves the other settings/default decisions in effect. |
| `fit_axes=1` / `fit_axes=0` | Request per-axis fitting / legacy uniform fitting when fitting is enabled. |
| `up_axis=none` | Default: do not apply an extra rotation. |
| `up_axis=x`, `y`, or `z` | Rotate that source up axis to the game's Y axis. For a Z-up OBJ, use `up_axis=z`. |
| `up_axis=auto` | Optional density-based axis/direction estimate from the add-on; a heuristic, not a guarantee. |
| `flip_up=1` | Rotate upside-down after axis conversion. |
| `weld_normals=1` | Area-weighted normals across coincident positions within a source object; can smooth intentional hard edges. |
| `drop_outliers=1` | Opt in to dropping material groups with excessive bounds relative to the median. Default is to retain and log them. |
| `outlier_factor=8` | Threshold multiplier; must exceed 1. At least three material groups are needed. |

Per-axis fitting is automatic only for custom donor-skinned character imports when fitting is enabled. It is not forced onto a static prop, a native round-trip mesh, or a verified authored rig. Normals use the inverse transpose of the fitting scale. Flat source/target axes have a finite non-collapsing fallback rather than zero scale.

For a static prop with already-correct dimensions:

```ini
custom=1
fit=0
anim=0
```

For a model with an authored rig, omit `adapt=toolkit` to use the existing native/foreign-rig workflow. That workflow still requires the native metadata needed to verify its mappings. Supplying a source bone number does not prove it matches the target. Geometry-only rotations, per-axis edits, and pins are rejected when they would conflict with an active authored-rig mapping; use `skin=transfer` deliberately when discarding that rig is intended.

### Accessory pins

Pins select a source **object or material name**, case-insensitively through the importer's existing name normalization. All primitives of a GLB object retain that same source-object identity, so a pin can cover all of its materials.

Syntax:

```ini
pin.<name>=rigid,<target_bone>
pin.<name>=mix,<target_bone>,<strength>
pin.<name>=skirt,<center_bone>,<left_bone>,<right_bone>,<strength>
```

These lines are syntax descriptions, not directly usable settings. Substitute real names and bone indices from the **current native target**, not a different character. Strength is between 0 and 1. `rigid` uses one bone; `mix` blends that bone with transferred weights; `skirt` leaves the upper piece's transfer intact and distributes lower vertices between center and two lateral bones. Out-of-range indices or a selector matching no part are rejected. Later overlapping pins apply after earlier ones.

No Spider-Man-only 66-bone layout or fixed body offsets were copied into this path. Pins are not a physics cloth simulation, and this interface does not import Blender per-vertex pin masks.

## Format and material support

**FBX:** retains the source tree's native binary/ASCII FBX reader, material/texture helpers, skin transfer, native/foreign-rig handling, static declarations, and large-mesh batching.

Custom character materials use their local diffuse image, or the FBX diffuse
color when no image is available. Both paths clear the donor character's
sphere-map effects so inherited highlights do not wash out skin or clothing.
Native lighting, outlines, and blend settings are preserved.

**OBJ:** retains object/group/material selection, with added handling for missing normals, negative indices, tab-separated faces, inline face comments, and rejected malformed face tokens. Missing normals receive polygon normals. Triangulate concave polygons before export. OBJ does not supply skeletal weights; a character replacement uses native donor transfer.

Optional `.mtl` files now supply `Kd`, `d`/`Tr`, and simple `map_Kd` diffuse paths. Keep dependencies next to, or below, their referring file. Quoted texture paths with spaces are accepted. `map_Kd` options such as texture transforms are not implemented: bake them before export; an unsupported option is logged and that texture path is skipped. Native material rendering still determines whether opacity is actually blended.

**GLB/glTF:** a new native glTF 2.0 reader handles GLB JSON/BIN chunks, local or base64 `.gltf` buffers, node matrices/TRS, scene selection, indexed/non-indexed triangles, triangle strips/fans, interleaved/normalized/sparse accessors, UVs, default morph weights, and skinned default poses. PNG/JPEG diffuse images can be embedded or local. `baseColorFactor`, base-color texture coordinate selection, and `KHR_texture_transform` feed the existing native material path. `KHR_materials_unlit` is accepted, but no separate unlit game shader is introduced.

GLB materials remain distinct; they are not indiscriminately merged into the first native slot. Mirrored glTF nodes preserve inverse-transpose normal directions. Authored skin indices and weights pass through the existing verified mapping path, not straight into an unrelated native bone array.

Not implemented: Draco/meshopt compression, KTX2/Basis/WebP images, full glTF PBR shaders, glTF clip playback, animated morph targets, glTF cameras/lights, or new gameplay skeleton registration. Unsupported required extensions are rejected. Full source transparency, double-sided/PBR appearance, and animation equivalence are not guaranteed by diffuse import.

GLTF safeguards include a 256 MiB input/dependency limit, 32 MiB JSON limit, a charged decoded-data budget of 192 MiB, two million source vertices/triangles, 4,096 primitives, up to 1,024 source joints, and a 64-level node hierarchy. The charged budget is not a peak process-memory guarantee. Relative dependency paths are confined to their referring directory; absolute, remote, parent-traversal, and escaping symlink paths are rejected.

The original custom section builder still splits complete triangles into safe native draws, including its 26-bone per-draw palette limit. This is not a universal claim that arbitrarily large assets will fit the 32-bit game's memory or render correctly on every driver.

## Build

Use the existing project's 32-bit Windows/MinGW setup. From its source root in WSL/Linux:

```sh
cmake -S . -B build-extra -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF -DUSM_AUTO_DEPLOY=OFF
cmake --build build-extra --target USM -j 8
```

The actual targets declared by this uploaded CMake file are `USM`, `ultimate_release`, `ultimate_prerelease`, and `ultimate_build`. Their existing executable injection/packaging settings are unchanged. The source tree's older comments and README mention some historical `binkw32*` target names that are not the current `add_library` target names. Keep the correct loader/executable pairing for your installation, back up the installed game, and follow its existing deployment workflow. Do not enable `TARGET_XBOX` for the PC build.

No CMake dependency download or additional link library was introduced by this mesh patch. The new JSON reader uses RapidJSON already vendored under `assimp/contrib/rapidjson`. Existing project dependencies and existing CMake problems, if any, still apply.

## Verification and troubleshooting

See `VERIFICATION_EXTRA_MESHES.md` for the exact host checks, failures reproduced on the original source, and untested boundaries. **The Windows DLLs were not built and the game was not run in this environment.**

Enable the loader's existing console/log output and look for `[modmesh] mesh registered`, `configured swap`, `glTF 2.0`, `toolkit per-axis fit`, or a specific rejection message. Missing textures should first be checked for supported encoding, actual relative path, and correct UVs. Severe limb deformation should first be checked against the exported rest pose and the selected native target; per-axis fitting cannot infer missing rig semantics.
