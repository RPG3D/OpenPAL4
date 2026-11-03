#include "pal4/world_map_buttons.h"

namespace pal4 {

std::uint32_t WorldMapButtons::Initialize(
    const std::uint32_t frame, IWorldMapButtonsRuntime& runtime) {
    for (std::uint32_t i = 0u; i < kCount; ++i) {
        const auto window_field = frame + 0x40u + i * 4u;
        const auto window = runtime.ReadWord(window_field);
        runtime.WriteWord(frame + 0x1e4u + i * 4u, 1u);
        runtime.SetVisible(window, false);
        runtime.SetEnabled(runtime.ReadWord(window_field), true);
    }
    runtime.InitializeScripts();
    const auto mode = runtime.ScriptMode();
    const auto interpreter = runtime.CreateInterpreter(
        mode, "WorldMapIniter", "init", "init", kScriptFile, kReader);
    if (interpreter != 0u) {
        const auto engine = runtime.ScriptEngine();
        if (runtime.AddScriptModule(engine + 0x54u, interpreter)) {
            runtime.ExecuteImmediate(runtime.ScriptEngine(), interpreter);
            runtime.ReleaseInterpreter(interpreter);
        }
    }
    return runtime.CleanupAllScripts();
}

std::uint32_t WorldMapButtons::SetVisibility(
    const std::uint32_t frame, const std::uint32_t input,
    const std::uint32_t state, IWorldMapButtonStateRuntime& runtime) {
    const auto source = runtime.ReadWord(input + 4u);
    const auto name = runtime.MakeUppercaseName(source);
    try {
        const auto target = runtime.NameText(name);
        for (std::uint32_t i = 0u; i < kCount; ++i) {
            if (runtime.RecordText(frame + 0x94u + i * 0x10u) != target) {
                continue;
            }
            runtime.WriteWord(frame + 0x1e4u + i * 4u, state);
            const auto window = runtime.ReadWord(frame + 0x40u + i * 4u);
            runtime.SetVisible(window, state != 0u);
            break;
        }
    } catch (...) {
        (void)runtime.CleanupName(name);
        throw;
    }
    return runtime.CleanupName(name);
}

std::uint32_t WorldMapButtons::DispatchExternalVisibility(
    const std::uint32_t input, const std::uint32_t state,
    IWorldMapExternalButtonRuntime& external,
    IWorldMapButtonStateRuntime& buttons) {
    const auto manager = external.FrameManager();
    const auto name = external.ReadWord(0x8b089cu);
    const auto frame = external.FindFrame(manager, name);
    if (frame == 0u) return 0u;
    return SetVisibility(frame, input, state, buttons);
}

}  // namespace pal4
