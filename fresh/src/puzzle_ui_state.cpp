#include "pal4/puzzle_ui_state.h"

namespace pal4 {

std::uint32_t PuzzleUiState::Enter(
    std::uint32_t, std::uint32_t config,
    IPuzzleUiRuntime& runtime) {
    runtime.SetUIFlag(runtime.UIFrameManager(), 1u);
    runtime.WriteWord(runtime.Console(), 0u);
    const auto mode = runtime.ReadIntegerAttribute(config, "Mode").value_or(0u);
    const auto type = runtime.ReadIntegerAttribute(config, "Type").value_or(0u);
    runtime.DispatchEvent(runtime.UIFrameManager(),
                          mode == 0u ? kShowPuzzleEvent : kShowJigsawEvent,
                          1u, type);
    return 1u;
}

std::uint32_t PuzzleUiState::Update(
    float delta, std::uint32_t, std::uint32_t,
    IPuzzleUiRuntime& runtime) {
    return runtime.UpdateUI(runtime.UIFrameManager(),
                            static_cast<double>(delta));
}

}  // namespace pal4
