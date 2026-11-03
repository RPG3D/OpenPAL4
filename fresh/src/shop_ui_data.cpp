#include "pal4/shop_ui_data.h"

namespace pal4 {

std::uint8_t ShopUiData::Load(
    const std::uint32_t frame, const std::uint32_t category,
    const std::uint8_t buy, IShopUiDataRuntime& runtime) {
    const auto vtable = runtime.ReadWord(frame);
    runtime.CallVisibilitySlot(runtime.ReadWord(vtable + 4u),
                               frame, 0u, 0u);
    runtime.EnsureGameDatabase(runtime.TradeSystem());
    return runtime.OpenShop(runtime.TradeSystem(),
                                      frame + 0x60u, buy, category);
}

}  // namespace pal4
