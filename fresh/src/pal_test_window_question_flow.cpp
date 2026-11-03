#include "pal4/pal_test_window_question_flow.h"

namespace pal4 {

std::uint32_t PalTestWindowQuestionFlow::SetCurrentQuestion(
    PalTestWindowState& state, std::uint32_t object_address,
    std::uint32_t category, IPalTestWindowQuestionFlowRuntime& runtime) {
    state.Write(0x70u, state.Read(0x6cu));
    state.Write(0x78u, 0u);
    state.bytes[0x7cu] = 1u;
    const auto records = runtime.ReadWord(runtime.GameDB() + 0xa0u);
    runtime.ResetQuestionCollection(state);
    const auto begin = runtime.ReadWord(records + 0x60u);
    const auto end = begin == 0u ? 0u : runtime.ReadWord(records + 0x64u);
    const auto count = begin == 0u ? 0u : (end - begin) / 0x4cu;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto record = begin + index * 0x4cu;
        if (runtime.ReadWord(record + 0x48u) == category + 1u)
            runtime.AppendQuestion(state, record);
    }
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.SetEnabled(state.Read(0x4cu + index * 4u), true);
    runtime.SetEnabled(state.Read(0x4cu + category * 4u), false);
    runtime.SetVisible(state.Read(0x64u), true);
    runtime.UpdateQuestionAndAnswers(state);
    const auto manager = runtime.FrameManager();
    const auto result = runtime.InsertActive(manager + 0x134u,
                                             object_address + 0x84u);
    state.Write(0x3cu, category);
    state.Write(0x80u, category);
    return result;
}

std::uint32_t PalTestWindowQuestionFlow::ResetUI(
    PalTestWindowState& state, std::uint32_t object_address,
    IPalTestWindowQuestionFlowRuntime& runtime) {
    runtime.SetVisible(state.Read(0x64u), false);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.SetEnabled(state.Read(0x4cu + index * 4u), true);
    for (std::uint32_t index = 0u; index < 3u; ++index)
        runtime.SetSelected(state.Read(0x58u + index * 4u), false);
    const auto manager = runtime.FrameManager();
    return runtime.RemoveActive(manager + 0x134u, object_address + 0x84u);
}

std::uint32_t PalTestWindowQuestionFlow::ProcessAnswer(
    PalTestWindowState& state, std::uint32_t object_address,
    IPalTestWindowQuestionFlowRuntime& runtime) {
    const bool passed = state.bytes[0x7cu] != 0u;
    if (passed) {
        const auto category = state.Read(0x80u);
        if (category <= 2u) {
            runtime.SetCompletionFlag(0x1cu + category * 4u, 1u);
            runtime.SaveSettings();
        }
    }
    (void)ResetUI(state, object_address, runtime);
    if (passed) runtime.PlayMusic("WA23.WAV", false);
    const auto text = runtime.TriggerItemText(passed ? 0xd7u : 0xd8u);
    const auto manager = runtime.FrameManager();
    return runtime.ShowCommonDialog(manager, text, 0.4f, 0.4f, 4u, 0u);
}

}  // namespace pal4
