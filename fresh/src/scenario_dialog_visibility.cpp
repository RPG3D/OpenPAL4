#include "pal4/scenario_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint32_t ScenarioDialog::HideAndClear(
    const std::uint32_t object,
    IScenarioVisibilityRuntime& runtime) {
    const auto script = runtime.ScriptInstance();
    if (runtime.AudioActive(script + 32u)) {
        const auto playing_script = runtime.ScriptInstance();
        runtime.PlayAudio(playing_script + 32u);
    }
    const auto empty = runtime.ReadGlobalString(0x8dc780u);
    runtime.SetText(runtime.ReadWord(object + 56u), empty);
    runtime.SetImage(runtime.ReadWord(object + 44u), 0u);
    runtime.Layout(object, 1u);
    runtime.Close(object, 0u, 0u);
    const auto manager = runtime.UiFrameManager();
    return runtime.RemoveHandler(manager + 0x134u,
                                 object + 40u);
}

}  // namespace pal4
