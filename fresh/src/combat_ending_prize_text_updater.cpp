#include "pal4/combat_ending_prize_text_updater.h"

#include <algorithm>
#include <string>

namespace pal4 {
namespace {

int RenderColumns(const std::uint32_t dialog,
                  const std::uint32_t head_offset,
                  const std::uint32_t left_offset,
                  const std::uint32_t right_offset,
                  const std::uint32_t panel_offset,
                  ICombatEndingResultTextRuntime& runtime) {
    std::string left;
    std::string right;
    bool next_left = true;
    const auto head = runtime.ReadWord(dialog + head_offset);
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.NextTreeNode(node)) {
        const auto key = runtime.ReadWord(node + 12u);
        auto name = runtime.PrimaryName(key);
        if (!name) name = runtime.SecondaryName(key);
        if (!name) continue;
        const auto text = runtime.FormatEntry(
            runtime.GameString(0xffu), *name,
            runtime.ReadWord(node + 16u));
        (next_left ? left : right).append(text);
        next_left = !next_left;
    }

    const auto left_window = runtime.ReadWord(dialog + left_offset);
    const auto right_window = runtime.ReadWord(dialog + right_offset);
    runtime.SetText(left_window, runtime.ToUtf8(left));
    runtime.SetText(right_window, runtime.ToUtf8(right));
    const auto text_height = std::max(runtime.GetTextHeight(left_window),
                                      runtime.GetTextHeight(right_window));
    const auto excess = runtime.GetHeight(left_window, 1u) - text_height;
    runtime.SetHeight(left_window, 1u, text_height);
    runtime.SetHeight(right_window, 1u, text_height);
    const auto panel = runtime.ReadWord(dialog + panel_offset);
    runtime.SetHeight(panel, 1u,
                      runtime.GetHeight(panel, 1u) - excess);
    return runtime.CleanupFormatBuffer();
}

}  // namespace

int CombatEndingPrizeTextUpdater::Update(
    const std::uint32_t dialog,
    ICombatEndingResultTextRuntime& runtime) {
    const auto count = runtime.ReadWord(dialog + 468u);
    if (count == 0u) return 0;

    runtime.SetFormatting(runtime.ReadWord(dialog + 112u), 2u, 2u);
    runtime.SetText(runtime.ReadWord(dialog + 112u),
                    runtime.ToUtf8(runtime.GameString(0x101u)));

    return RenderColumns(dialog, 460u, 104u, 108u, 88u, runtime);
}

int CombatEndingPropertyTextUpdater::Update(
    const std::uint32_t dialog,
    ICombatEndingResultTextRuntime& runtime) {
    return RenderColumns(dialog, 444u, 96u, 100u, 76u, runtime);
}

}  // namespace pal4
