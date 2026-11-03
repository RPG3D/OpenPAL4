#pragma once

#include "pal4/actor_manager_target_selection_controller.h"

#include <cstdint>

namespace pal4 {

class IWorldManagerFrameRuntime {
public:
    virtual ~IWorldManagerFrameRuntime() = default;
    virtual void UpdateWorldRenderParameters(
        std::uint32_t world, float delta) = 0;
};

class WorldManagerFrameUpdater final {
public:
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t manager, float delta,
        IWorldManagerFrameRuntime& world_runtime,
        IActorManagerTargetInputRuntime& selection_runtime);
};

}  // namespace pal4
