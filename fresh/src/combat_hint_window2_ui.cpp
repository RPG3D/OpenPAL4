#include "pal4/combat_hint_window2.h"
#include "pal4/preview_text_codec.h"

namespace pal4 {

void CombatHintWindow2::ShowText(
    const std::uint32_t window, const std::string_view source,
    const std::uint32_t position, const std::uint32_t duration,
    const std::uint32_t parent, const std::uint8_t flag,
    ICombatHintWindow2UiRuntime& runtime) {
    const auto utf8 = runtime.ConvertToUtf8(source);
    Show(window, PreviewTextCodec::DecodeUtf8(utf8), position,
         duration, parent, flag, runtime);
}

void CombatHintWindow2::Show(
    const std::uint32_t window, const std::u32string_view text,
    const std::uint32_t position, const std::uint32_t duration,
    const std::uint32_t parent, const std::uint8_t flag,
    ICombatHintWindow2UiRuntime& runtime) {
    const auto edit = runtime.ReadWord(window + 40u);
    runtime.WriteWord(window + 68u, 0xbf800000u);
    runtime.WriteByte(window + 64u, flag);
    runtime.SetText(edit, text);

    const auto sheet = parent != 0u ? parent : runtime.DefaultSheet();
    runtime.SetVisibility(window, true, sheet);
    const auto root = runtime.ReadWord(window + 36u);
    runtime.SetPosition(root, 0u, position);
    runtime.SetAlwaysOnTop(root, true);
    runtime.SetX(root, runtime.GetX(root) - runtime.GetWidth(root) * 0.5f);
    runtime.SetY(root, runtime.GetY(root) - runtime.GetHeight(root) * 0.5f);

    const auto manager = runtime.FrameManager();
    runtime.RemoveQueued(runtime.ReadWord(manager + 0x1a0u), window + 48u);
    const auto queue_manager = runtime.FrameManager();
    runtime.QueueClose2(runtime.ReadWord(queue_manager + 0x1a0u),
                        window + 48u, duration, window,
                        CombatHintWindow::kCleanupCallback);
    runtime.WriteWord(window + 68u, 0u);
}

}  // namespace pal4
