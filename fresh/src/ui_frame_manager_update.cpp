#include "pal4/ui_frame_manager_singleton.h"

namespace pal4 {

std::uint32_t UiFrameManagerSingleton::Update(
    const std::uint32_t manager, const double elapsed,
    IUiFrameManagerUpdateRuntime& runtime) {
    const auto elapsed_float = static_cast<float>(elapsed);
    const auto system = runtime.ReadWord(manager + 0x198u);
    if (system != 0u) runtime.InjectTimePulse(system, elapsed_float);

    const auto objects = runtime.ReadWord(manager + 0x19cu);
    if (objects != 0u) runtime.RotateGameObjects(objects, elapsed);

    const auto jump_hint = runtime.ReadWord(manager + 0x1b8u);
    if (jump_hint != 0u && runtime.IsVisible(jump_hint)) {
        runtime.UpdateJumpHint(runtime.ReadWord(manager + 0x1b8u),
                               elapsed_float);
    }

    const auto special_frame = runtime.ReadWord(manager + 0x1d0u);
    if (special_frame != 0u && runtime.IsVisible(special_frame)) {
        runtime.UpdateFrame(runtime.ReadWord(manager + 0x1d0u), elapsed);
    }

    for (std::uint32_t index = 0;; ++index) {
        const auto begin = runtime.ReadWord(manager + 0x18cu);
        if (begin == 0u) break;
        const auto end = runtime.ReadWord(manager + 0x190u);
        if (index >= (end - begin) / 4u) break;
        const auto frame = runtime.ReadWord(begin + index * 4u);
        if (runtime.IsVisible(frame))
            runtime.UpdateFrame(frame, elapsed);
    }

    const auto effects = runtime.ReadWord(manager + 0x1a0u);
    runtime.UpdateTimedEffects(effects, elapsed_float);
    return runtime.UpdateAnimations(manager + 0x1dcu, elapsed_float);
}

}  // namespace pal4
