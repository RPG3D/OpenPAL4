#include "pal4/shop_ui_update.h"

#include <algorithm>

namespace pal4 {

std::uint32_t ShopUiUpdate::Update(const std::uint32_t frame,
                                   IShopUiUpdateRuntime& runtime) {
    for (std::uint32_t offset = 0x40u; offset < 0x60u; offset += 4u) {
        runtime.SetVisible(runtime.ReadWord(frame + offset), false);
    }

    const auto mouse_begin = runtime.ReadWord(frame + 0x30u);
    runtime.WriteWord(frame + 0x34u, mouse_begin);
    std::uint32_t position = 0u;
    const auto shop_text = frame + 0x60u;
    const auto mouse_list = frame + 0x28u;
    const auto show = [&](const std::uint32_t window_offset,
                          const bool bind_mouse = true) {
        const auto window = runtime.ReadWord(frame + window_offset);
        runtime.SetVisible(window, true);
        runtime.SetPosition(window, frame + 0x80u + position * 8u);
        ++position;
        if (bind_mouse) runtime.SetupMouseEventHandlers(mouse_list, window);
    };

    if (runtime.HasTradeItems(runtime.TradeSystem(), shop_text, 0u)) {
        show(0x40u, false);
        show(0x44u, false);
        runtime.SetupMouseEventHandlers(mouse_list,
                                        runtime.ReadWord(frame + 0x40u));
        runtime.SetupMouseEventHandlers(mouse_list,
                                        runtime.ReadWord(frame + 0x44u));
    }
    if (runtime.HasTradeItems(runtime.TradeSystem(), shop_text, 1u)) {
        show(0x48u, false);
        show(0x4cu, false);
        runtime.SetupMouseEventHandlers(mouse_list,
                                        runtime.ReadWord(frame + 0x48u));
        runtime.SetupMouseEventHandlers(mouse_list,
                                        runtime.ReadWord(frame + 0x4cu));
    }
    if (runtime.HasTradeItems(runtime.TradeSystem(), shop_text, 2u)) {
        show(0x50u);
    }

    const auto text_length = runtime.ReadWord(frame + 0x78u);
    const auto empty_length = runtime.CStringLength(kEmptyErrorString);
    const auto compared = std::min(text_length, empty_length);
    if (runtime.CompareBytes(runtime.ReadWord(frame + 0x74u),
                             kEmptyErrorString, compared) != 0 ||
        text_length != empty_length) {
        show(0x58u);
    }

    if (runtime.GameEntry(runtime.Game(), 15u) != 0u) {
        const auto database = runtime.GameDatabase();
        const auto shop = runtime.FindShopRecord(database + 0x6cu, shop_text);
        if (shop != 0u && runtime.ReadWord(shop + 0x34u) == 0u) {
            show(0x54u);
        }
    }
    show(0x5cu);

    const auto bottom = runtime.ReadWord(frame + 0xc0u);
    const auto new_height = runtime.GetHeight(runtime.ReadWord(frame + 0x40u)) *
                            static_cast<float>(position);
    const auto delta = new_height - runtime.GetHeight(bottom);
    runtime.SetHeight(bottom, new_height);
    const auto image = runtime.ReadWord(frame + 0xc4u);
    runtime.SetYPosition(image, runtime.GetYPosition(image) + delta);

    if (mouse_begin != 0u) {
        const auto mouse_end = runtime.ReadWord(frame + 0x34u);
        if ((static_cast<std::int32_t>(mouse_end - mouse_begin) >> 2) != 0) {
            runtime.WriteWord(frame + 0x3cu, 0u);
        }
    }
    return mouse_begin;
}

}  // namespace pal4
