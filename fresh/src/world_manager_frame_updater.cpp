#include "pal4/world_manager_frame_updater.h"

namespace pal4 {

std::uint32_t WorldManagerFrameUpdater::Update(
    const std::uint32_t manager, const float delta,
    IWorldManagerFrameRuntime& world_runtime,
    IActorManagerTargetInputRuntime& selection_runtime) {
    world_runtime.UpdateWorldRenderParameters(manager + 116u, delta);
    return ActorManagerTargetSelectionController::ProcessInput(
        manager, delta, selection_runtime);
}

}  // namespace pal4
