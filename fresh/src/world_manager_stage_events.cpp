#include "pal4/world_manager_stage_events.h"

namespace pal4 {

void WorldManagerStageEvents::JoinWrapper(
    const std::uint32_t manager, const std::uint32_t event_args,
    IWorldManagerStageEventRuntime& runtime) {
    Join(manager, event_args, runtime);
}

void WorldManagerStageEvents::Join(
    const std::uint32_t manager, const std::uint32_t event_args,
    IWorldManagerStageEventRuntime& runtime) {
    JoinScene(manager+116u,event_args,runtime);
}

void WorldManagerStageEvents::JoinScene(
    const std::uint32_t scene,const std::uint32_t event_args,
    IWorldManagerStageEventRuntime& runtime) {
    const auto data = runtime.DialogData(event_args);
    if (data == 0u) return;
    const auto actor = runtime.ReadWord(data + 808u);
    if (actor == 0u) return;
    runtime.ActivateActor(actor, runtime.ReadWord(scene + 16u),
                          scene + 212u);
}

void WorldManagerStageEvents::LeaveWrapper(
    const std::uint32_t manager, const std::uint32_t event_args,
    IWorldManagerStageEventRuntime& runtime) {
    Leave(manager, event_args, runtime);
}

void WorldManagerStageEvents::Leave(
    const std::uint32_t manager, const std::uint32_t event_args,
    IWorldManagerStageEventRuntime& runtime) {
    (void)manager;
    const auto data = runtime.DialogData(event_args);
    if (data == 0u) return;
    const auto actor = runtime.ReadWord(data + 808u);
    if (actor == 0u) return;
    runtime.LeaveActor(actor, {0u, 0x7f7fffffu, 0u}, 0u);
}

}  // namespace pal4
