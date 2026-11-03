#include "pal4/script_scenario_portrait.h"

namespace pal4 {

std::uint8_t ScriptScenarioPortrait::Set(
    std::uint32_t name_payload, std::int32_t argument,
    IScriptScenarioPortraitRuntime& runtime) {
    const auto dialog = runtime.ScenarioDialog(runtime.UiFrameManager());
    const auto visible = runtime.IsVisible(dialog);
    if (visible != 0u) return visible;
    auto name = runtime.ReadWord(name_payload + 4u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    return runtime.SetPortrait(dialog, name, argument);
}

}  // namespace pal4
