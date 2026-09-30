#pragma once
// Retail PC host/join. No native structure is enlarged. XBPACK targets are no-op.
struct InputState;
bool multiplayer_mode_install();
void multiplayer_mode_tick_before(float game_dt);
void multiplayer_mode_tick_after(float game_dt);
void multiplayer_mode_world_shutdown();
// Destroy proxies borrowing HERO resources before a costume/hero pack unload.
// Keep network membership and deferred owned-pack cleanup alive.
void multiplayer_mode_hero_pack_unloading();
void multiplayer_mode_draw_overlay();
bool multiplayer_mode_active();
// Isolated arena owns the frame; callers must still preserve loading/frontend
// processes and continue device/resource polling outside native world advance.
bool multiplayer_mode_blocks_world();
// Only session UI / controller-owned root menu capture input. Native gameplay
// keeps its own controller, movement, animation state machines and camera.
bool multiplayer_mode_captures_native_input();
bool multiplayer_mode_owns_native_pad(unsigned native_handle);
void multiplayer_mode_merge_native_input(unsigned native_handle,InputState&);
