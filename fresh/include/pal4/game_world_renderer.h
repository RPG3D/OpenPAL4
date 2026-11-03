#pragma once

#include "pal4/game_world_object.h"

#include <cstdint>

namespace pal4 {

class IGameWorldDrawRuntime {
public:
    virtual ~IGameWorldDrawRuntime() = default;
    virtual void RenderWorld(std::uint32_t world) = 0;
};
class IGameWorldRenderRuntime : public IGameWorldDrawRuntime {
public:
    virtual void UpdateUvAnimation(GameWorldObjectState& state,
                                   std::uint32_t word_offset,
                                   std::uint32_t delta_bits) = 0;
};

class GameWorldRenderer final {
public:
    [[nodiscard]] static bool Render(const GameWorldObjectState& state,
                                     IGameWorldDrawRuntime& runtime);
    [[nodiscard]] static bool UpdateRenderParameters(
        GameWorldObjectState& state, std::uint32_t delta_bits,
        IGameWorldRenderRuntime& runtime);
};

}  // namespace pal4
