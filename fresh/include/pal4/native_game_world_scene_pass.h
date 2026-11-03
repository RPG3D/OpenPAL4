#pragma once

#include "pal4/game_world_scene_pass.h"

#include <cstdint>
#include <functional>

namespace pal4 {

class NativeGameWorldScenePassRuntime final : public IGameWorldScenePassRuntime {
public:
    using RenderSequence =
        std::function<void(std::uint32_t, std::uint32_t)>;

    explicit NativeGameWorldScenePassRuntime(RenderSequence render_sequence);

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t Call(
        GameWorldScenePassOperation operation, std::uint32_t first,
        std::uint32_t second = 0u) override;

private:
    RenderSequence render_sequence_;
};

}  // namespace pal4
