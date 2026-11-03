#include "pal4/shop_ui_click_callback.h"

namespace pal4 {

std::uint32_t ShopUiClickCallback::Construct(
    const std::uint32_t storage, const std::uint32_t function,
    const std::uint32_t adjustment, const std::uint32_t frame,
    IShopUiClickCallbackRuntime& runtime) {
    runtime.WriteWord(storage + 8u, function);
    runtime.WriteWord(storage + 0xcu, adjustment);
    runtime.WriteWord(storage + 0x10u, frame);
    runtime.WriteWord(storage, kVtable);
    return storage;
}

}  // namespace pal4
