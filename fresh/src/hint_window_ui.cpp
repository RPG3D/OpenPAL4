#include "pal4/hint_window.h"

#include "pal4/preview_text_codec.h"

#include <bit>

namespace pal4 {

void HintWindow::ConvertAndShow(
    const std::uint32_t window, const std::string_view source,
    const HintPosition position, const std::uint32_t duration,
    const std::uint32_t parent, const std::uint8_t flag,
    IHintWindowUiRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(source);
    const auto text = PreviewTextCodec::DecodeUtf8(utf8);
    Show(window, text, position, duration, parent, flag, runtime);
}

void HintWindow::Show(const std::uint32_t window,
                      const std::u32string_view text,
                      const HintPosition position,
                      const std::uint32_t duration,
                      const std::uint32_t parent,
                      const std::uint8_t flag,
                      IHintWindowUiRuntime& runtime) {
    runtime.WriteWord(window + 68u, 0xbf800000u);
    runtime.WriteByte(window + 64u, flag);
    runtime.SetText(runtime.ReadWord(window + 40u), text);
    const auto target = parent != 0u ? parent : runtime.DefaultSheet();
    (void)runtime.SetVisibility(window, true, target);
    const auto root = runtime.ReadWord(window + 36u);
    runtime.SetPosition(root, 0u, position);
    runtime.SetAlwaysOnTop(root, true);
    runtime.PrepareLayout(window);
    runtime.PrepareState(window);
    runtime.RemoveQueued(window + 48u);
    runtime.QueueClose(window + 48u, duration, window, 0x4abe30u);
    runtime.WriteWord(window + 68u, 0u);
}

void HintWindow::Center(const std::uint32_t window,
                        IHintWindowUiRuntime& runtime) {
    const auto root = runtime.ReadWord(window + 36u);
    runtime.SetX(root, runtime.GetX(root) - runtime.GetWidth(root) * 0.5f);
    runtime.SetY(root, runtime.GetY(root) - runtime.GetHeight(root) * 0.5f);
}

bool HintWindow::ConditionalClose(const std::uint32_t window,
                                   IHintWindowUiRuntime& runtime) {
    if (runtime.ReadByte(window + 64u) != 0u &&
        std::bit_cast<float>(runtime.ReadWord(window + 68u)) >= 0.0f)
        (void)Close(window, runtime);
    return true;
}

}  // namespace pal4
