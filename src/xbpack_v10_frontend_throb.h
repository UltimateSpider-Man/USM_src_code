#pragma once

// The PC executable running Xbox v10 packs. No menu/object layout changes.
namespace xbpack::v10_frontend_throb {
#if defined(OPENUSM_XBPACK_MODE) && defined(OPENUSM_XBPACK_V10) && !defined(TARGET_XBOX)
// active: FE state 4, outside the attract movie.
// ready: the Press Start pulse animation has reached visual state 7.
void update(bool active, bool ready);
void stop();
#else
inline void update(bool, bool) {}
inline void stop() {}
#endif
} // namespace xbpack::v10_frontend_throb
