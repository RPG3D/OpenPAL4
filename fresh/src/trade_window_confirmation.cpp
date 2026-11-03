#include "pal4/trade_window_trade_actions.h"
#include <string>
namespace pal4 {
namespace {

constexpr std::uint32_t kSuccessTrigger = 0x108u;
constexpr float kSuccessDialogScale = 0.5f;
constexpr float kErrorDialogScale = 0.3f;

void ShowResultDialog(const std::uint32_t trigger, const bool success,
                      const bool create_first,
                      ITradeWindowConfirmationRuntime& runtime) {
    if (create_first) runtime.CreateCommonDialog(runtime.FrameManager());
    const auto text = runtime.TriggerText(trigger);
    if (!create_first) runtime.CreateCommonDialog(runtime.FrameManager());
    const auto scale = success ? kSuccessDialogScale : kErrorDialogScale;
    runtime.ShowDialog(text, scale, scale, 4u, 0u);
}

void UpdateAndRestoreList(const std::uint32_t frame,
                          const std::uint32_t mode,
                          const std::uint32_t selected,
                          ITradeWindowConfirmationRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    const auto row = static_cast<std::int32_t>(
        runtime.ReadWord(list + 0x4c0u));
    const auto scrollbar = runtime.ReadWord(list + 0x4b4u);
    const auto scroll = runtime.ReadFloat(scrollbar + 0x4c0u);
    runtime.UpdatePage(frame, mode, selected, 1u);
    runtime.SetHoverRowIndex(list, row);
    runtime.SetScrollPosition(scrollbar, scroll);
}

std::vector<TradeBatchEntry> CollectCharacterItems(
    const std::uint32_t frame, const bool buy,
    ITradeWindowConfirmationRuntime& runtime) {
    std::vector<TradeBatchEntry> items;
    const auto list = runtime.ReadWord(frame + 0x28u);
    const auto rows = runtime.RowCount(list);
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto item = runtime.GridItem(list, row, 1);
        const auto quantity = runtime.SpinnerValue(item);
        if (quantity <= 0) continue;
        const auto id = runtime.ReadWord(item + 0x138u);
        std::uint32_t value = 0u;
        if (buy) {
            value = runtime.ReadWord(runtime.ResolveCharacterItem(
                runtime.GameDatabase(), id));
        } else {
            value = runtime.ReadWord(runtime.FindInventoryEntry(
                runtime.InventorySubsystem(), id, 5u) + 4u);
        }
        runtime.InsertBatchRecord(items, {value, quantity});
    }
    return items;
}

}  // namespace

