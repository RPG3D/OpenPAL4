#pragma once

#include <cstdint>

#include "pal4/actor_manager_combat_prepare.h"

namespace pal4 {

class IGameStateFrameScaleRuntime {
public:
    virtual ~IGameStateFrameScaleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameState() = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t ResetEffectHandle(
        ActorManagerEffectHandle& handle) = 0;
};

class GameStateFrameScale final {
public:
    static constexpr std::uint32_t kScaleOffset = 1404u;

    [[nodiscard]] static std::uint32_t SetAndReset(
        ActorManagerEffectHandle handle, float scale,
        IGameStateFrameScaleRuntime& runtime);
};

}  // namespace pal4
