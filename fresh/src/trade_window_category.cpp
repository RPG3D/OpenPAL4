#include "pal4/trade_window_trade_actions.h"
namespace pal4 {
namespace {

bool MoveSelection(const std::uint32_t frame, const bool next,
                   ITradeWindowCategoryRuntime& runtime) {
    const auto mode = runtime.ReadWord(frame + 0xe8u);
    if (mode > 2u) return true;
    const auto offset = mode == 0u ? 0xecu : mode == 1u ? 0xf0u : 0xf4u;
    const auto selected = runtime.AdjacentIndex(
        mode, runtime.ReadWord(frame + offset), next);
    // All three IDA branches write the new index before updating the page.
    runtime.WriteWord(frame + offset, selected);
    runtime.UpdatePage(frame, mode, selected, next ? 1u : 0u);
    return true;
}

}  // namespace

bool TradeWindowTradeActions::OnPrevButton(
    const std::uint32_t frame, ITradeWindowCategoryRuntime& runtime) {
    return MoveSelection(frame, false, runtime);
}

bool TradeWindowTradeActions::OnNextButton(
    const std::uint32_t frame, ITradeWindowCategoryRuntime& runtime) {
    return MoveSelection(frame, true, runtime);
}

}
