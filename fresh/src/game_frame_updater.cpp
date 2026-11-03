#include "pal4/game_frame_updater.h"

#include <bit>

namespace pal4 {

void GameFrameUpdater::Update(IGameFrameTimingRuntime& runtime) {
    if ((config_[3] & 0xffu) != 0u) return;

    if (state_.Read(kFirstFrameOffset) != 0u) {
        const auto initial_time = runtime.TimeGetTime();
        state_.Write(kPreviousTimeOffset, initial_time);
        state_.Write(kWindowStartOffset, initial_time);
    }
    const auto now = runtime.TimeGetTime();
    state_.Write(kCurrentTimeOffset, now);

    if (now > state_.Read(kWindowStartOffset) + 1000u) {
        state_.Write(kPreviousFrameCountOffset,
                     state_.Read(kFrameCountOffset));
        state_.Write(kFrameCountOffset, 0u);
        state_.Write(kWindowStartOffset, now);
    }
    state_.Write(kFrameCountOffset, state_.Read(kFrameCountOffset) + 1u);

    const auto milliseconds = now - state_.Read(kPreviousTimeOffset);
    constexpr auto kMillisecondsToSeconds = std::bit_cast<float>(0x3a83126fu);
    double elapsed = static_cast<double>(milliseconds) *
                     static_cast<double>(kMillisecondsToSeconds);
    const auto cap = std::bit_cast<float>(state_.Read(kDeltaCapOffset));
    if (elapsed >= static_cast<double>(cap)) elapsed = cap;
    state_.Write(kPreviousTimeOffset, now);
    if (state_.Read(kFirstFrameOffset) != 0u)
        state_.Write(kFirstFrameOffset, 0u);

    const auto accumulated = elapsed + static_cast<double>(accumulator_);
    accumulator_ = static_cast<float>(accumulated);
    constexpr auto kFrameThreshold = std::bit_cast<float>(0x3c888889u);
    if (accumulated >= static_cast<double>(kFrameThreshold)) {
        runtime.ProcessInputs();
        runtime.InvokeFrame(static_cast<double>(accumulator_), 0u);
        accumulator_ = 0.0f;
    }
}

}  // namespace pal4
