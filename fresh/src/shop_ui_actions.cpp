#include "pal4/shop_ui_actions.h"
#include "pal4/shop_ui_data.h"

namespace pal4 {
namespace {

std::uint8_t Select(const std::uint32_t frame,
                    const std::uint32_t category,
                    const std::uint32_t buy,
                    IShopUiActionsRuntime& runtime) {
    runtime.GetData(frame, category, buy);
    return 1u;
}

std::uint8_t Select(const std::uint32_t frame,
                    const std::uint32_t category,
                    const std::uint8_t buy,
                    IShopUiDataRuntime& runtime) {
    static_cast<void>(ShopUiData::Load(frame, category, buy, runtime));
    return 1u;
}

}  // namespace

std::uint8_t ShopUiActions::OnBuyEquip(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiActionsRuntime& runtime) {
    return Select(frame, 0u, 1u, runtime);
}

std::uint8_t ShopUiActions::OnBuyEquip(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiDataRuntime& runtime) {
    return Select(frame, 0u, 1u, runtime);
}

std::uint8_t ShopUiActions::OnSellEquip(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiActionsRuntime& runtime) {
    return Select(frame, 0u, 0u, runtime);
}

std::uint8_t ShopUiActions::OnSellEquip(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiDataRuntime& runtime) {
    return Select(frame, 0u, 0u, runtime);
}

std::uint8_t ShopUiActions::OnBuyProperty(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiActionsRuntime& runtime) {
    return Select(frame, 1u, 1u, runtime);
}

std::uint8_t ShopUiActions::OnBuyProperty(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiDataRuntime& runtime) {
    return Select(frame, 1u, 1u, runtime);
}

std::uint8_t ShopUiActions::OnSellProperty(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiActionsRuntime& runtime) {
    return Select(frame, 1u, 0u, runtime);
}

std::uint8_t ShopUiActions::OnSellProperty(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiDataRuntime& runtime) {
    return Select(frame, 1u, 0u, runtime);
}

std::uint8_t ShopUiActions::OnBuyDescription(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiActionsRuntime& runtime) {
    return Select(frame, 2u, 1u, runtime);
}

std::uint8_t ShopUiActions::OnBuyDescription(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiDataRuntime& runtime) {
    return Select(frame, 2u, 1u, runtime);
}

std::uint8_t ShopUiActions::OnClose(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiActionsRuntime& runtime) {
    static_cast<void>(Close(frame, runtime));
    return 1u;
}

std::uint32_t ShopUiActions::Close(
    const std::uint32_t frame, IShopUiActionsRuntime& runtime) {
    const auto vtable = runtime.ReadWord(frame);
    runtime.CallVisibilitySlot(runtime.ReadWord(vtable + 4u),
                               frame, 0u, 0u);
    return runtime.SetTradeState(runtime.TradeSystem());
}

}  // namespace pal4
