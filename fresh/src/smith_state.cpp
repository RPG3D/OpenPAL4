#include "pal4/smith_state.h"

#include <bit>

namespace pal4 {

std::uint32_t SmithState::EnterWrapper(
    std::uint32_t, std::uint32_t, std::uint32_t config,
    ISmithStateRuntime& runtime) {
    return Enter(config, runtime);
}

std::uint32_t SmithState::Enter(
    std::uint32_t config, ISmithStateRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    runtime.AddUIElement(runtime.UIFrameManager(), "BJ");
    runtime.WriteWord(runtime.GameState() + 1412u, 0u);
    runtime.WriteWord(runtime.GameState() + 1364u, 0u);
    bool open_with_system = false;
    if (config != 0u) {
        const auto value = runtime.ReadIntegerAttribute(
            config, "Smith_OpenWithSystem").value_or(0u);
        open_with_system = std::bit_cast<std::int32_t>(value) > 0;
    }
    runtime.GetActiveCamera(
        runtime.ReadWord(runtime.UIFrameManager() + 412u));
    runtime.ActivateCamera(runtime.CameraManager(), runtime.UICameraName());
    runtime.SetSmithFlag(runtime.SmithSystem(), open_with_system);
    return 1u;
}

std::uint32_t SmithState::Update(
    float delta, std::uint32_t, std::uint32_t,
    ISmithStateRuntime& runtime) {
    runtime.SetSmithState(runtime.SmithSystem(),
                          std::bit_cast<std::uint32_t>(delta));
    runtime.UpdateUI(runtime.UIFrameManager(), static_cast<double>(delta));
    return runtime.RenderEffectTransform(runtime.UIFrameManager(),
                                         static_cast<double>(delta));
}

std::uint32_t SmithState::LeaveWrapper(
    std::uint32_t, std::uint32_t, ISmithStateRuntime& runtime) {
    return Leave(runtime);
}

std::uint32_t SmithState::Leave(ISmithStateRuntime& runtime) {
    runtime.ClearSmithFlags(runtime.SmithSystem());
    runtime.CleanupEffects(runtime.UIFrameManager());
    return 1u;
}

}  // namespace pal4
