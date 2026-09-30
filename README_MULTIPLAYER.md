# Ultimate Spider-Man PC multiplayer

## Switch Multiplayer Type

Open **MULTIPLAYER MODE**, then select **SWITCH MULTIPLAYER TYPE** to move between **ONLINE FREE-ROAM** and **NDS VERSUS - PC**. The selection is saved beside the game executable in `multiplayer.ini`:

```ini
[Multiplayer]
; 0 = online free-roam, 1 = NDS-inspired PC versus
Type=0
```

The default remains online free-roam. Its setup is documented in [README_ONLINE_IT.md](README_ONLINE_IT.md). The new NDS choice activates the separate two-player arena implementation using native PC character models, procedural 3D stages and NGL/D3D9 rendering.

**This is an NDS-inspired adaptation, not an exact port of the Nintendo DS game.** It does not contain extracted DS assets or recovered DS combat rules. Native in-game rendering, animation and two-PC play still need runtime verification. Passing source and portable checks does not establish a playable release.

## Opening an arena match

From the main menu, use **MULTIPLAYER MODE**, **M**, or the footer mouse click. The **PS/Guide button** or **F6** also opens multiplayer from the supported game/menu input path. Select **SWITCH MULTIPLAYER TYPE** to choose NDS Versus. In the NDS lobby, select both fighters, an arena, view, controls and rules, then choose **START MATCH**. **BACK** closes the lobby.

The lobby can open before the game's resource managers are ready. If actor loading reports that the world or resources are unavailable, load an existing save first and reopen multiplayer. Resource readiness and actor availability are checked before a match starts.

| Lobby input | Action |
|---|---|
| Up / Down or pad stick / D-pad | Select row |
| Left / Right | Change selected setting |
| Enter / Cross / A | Confirm or cycle setting |
| Options / Start | Start match or join host |
| Escape / Circle / B | Back |
| PS / Guide or F6 | Close lobby |

The lobby supports Spider-Man, Venom, Black Suit and Carnage; Warehouse, Subway, Queensboro Bridge and Football Field arenas; a round-win target; an optional timer; and an optional subway hazard. Views are **SHARED CAMERA**, **TOP / BOTTOM**, **SIDE BY SIDE**, and **DS-STYLE PANELS**. The panel view places one gameplay view above each player's combo-card display. Other layouts retain smaller combo displays.

Input presets are two keyboards, keyboard plus controller, two controllers, or automatic assignment. AUTO uses two controllers when available; with one controller it assigns P1 to that controller and P2 to the second keyboard layout; with no controllers it uses both keyboard layouts. See [README_MULTIPLAYER_GAMEPADS.md](README_MULTIPLAYER_GAMEPADS.md) for controller mapping and device configuration.

## Match controls

| Action | Player 1 keyboard | Player 2 keyboard | PlayStation / Xbox pad |
|---|---|---|---|
| Move | W / A / S / D | Arrow keys | Left stick / D-pad |
| Jump | Space | Numpad 0 | Cross / A |
| Light | F | Numpad 1 | Square / X |
| Heavy | G | Numpad 2 | Triangle / Y |
| Special | H | Numpad 3 | Circle / B |
| Guard | R | Numpad 4 | L1 / LB |
| Dodge | T | Numpad 5 | R1 / RB |
| Local pause | Escape or F6 | Escape or F6 | Options / Start or PS / Guide |

Enable Num Lock for the second keyboard layout. Simultaneous keys depend on the keyboard's rollover support. Attacks and jumping use press edges.

Pause/results menus use Up/Down and Cross/A/Enter. **R** requests a local rematch, **L** returns to character selection, and **Q** exits multiplayer to the underlying game or menu. Loss of focus or a required controller pauses local play. LAN is not independently pausable: focus loss sends neutral input, and Escape, Start, PS/Guide or F6 leaves the session for the lobby. After a network match, return to the lobby and host/join again; network rematch negotiation is not implemented.

Combat runs as a separate 60 Hz two-human simulation with jumping, directional guard, dodge, meter, hit reactions, simultaneous-hit resolution, combo cards, rounds and results. It does not create a second native story hero or use story-mode combat logic.

Switching from online free-roam to NDS Versus disconnects the online session and
releases its remote characters. While the arena owns a loaded running/paused
world, native story simulation is held; input, resource polling and rendering
continue. Closing the arena resumes the existing native state. Loading and
frontend transitions are allowed to finish.

