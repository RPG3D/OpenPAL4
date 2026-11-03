#include "pal4/jigsaw_window_counter.h"
#include "pal4/preview_text_codec.h"

namespace pal4 {

std::uint32_t JigsawWindowCounter::Update(
    const std::uint32_t frame, IJigsawWindowCounterRuntime& runtime) {
    std::string formatted;
    try {
        const auto count = runtime.ReadSignedWord(frame + 0x74u);
        const auto format = runtime.TriggerFormat(0xf5u);
        formatted = runtime.FormatText(format.value_or(""), count);
        const auto utf8 = runtime.ConvertToUtf8(formatted);
        runtime.SetText(runtime.ReadWord(frame + 0x188u),
                        PreviewTextCodec::DecodeUtf8(utf8));
        if (runtime.ReadSignedWord(frame + 0x74u) == 0)
            runtime.ShowResult(frame, 0u);
    } catch (...) {
        (void)runtime.ReleaseFormatted(formatted);
        throw;
    }
    return runtime.ReleaseFormatted(formatted);
}

std::uint8_t JigsawWindowCounter::Decrement(
    const std::uint32_t frame, IJigsawWindowCounterRuntime& runtime) {
    const auto gate = runtime.ReadByte(frame + 0x18cu);
    if (gate != 0u) return gate;
    auto count = runtime.ReadSignedWord(frame + 0x74u);
    if (count > 0) --count;
    runtime.WriteSignedWord(frame + 0x74u, count);
    return static_cast<std::uint8_t>(Update(frame, runtime));
}

}  // namespace pal4
