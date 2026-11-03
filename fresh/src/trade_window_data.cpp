#include "pal4/trade_window.h"
#include <vector>
namespace pal4 {
bool TradeWindow::UpdateTradeItems(const std::uint32_t frame,
                                   const std::uint32_t event,
                                   ITradeWindowDataRuntime& runtime) {
    const auto group = runtime.ReadWord(event);
    runtime.WriteWord(frame + 0xe8u, group);
    if (runtime.ReadByte(frame + 0xf8u) != 0u) {
        const auto attribute = runtime.CopyShopId(
            runtime.ReadWord(event + 4u) + 4u);
        try {
            const auto data = GetDataPtr(runtime.GameDatabase() + 0x6cu,
                                         runtime);
            std::vector<std::uint32_t> filtered;
            const auto count = runtime.TradeItemCount(data);
            filtered.reserve(count);
            for (std::uint32_t index = 0; index < count; ++index) {
                const auto item = runtime.TradeItemAt(data, index);
                if (!runtime.ShopIdDiffers(item + 0x14u, attribute)) {
                    filtered.push_back(item);
                }
            }
            if (group <= 2u) {
                runtime.ClearTradeItems(frame, group);
                for (const auto item : filtered) {
                    if (runtime.ReadWord(item + 4u) != group ||
                        !MeetsOpenCondition(item, runtime)) {
                        continue;
                    }
                    const auto resolved = runtime.ResolveTradeItem(
                        group, runtime.ReadWord(item + 8u));
                    if (resolved != 0u) {
                        runtime.AppendTradeItem(frame, group, resolved);
                    }
                }
            }
        } catch (...) {
            runtime.ReleaseShopId(attribute);
            throw;
        }
        runtime.ReleaseShopId(attribute);
    }

    const auto active = runtime.FrameManager() + 0x134u;
    runtime.RemoveActiveFrame(active, frame + 0x19cu);
    runtime.RemoveActiveFrame(runtime.FrameManager() + 0x134u,
                              frame + 0x1a8u);
    if (group <= 2u) {
        const bool shop = group == 0u;
        runtime.SetWindowVisible(runtime.ReadWord(frame + 0xd4u), shop);
        runtime.SetWindowVisible(runtime.ReadWord(frame + 0xd8u), !shop);
        runtime.UpdateTradePage(frame, group, 0u, 1u);
        runtime.InsertActiveFrame(runtime.FrameManager() + 0x134u,
                                  frame + (group == 1u ? 0x1a8u : 0x19cu));
    }
    const auto enabled = HasMultipleCategories(frame, runtime);
    runtime.SetWindowVisible(runtime.ReadWord(frame + 0x44u), enabled);
    runtime.SetWindowVisible(runtime.ReadWord(frame + 0x48u), enabled);
    return true;
}

std::uint32_t TradeWindow::GetDataPtr(const std::uint32_t data_field,
                                      ITradeWindowDataRuntime& runtime) {
    return runtime.ReadWord(data_field + 8u);
}

bool TradeWindow::MeetsOpenCondition(const std::uint32_t item,
                                   ITradeWindowDataRuntime& runtime) {
    const auto game = runtime.GameDataOwner();
    const auto condition = runtime.ReadWord(item + 0xcu);
    const auto threshold = static_cast<std::int32_t>(runtime.ReadWord(item + 0x10u));
    return runtime.GameDataValue(game, condition) >= threshold;
}

bool TradeWindow::HasMultipleCategories(const std::uint32_t frame,
                                  ITradeWindowDataRuntime& runtime) {
    const auto group = runtime.ReadWord(frame + 0xe8u);
    if (group > 2u) return false;
    if (runtime.ReadByte(frame + 0xf8u) != 0u) {
        const auto base = frame + 0x130u + group * 0x10u;
        const auto begin = runtime.ReadWord(base + 4u);
        if (begin == 0u) return false;
        const auto end = runtime.ReadWord(base + 8u);
        const auto count = (end - begin) / 4u;
        const auto field = group == 0u ? 0x94u : group == 1u ? 4u : 0x64u;
        std::uint32_t reference = 0u;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto item = runtime.ReadWord(begin + i * 4u);
            if (reference == 0u) {
                reference = item;
            } else if (runtime.ReadWord(reference + field) !=
                       runtime.ReadWord(item + field)) {
                return true;
            }
        }
        return false;
    }

    const auto limit = group == 0u ? 7u : group == 1u ? 5u : 3u;
    std::uint32_t nonempty = 0u;
    for (std::uint32_t i = 0; i < limit; ++i) {
        const auto inventory = runtime.InventorySubsystem();
        const auto size = group == 0u
            ? runtime.InventoryElementSize(inventory + 0x7cu, i)
            : group == 1u
                ? runtime.InventoryCalculatedValue(inventory, i)
                : runtime.InventorySubSize(inventory + 0x104u, i);
        if (size != 0) ++nonempty;
    }
    return nonempty > 1u;
}

bool TradeWindow::HasTradeItems(const std::uint32_t frame,
                                ITradeWindowDataRuntime& runtime) {
    const auto group = runtime.ReadWord(frame + 0xe8u);
    if (group > 2u) return false;
    if (runtime.ReadByte(frame + 0xf8u) != 0u) {
        const auto base = frame + 0x130u + group * 0x10u;
        const auto begin = runtime.ReadWord(base + 4u);
        return begin != 0u &&
               (runtime.ReadWord(base + 8u) - begin) / 4u != 0u;
    }

    const auto limit = group == 0u ? 7u : group == 1u ? 5u : 3u;
    std::int32_t total = 0;
    for (std::uint32_t i = 0; i < limit; ++i) {
        const auto inventory = runtime.InventorySubsystem();
        total += group == 0u
            ? runtime.InventoryElementSize(inventory + 0x7cu, i)
            : group == 1u
                ? runtime.InventoryCalculatedValue(inventory, i)
                : runtime.InventorySubSize(inventory + 0x104u, i);
    }
    return total > 0;
}

bool TradeWindow::SetActiveState(const std::uint32_t frame,
                                 const std::uint32_t event,
                                 ITradeWindowDataRuntime& runtime) {
    const auto active = static_cast<std::int32_t>(runtime.ReadWord(event)) > 0;
    runtime.WriteByte(frame + 0xf8u, active ? 1u : 0u);
    runtime.SetWindowVisible(runtime.ReadWord(frame + 0xdcu), active);
    runtime.SetWindowVisible(runtime.ReadWord(frame + 0xe0u), !active);
    return true;
}

}
