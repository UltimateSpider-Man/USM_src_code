#pragma once

namespace xbpack::v10_fade
{
// Xbox globals 146/147 release the script fade through native 0x0015B850.
// Its "not animating" flag also covers a completed, held-black fade. In the
// PC state machine that state is 3/4, not 0: releasing only the hero leaves
// the screen held and its rate at zero. Let the PC fade-off routine perform
// the state/flag transition; preserve an existing fade in states 1/2.
// The beta's default release rate is -1 alpha/second. Global 147 additionally
// clears alpha immediately after releasing, while retaining that rate.
template <typename Manager, typename FadeOff, typename ReleaseHero>
inline void release_script_fade(Manager &manager, bool clear_alpha,
                                FadeOff fade_off, ReleaseHero release_hero)
{
    if (manager.field_FC == 3 || manager.field_FC == 4) {
        fade_off(1.0f);
    } else {
        release_hero();
        if (manager.field_FC == 0) {
            manager.field_F8 = -1.0f;
        }
    }

    if (clear_alpha) {
        manager.field_F4 = 0.0f;
    }
}
}