bool TradeWindowTradeActions::OnTradeClick(
    const std::uint32_t frame, const std::uint32_t event,
    ITradeWindowConfirmationRuntime& runtime) {
    if (event != 1u) return true;
    const auto mode = runtime.ReadWord(frame + 0xe8u);
    const auto active = runtime.ReadByte(frame + 0xf8u) != 0u;
    if (active) {
        if (mode == 2u) {
            const auto hovered = runtime.GetPassiveFromHover(frame);
            if (hovered == 0u) return true;
            const auto result = runtime.LearnRecipe(
                runtime.TradeSystem(), runtime.ReadWord(hovered));
            if (result == 0u) {
                UpdateAndRestoreList(frame, 2u,
                    runtime.ReadWord(frame + 0xf4u), runtime);
            }
            ShowResultDialog(result == 0u ? kSuccessTrigger : result,
                             result == 0u, result == 0u, runtime);
        } else if (mode == 1u) {
            const auto items = CollectCharacterItems(frame, true, runtime);
            const auto result = runtime.AddMultipleItems(
                runtime.TradeSystem(), items);
            if (result == 0u) {
                UpdateAndRestoreList(frame, 1u,
                    runtime.ReadWord(frame + 0xf0u), runtime);
            }
            ShowResultDialog(result == 0u ? kSuccessTrigger : result,
                             result == 0u, result == 0u, runtime);
        } else if (mode == 0u) {
            const auto hovered = runtime.GetItemFromHover(frame);
            if (hovered == 0u) return true;
            const auto result = runtime.BuyItem(
                runtime.TradeSystem(), runtime.ReadWord(hovered + 0x90u));
            if (result == 0u) runtime.UpdateItemDisplay(frame, hovered);
            ShowResultDialog(result == 0u ? kSuccessTrigger : result,
                             result == 0u, true, runtime);
            UpdateAndRestoreList(frame, 0u,
                runtime.ReadWord(frame + 0xecu), runtime);
        }
    } else if (mode == 1u) {
        const auto items = CollectCharacterItems(frame, false, runtime);
        const auto result = runtime.RemoveMultipleItems(
            runtime.TradeSystem(), items);
        if (result == 0u) {
            // IDA48C600 sale branch: rebuild only. The purchased-list branch
            // restores its row/scroll, but sale may remove the last list row.
            runtime.UpdatePage(frame, 1u,
                runtime.ReadWord(frame + 0xf0u), 1u);
        }
        ShowResultDialog(result == 0u ? kSuccessTrigger : result,
                         result == 0u, result == 0u, runtime);
    } else if (mode == 0u) {
        const auto hovered = runtime.GetInventoryFromHover(frame);
        if (hovered == 0u) return true;
        runtime.SellItem(runtime.TradeSystem(),
                         runtime.ReadWord(hovered + 0x190u));
        runtime.UpdatePage(frame, 0u, runtime.ReadWord(frame + 0xecu), 1u);
        ShowResultDialog(kSuccessTrigger, true, true, runtime);
    }
    return true;
}

bool TradeWindowTradeActions::ShowTradePrompt(
    const std::uint32_t frame, ITradeWindowConfirmationRuntime& runtime) {
    const auto list = runtime.ReadWord(frame + 0x28u);
    if (runtime.RowCount(list) == 0) return true;
    runtime.CreateCommonDialog(runtime.FrameManager());
    const auto mode = runtime.ReadWord(frame + 0xe8u);
    const auto active = runtime.ReadByte(frame + 0xf8u) != 0u;
    if (mode == 2u && active) {
        const auto hovered = runtime.GetPassiveFromHover(frame);
        if (hovered != 0u && runtime.HasInventoryRecipe(
                runtime.InventorySubsystem() + 0x104u,
                runtime.ReadWord(hovered))) {
            runtime.ShowDialog(runtime.TriggerText(0xcbu),
                               0.5f, 0.5f, 4u, 0u);
            return true;
        }
    } else if (mode == 1u) {
        const auto rows = runtime.RowCount(list);
        bool has_quantity = false;
        for (std::int32_t row = 0; row < rows; ++row) {
            if (runtime.SpinnerValue(runtime.GridItem(list, row, 1)) > 0) {
                has_quantity = true;
                break;
            }
        }
        if (!has_quantity) {
            runtime.ShowDialog(runtime.TriggerText(active ? 0x76u : 0x77u),
                               0.5f, 0.5f, 4u, 0u);
            return true;
        }
    } else if (mode == 0u && !active) {
        const auto hovered = runtime.GetInventoryFromHover(frame);
        if (hovered != 0u && runtime.CheckPlayersCondition(hovered)) {
            runtime.ShowDialog(runtime.TriggerText(0x104u),
                               0.5f, 0.5f, 4u, 0u);
            return true;
        }
    }

    const auto callback = runtime.Allocate(12u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, 0x48c600u);
        runtime.WriteWord(callback + 8u, frame);
        runtime.WriteWord(callback, 0x843d80u);
    }
    try {
        runtime.ShowDialog(runtime.TriggerText(0x2cu),
                           0.3f, 0.3f, 3u, callback);
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback);
        throw;
    }
    if (callback != 0u) runtime.ReleaseCallback(callback);
    return true;
}

}
