> **Documento storico.** Il percorso online corrente è descritto in `README_ONLINE_IT.md`.
> Arena, split-screen e relativi preset descritti sotto non sono più il percorso attivo.
> I dettagli dei profili gamepad restano utili; i nuovi comandi nativi sono nella guida Online M1.

# Ultimate Spider-Man PC — multiplayer controller update

**Source update for the corrected September 16 multiplayer project. Native Windows compilation, physical controllers and in-game rendering are not verified in this environment. No newly compiled EXE or DLL is included.**

This update adds controller operation to the custom **Multiplayer Mode** main-menu entry, lobby, local/private-LAN fighter inputs, pause menu and results menu. It does not remap story/free-roam gameplay or add story co-op.

## Apply and build

Use the full updated source package, or overlay the patch files into the **project root containing `CMakeLists.txt`** of `USM_16_09_2026_multiplayer_source_fixed.zip`. Do not copy source into `build/` or the installed game's directory. Back up files you have edited. The full package retains the previous BOM fix in `Ultimate_release.cpp`; the controller-only overlay assumes that earlier fix is already applied.

Reconfigure CMake: the new `src/multiplayer_gamepad_win.cpp` must be collected by the root source glob. From the project root, in the same MinGW/WSL build environment used for the project:

```sh
cmake -S . -B build -DULTIMATE_RELEASE_XBPACK_MODE=OFF -DUSM_AUTO_DEPLOY=OFF
cmake --build build --target USM ultimate_release -j2
```

The existing project already links `dinput8` and `dxguid`. No SDL DLL, new third-party controller library, driver installation, system setting or firewall modification is introduced. The new Windows backend follows this project's global `CINTERFACE` setting and loads the input DLLs from the Windows system directory.

Keep `multiplayer.ini` beside the executable that actually launches the game. When you have customized asset names, animation mappings or network settings, **merge** the new `[Gamepads]` and `[DirectInput]` sections and set `[Multiplayer] InputPreset=4` rather than overwriting your configuration. Restart the game after editing controller mappings.

## Device paths

**XInput:** preserves Xbox-compatible/virtual-controller input and scans all four XInput indices before assigning two logical seats.

**DirectInput8:** adds PS4/PS5 Sony VID/PID profiles and a configurable generic gamepad profile. DualShock 4 revisions, the Sony wireless adapter, DualSense and the Edge PID are recognized by the profile selector. The basic controls, not touch/gyro/haptic features, are mapped. These are implemented profiles, not a hardware certification list.

USB and Bluetooth transport are left to Windows and the installed driver: the controller must enumerate and report its controls through one of these APIs. Neither transport was physically tested here. Native DirectInput is intended to work without an XInput remapper, but runtime confirmation is still required.

The lobby shows the two selected device names. `multiplayer.log` records connection changes, logical seat IDs and DirectInput VID:PID. Devices can be connected while the game is running; DirectInput enumeration is refreshed approximately once per second. During a match, seat identities are locked so disconnecting P1 cannot turn P2's controller into P1. Reconnect the same device identity or return to the lobby to assign a replacement.

## Main menu and lobby

| Action | DualShock 4 / DualSense | Xbox-style / generic mapped pad |
|---|---|---|
| Navigate main menu or lobby rows | D-pad or left stick | D-pad or left stick |
| Open selected Multiplayer Mode entry / confirm | Cross | A / South |
| Open Multiplayer Mode directly from the root menu | Options | Start |
| Change lobby value | D-pad / stick Left or Right | D-pad / stick Left or Right |
| Start or join from the lobby | Options, or confirm START MATCH / JOIN HOST | Start, or confirm START MATCH / JOIN HOST |
| Return / cancel loading | Circle | B / East |

The virtual Multiplayer Mode entry remains after Quit (Down) and before Continue (Up). Native selection indices stay within 0–5; the seventh row does not enlarge the native `FEText` array. Raw and native controller processing are not both allowed to navigate the same root menu. Native transitions require the triggering buttons to be released before input is handed back to the stock menus.

The existing keyboard/mouse multiplayer entry remains: **M** or click the footer. Arrow keys, Enter and Escape remain available in the custom UI.

## Fighter controls

| Action | DualShock 4 / DualSense | Xbox-style pad |
|---|---|---|
| Move around the arena | Left stick / D-pad | Left stick / D-pad |
| Jump | Cross | A |
| Light attack | Square | X |
| Heavy attack | Triangle | Y |
| Special attack | Circle | B |
| Guard | L1 | LB |
| Dodge | R1 | RB |
| Local pause / resume | Options | Start |

Movement is mapped to the existing eight-way arena rules; this does not introduce analog walking speeds or free camera rotation. Triggers, stick clicks, touchpad, gyro, vibration and adaptive triggers have no gameplay function in this update. L2/R2/Select/L3/R3 can be decoded by the generic mapping, but are not assigned match actions.

The pause/results overlay is now a selectable menu. Use Up/Down, then Cross/A to choose **Resume**, **Rematch**, **Character Select / Lobby**, or **Return to Main Menu**, as appropriate. Circle/B or Options/Start resumes a local pause and returns to the lobby from results. Rematch is a local-match option; network results offer Lobby or Main Menu. Keyboard shortcuts R/L/Q remain available.

