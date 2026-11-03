#include "pal4/pal_test_window_question_display.h"

#include "pal4/preview_text_codec.h"

namespace pal4 {

void PalTestWindowQuestionDisplay::Update(
    PalTestWindowState& state,
    IPalTestWindowQuestionDisplayRuntime& runtime) {
    const auto begin = state.Read(0x6cu);
    const auto count = begin == 0u ? 0u :
        (state.Read(0x70u) - begin) >> 2u;
    const auto index = state.Read(0x78u);
    constexpr std::uint32_t window_offsets[]{0x40u, 0x58u, 0x5cu, 0x60u};
    if (index >= count) {
        for (const auto offset : window_offsets)
            runtime.SetText(state.Read(offset), U"");
        runtime.ProcessAnswer(state);
        return;
    }

    const auto record = runtime.ReadWord(begin + index * 4u);
    constexpr std::uint32_t text_offsets[]{8u, 0x18u, 0x28u, 0x38u};
    for (std::uint32_t field = 0u; field < 4u; ++field) {
        const auto pointer = runtime.ReadWord(record + text_offsets[field]);
        const auto raw = pointer == 0u ? std::string{} :
            runtime.ReadCString(pointer);
        const auto utf8 = runtime.ConvertToUtf8(raw);
        runtime.SetText(state.Read(window_offsets[field]),
                        PreviewTextCodec::DecodeUtf8(utf8));
    }
    SetSelectedAnswer(state, 0u, runtime);
}

void PalTestWindowQuestionDisplay::SetSelectedAnswer(
    PalTestWindowState& state, std::uint32_t index,
    IPalTestWindowQuestionDisplayRuntime& runtime) {
    for (std::uint32_t slot = 0u; slot < 3u; ++slot)
        runtime.SetSelected(state.Read(0x58u + slot * 4u), false);
    runtime.SetSelected(state.Read(0x58u + index * 4u), true);
}

std::int32_t PalTestWindowQuestionDisplay::SelectedAnswerIndex(
    const PalTestWindowState& state,
    IPalTestWindowQuestionDisplayRuntime& runtime) {
    for (std::int32_t index = 0; index < 3; ++index) {
        const auto answer = state.Read(0x58u +
            static_cast<std::uint32_t>(index) * 4u);
        if (runtime.ReadByte(answer + 0x4b2u) != 0u) return index;
    }
    return -1;
}

}  // namespace pal4
