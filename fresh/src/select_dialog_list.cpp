#include "pal4/select_dialog.h"

#include "pal4/preview_text_codec.h"

#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {

std::uint32_t SelectDialog::InitializeList(
    const std::uint32_t object, const std::u32string& question,
    const std::uint32_t option, ISelectDialogListRuntime& runtime) {
    runtime.CleanupCallback(object);
    runtime.WriteWord(object + 64u, option);
    runtime.WriteWord(object + 156u, 0xffffffffu);
    runtime.ClearNavigation(object + 44u);
    runtime.WriteWord(object + 60u, 0u);
    runtime.SetText(runtime.ReadWord(object + 132u), question);

    const auto begin = runtime.ReadWord(object + 72u);
    const auto end = runtime.ReadWord(object + 76u);
    const auto count = begin == 0u ? 0u : (end - begin) / 16u;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto available_begin = runtime.ReadWord(object + 88u);
        const auto available_end = runtime.ReadWord(object + 92u);
        std::uint32_t button = 0u;
        if (available_begin != 0u &&
            available_begin != available_end) {
            button = runtime.ReadWord(available_begin);
            for (auto cursor = available_begin + 4u;
                 cursor != available_end; cursor += 4u) {
                runtime.WriteWord(cursor - 4u,
                                  runtime.ReadWord(cursor));
            }
            runtime.WriteWord(object + 92u, available_end - 4u);
        } else {
            button = runtime.CreateButton(object);
        }

        const auto text_window = runtime.ReadWord(
            runtime.ReadWord(button + 32u));
        const auto source = runtime.ReadWord(begin + index * 16u + 4u);
        const auto utf8 = runtime.ConvertToUtf8(
            source == 0u ? std::string{} :
                           runtime.ReadCString(source));
        runtime.SetText(text_window, PreviewTextCodec::DecodeUtf8(utf8));
        runtime.WriteWord(text_window + 280u, index);
        runtime.SetupMouseHandlers(object + 40u, text_window);
        runtime.AppendActiveButton(object + 100u,
                                   runtime.ReadWord(object + 108u), button);

        const auto template_window = runtime.ReadWord(object + 140u);
        const auto x = std::bit_cast<float>(
            runtime.ReadWord(object + 116u));
        const auto y = std::bit_cast<float>(
            runtime.ReadWord(object + 120u)) +
            runtime.GetHeight(template_window) *
                static_cast<float>(index);
        runtime.AddChild(runtime.ReadWord(object + 152u), button);
        runtime.SetPosition(button, x, y);
        runtime.SetWidth(button, runtime.GetWidth(template_window));
        runtime.SetHeight(button, runtime.GetHeight(template_window));
    }

    const auto x = std::bit_cast<float>(
        runtime.ReadWord(object + 116u));
    const auto y = std::bit_cast<float>(
        runtime.ReadWord(object + 120u)) +
        runtime.GetHeight(runtime.ReadWord(object + 140u)) *
            static_cast<float>(count);
    runtime.SetPosition(runtime.ReadWord(object + 144u), x, y);

    const auto root = runtime.ReadWord(object + 36u);
    runtime.AddChild(runtime.DesktopWindow(), root);
    runtime.SetAlwaysOnTop(root, true);
    runtime.SetModalState(root, true);
    runtime.SetPositionWithMetrics(root, 0u,
                                   std::bit_cast<float>(0x3eb33333u),
                                   std::bit_cast<float>(0x3e99999au));
    if (runtime.ReadWord(object + 48u) != 0u &&
        runtime.ReadWord(object + 52u) !=
            runtime.ReadWord(object + 48u)) {
        runtime.WriteWord(object + 60u, 0u);
    }
    return runtime.RefreshNavigation(object + 40u);
}

}  // namespace pal4
