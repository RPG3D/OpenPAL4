#include "pal4/puzzle_window_counter.h"
#include "pal4/preview_text_codec.h"

namespace pal4 {

std::uint8_t PuzzleWindowCounter::UpdateDisplay(
    const std::uint32_t frame, IPuzzleWindowCounterRuntime& runtime) {
    const auto gate = runtime.ReadByte(frame + 0x190u);
    if (gate != 0u) return gate;

    const auto count = runtime.ReadSignedWord(frame + 0x74u);
    const auto window = runtime.ReadWord(frame + 0x188u);
    if (count <= 10) {
        runtime.SetTextColours(window, {1.0f, 0.0f, 0.0f, 1.0f});
        if (count > 0) runtime.PlaySound("WA25.wav", 1u);
    } else {
        runtime.SetTextColours(window, {1.0f, 1.0f, 1.0f, 1.0f});
    }

    std::string formatted;
    try {
        const auto current = runtime.ReadSignedWord(frame + 0x74u);
        const auto format = runtime.TriggerFormat(0xe9u);
        formatted = runtime.FormatText(format.value_or(""), current);
        runtime.SetText(runtime.ReadWord(frame + 0x188u),
                        PreviewTextCodec::DecodeUtf8(
                            runtime.ConvertToUtf8(formatted)));
        if (runtime.ReadSignedWord(frame + 0x74u) == 0)
            runtime.ShowResult(frame, 0u);
    } catch (...) {
        (void)runtime.ReleaseFormatted(formatted);
        throw;
    }
    return static_cast<std::uint8_t>(runtime.ReleaseFormatted(formatted));
}

std::uint8_t PuzzleWindowCounter::Decrement(
    const std::uint32_t frame, IPuzzleWindowCounterRuntime& runtime) {
    const auto gate = runtime.ReadByte(frame + 0x190u);
    if (gate != 0u) return gate;
    auto count = runtime.ReadSignedWord(frame + 0x74u);
    if (count > 0) --count;
    runtime.WriteSignedWord(frame + 0x74u, count);
    return UpdateDisplay(frame, runtime);
}

}  // namespace pal4
