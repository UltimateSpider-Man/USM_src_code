#pragma once
#include <cstdint>

namespace openusm::frontend_throb {

// Separate from the x86 hooks so the actual lifetime/retry policy can be
// tested without launching the game. Audio owns the native bank and voice.
template <class Audio>
class lifecycle {
    Audio audio_{};
    bool active_ = false;
    bool attempted_play_ = false;
    std::uint32_t last_play_attempt_ = 0;

public:
    Audio &audio() { return audio_; }

    void update(bool active, bool ready, std::uint32_t now)
    {
        if (!active) {
            stop();
            return;
        }
        if (!active_) {
            active_ = true;
            audio_.begin(now);
        }
        audio_.update(now);
        if (!ready || audio_.playing() || !audio_.ready())
            return;
        // GetTickCount wraparound is intentional. Failed native allocation
        // must not start many voices per frame or prevent later recovery.
        if (attempted_play_ && std::uint32_t(now - last_play_attempt_) < 250u)
            return;
        attempted_play_ = true;
        last_play_attempt_ = now;
        audio_.play();
    }

    void stop()
    {
        if (!active_)
            return;
        // A live source must never outlive its owned bank/file.
        audio_.stop();
        audio_.close();
        active_ = false;
        attempted_play_ = false;
    }
};

} // namespace openusm::frontend_throb