Opening, starting, resuming, rematching, recovering focus or reconnecting with a button held does not generate a fresh press. **Release held buttons and center the stick**, then press again. This prevents Cross/A used to confirm from also jumping or activating the next screen.

A local match pauses after loss of focus or a required controller. A missing controller prevents Resume/Rematch but not returning to the lobby or main menu. **LAN matches are not independently pausable: Options/Start or Escape leaves the session.** Focus loss or a missing local controller sends neutral LAN input instead of pausing only one peer.

## Player assignment

The default is now `InputPreset=4` (**AUTO**). Assignment is resolved when the match starts, not changed during it.

| Preset | Local match | Local input on each LAN PC |
|---|---|---|
| 0 | P1 keyboard + P2 keyboard | P1 keyboard |
| 1 | P1 keyboard + controller 1 as P2 | P1 keyboard |
| 2 | Controller 1 as P1 + controller 2 as P2 | Controller 1 only |
| 3 | Controller 1 as P1 + P2 keyboard | Controller 1 |
| 4 AUTO | Two pads: P1/P2 pads. One pad: P1 pad + P2 keyboard. No pads: two keyboards. | Controller 1 when connected, otherwise P1 keyboard |

A preset that explicitly needs an unavailable controller will not start loading fighter assets. The two keyboard layouts remain WASD/Space/F/G/H/R/T and arrows/Num0/Num1/Num2/Num3/Num4/Num5. Turn Num Lock on for the numpad layout.

## Generic pad remapping and duplicate devices

Generic DirectInput button order is not universal. Inspect the button numbers in Windows `joy.cpl` and override only the needed keys. Config values are **1-based** Windows button labels; `0` disables a binding. Put per-model overrides in `[DirectInput_VVVV_PPPP]`, replacing VVVV/PPPP with the hexadecimal VID/PID printed in the log. `[DirectInput]` overrides apply to every DirectInput device, including Sony devices.

Example profile illustrating the default Sony face/shoulder buttons, not a required configuration change:

```ini
[DirectInput_054C_0CE6]
Layout=playstation
South=2
East=3
West=1
North=4
L1=5
R1=6
Start=10
AxisX=0
AxisY=1
InvertX=0
InvertY=1
POV=0
```

`AxisX`/`AxisY`: 0=X, 1=Y, 2=Z, 3=Rx, 4=Ry, 5=Rz, 6/7=sliders; `-1` disables an axis. `POV`: 0–3, or `-1` disabled. Pads using individual D-pad buttons can use `DpadUp`, `DpadDown`, `DpadLeft`, `DpadRight`. Absent axes and POV objects are kept neutral. `[Gamepads] Deadzone=9000` is a normalized stick threshold (allowed 2000–25000), with a smaller release threshold to reduce menu jitter.

Automatic XUSB filtering excludes the usual DirectInput duplicate of an XInput device using its `IG_` identifier. It **cannot reliably correlate a remapper's virtual Xbox device with a separate physical Sony/generic HID**. When one physical controller occupies both seats, use only one path. For an existing XInput remapper:

```ini
[Gamepads]
Backend=xinput
Deadzone=9000
```

For physical HID controllers without that remapper, use `Backend=auto`, or `Backend=directinput` to explicitly disable XInput. These settings affect this multiplayer layer only. They do not configure, install or disable external remapper software.

## Verification and remaining acceptance

The portable rules/network tests, mapping/state tests and tests executing production menu/match orchestration with simulated devices passed using GCC, and again with Clang AddressSanitizer/UndefinedBehaviorSanitizer. The earlier BOM regression also passes. See `docs/multiplayer/GAMEPAD_VERIFICATION.md` for commands, raw logs and scope.

The native project configure step fails here because `/usr/bin/i686-w64-mingw32-gcc` and `g++` are absent. The Windows input backend, COM declarations/linkage, hook installation and rendered menus still require the real Windows build and game. The host engine/device adapter deliberately does not represent that evidence.

After a successful Windows build, check each physical USB/Bluetooth configuration: root menu wrapping, lobby navigation, all six fighter actions, two different pads simultaneously, start/resume button release, focus loss, unplug/replug, result-menu choices, original single-player menu transitions, and two-PC LAN input. Check low-resolution UI readability. Existing multiplayer actor, animation and rendering limitations still apply.

### API/profile references used during implementation

- Microsoft, XInput and DirectInput comparison (legacy-device coverage, side-by-side APIs, XUSB duplicate filtering): https://learn.microsoft.com/en-us/windows/win32/xinput/xinput-and-directinput
- Microsoft, DirectInput `SetCooperativeLevel` (foreground/nonexclusive ownership): https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee417921(v=vs.85)
- SDL project's Windows DirectInput controller mappings (Sony face buttons, Start, axes and POV): https://raw.githubusercontent.com/libsdl-org/SDL/SDL2/src/joystick/SDL_gamecontrollerdb.h

The input implementation and tests here are new code; SDL is not linked or bundled as a dependency. The references are engineering inputs, not claims that these project-specific integrations have been validated by Microsoft, Sony or SDL.
