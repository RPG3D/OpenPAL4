#include "pal4/pal_test_window_actions.h"

namespace pal4 {

std::uint8_t PalTestWindowActions::OnCategoryClick(
    PalTestWindowState& state, std::uint32_t event,
    IPalTestWindowActionsRuntime& runtime) {
    const auto window = runtime.ReadWord(event + 8u);
    runtime.SetCurrentQuestion(state, runtime.ReadWord(window + 0x118u));
    return 1u;
}

std::uint8_t PalTestWindowActions::OnOK(
    PalTestWindowState& state, IPalTestWindowActionsRuntime& runtime) {
    const auto begin = state.Read(0x6cu);
    const auto count = begin == 0u ? 0u :
        (state.Read(0x70u) - begin) >> 2u;
    const auto index = state.Read(0x78u);
    if (index >= count) {
        runtime.ProcessAnswer(state);
    } else {
        const auto record = runtime.ReadWord(begin + index * 4u);
        if (runtime.ReadWord(record + 0x44u) !=
            runtime.SelectedAnswerIndex(state) + 1u)
            state.bytes[0x7cu] = 0u;
        state.Write(0x78u, index + 1u);
        runtime.UpdateQuestionAndAnswers(state);
    }
    return 1u;
}

std::uint8_t PalTestWindowActions::OnAnswerClick(
    PalTestWindowState& state, std::uint32_t event,
    IPalTestWindowActionsRuntime& runtime) {
    if (runtime.ReadWord(event + 0x1cu) == 0u) {
        const auto window = runtime.ReadWord(event + 8u);
        runtime.SetSelectedAnswer(state, runtime.ReadWord(window + 0x118u));
    }
    return 1u;
}

}  // namespace pal4