## PC character resources

Fighters use installed native actor packs, skeletal animation and materials. Character selection does not install missing resources. If a suitable actor or compatible idle animation cannot load, the match reports the problem instead of substituting another character. **Black Suit needs a matching actor resource/mod.** Loose FBX or PCMESH files can replace assets through the mod loader but are not independently spawnable actor packs.

In `multiplayer.ini`, override `[SpiderMan]`, `[Venom]`, `[BlackSuit]` or `[Carnage]` with paired `Pack1` / `Entity1` values, up to six pairs. Use registered names without directories or extensions. A valid explicit list replaces the built-in candidates. `Scale` and `FloorOffset` affect presentation, not combat reach.

Use resource names, not menu labels: `spider-man` is not a retail pack. The
following pairs were verified in the installed PC pack directories:

| INI section | Pack1 | Entity1 |
|---|---|---|
| SpiderMan | ultimate_spiderman | ultimate_spiderman |
| Venom | venom | venom |
| BlackSuit | usm_blacksuit | usm_blacksuit |
| Carnage | carnage | carnage |

Leaving both values empty uses the built-in search. Multiplayer first borrows
an exact matching pack already loaded in the hero, common or mission
partition. Only newly pushed mission packs are owned and released by
multiplayer. Failed candidates log whether the pack, entity, space, or actor
data was missing in `multiplayer_online.log`.

Changing the story hero releases multiplayer actors before the hero pack is
unloaded. The online connection stays open and presence resumes after loading.
The supported retail hero-unload entry is signature-checked during hook setup.

Native ALS clips are selected by name and checked against the actor's skeleton type. Missing action clips use the compatible idle fallback and display a warning. Exact names can be supplied with `AnimIDLE`, `AnimMOVE`, `AnimLIGHT`, and the other keys documented in the INI. Mappings and load errors are recorded in the shared `multiplayer_online.log`.

## Private LAN

Use the same rules build and usable character resources on both PCs. Choose **HOST PRIVATE LAN** on the first PC and start. On the second, set `HostIP` to the host's literal IPv4 address in `multiplayer.ini`, choose **JOIN PRIVATE LAN**, and join. Both use `Port`, default TCP 7777. `127.0.0.1` reaches only the same PC.

The host selects fighters, arena and rules, and controls P1. The joining PC controls P2. Each PC selects its own visual layout. In LAN, presets 2 and 3 use local controller 1; AUTO uses local controller 1 when connected, otherwise the P1 keyboard; presets 0 and 1 use the P1 keyboard.

Transport uses framed TCP input and checksums in deterministic lockstep. It has no prediction, rollback, matchmaking, NAT traversal, spectators, mid-match join, disconnect recovery, authentication or encryption. It is intended for a trusted private LAN. No router or firewall settings are changed automatically.

## Build and verification

Reconfigure the root CMake project so its source glob includes `src/multiplayer_arena_mode.cpp`, then build the retail 32-bit PC target using the existing MinGW/DirectX/Assimp setup. Keep `ULTIMATE_RELEASE_XBPACK_MODE=OFF` and `USM_AUTO_DEPLOY=OFF`. Copy `multiplayer.ini` beside the executable that launches the game. Source changes do not update an already installed executable automatically.

Portable checks are under `test/multiplayer`; online regression checks are under `test/online`. The older [verification report](docs/multiplayer/VERIFICATION.md) records the original baseline and historical toolchain limits, not proof of current in-game behavior. The updated renderer compiles as a 32-bit Windows object with the repository's native include paths and ABI checks.

Run `powershell -NoProfile -ExecutionPolicy Bypass -File test/verify_multiplayer_pack_loading.ps1`
to check both production pack-acquisition paths with simulated resource slots,
including hero reuse, entity ownership, capacity and duplicate loading.

Runtime checks still needed include menu switching, native actor loading, first-frame materials/lighting, all four layouts, animation mapping, repeated teardown, alt-tab/device reset, controller reconnect and two-PC synchronization. Native calls depend on the supported retail PC executable addresses.

## Adaptation limits

The stages are procedural approximations, with simple effects and independently implemented combat timing and collision. This mode does not reproduce the DS stage meshes/textures, exact movesets, destructibles, music, voices, touch controls or animation timing. It does not add wall crawling, web swinging or story/free-roam co-op to the arena mode. Native PC character quality depends on the installed model and animation resources.
