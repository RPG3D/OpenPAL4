#pragma once

#include <cstdint>

#include "pal4/game_config_initializer.h"

namespace pal4 {

class IGameFrameTimingRuntime {
public:
    virtual ~IGameFrameTimingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t TimeGetTime() = 0;
    virtual void ProcessInputs() = 0;
    virtual void InvokeFrame(double elapsed, std::uint32_t flag) = 0;
};

class GameFrameUpdater final {
public:
    static constexpr std::uint32_t kFrameCountOffset = 0x540u;
    static constexpr std::uint32_t kPreviousFrameCountOffset = 0x544u;
    static constexpr std::uint32_t kCurrentTimeOffset = 0x56cu;
    static constexpr std::uint32_t kWindowStartOffset = 0x570u;
    static constexpr std::uint32_t kPreviousTimeOffset = 0x574u;
    static constexpr std::uint32_t kDeltaCapOffset = 0x578u;
    static constexpr std::uint32_t kFirstFrameOffset = 0x580u;

    GameFrameUpdater(GameConfigWords& config, GameRenderState& state,
                     float& accumulator)
        : config_(config), state_(state), accumulator_(accumulator) {}

    void Update(IGameFrameTimingRuntime& runtime);

private:
    GameConfigWords& config_;
    GameRenderState& state_;
    float& accumulator_;
};

}  // namespace pal4
