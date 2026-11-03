#include "pal4/shop_ui_controls.h"

#include <array>

namespace pal4 {
namespace {

struct ButtonSpec {
    std::string_view suffix;
    std::uint32_t window_offset;
    std::uint32_t position_offset;
    std::uint32_t trigger;
    std::uint32_t callback;
    bool use_constructor;
};

constexpr std::array<ButtonSpec, 8> kButtons{{
    {"BtnBuyEquip",       0x40u, 0x80u, 147u, 0x4864f0u, true},
    {"BtnSellEquip",      0x44u, 0x88u, 148u, 0x486500u, true},
    {"BtnBuyProp",        0x48u, 0x90u, 145u, 0x486510u, false},
    {"BtnSellProp",       0x4cu, 0x98u, 146u, 0x486520u, false},
    {"BtnBuyDescription", 0x50u, 0xa0u, 149u, 0x486530u, false},
    {"BtnExit",           0x5cu, 0xb8u,  30u, 0x486540u, false},
    {"BtnTask",           0x58u, 0xb0u, 189u, 0x486570u, false},
    {"BtnSmith",          0x54u, 0xa8u, 258u, 0x486730u, false},
}};

std::uint32_t FindNamedWindow(const std::uint32_t buffer,
                              const std::uint32_t frame,
                              const std::string_view suffix,
                              IShopUiControlsRuntime& runtime) {
    auto prefix = runtime.ReadWord(frame + 0x18u);
    if (prefix == 0u) prefix = runtime.EmptyText();
    runtime.FormatName(buffer, prefix, suffix);
    return runtime.FindWindow(buffer);
}

void BindButton(const std::uint32_t frame,
                const std::uint32_t buffer,
                const ButtonSpec& button,
                IShopUiControlsRuntime& runtime) {
    const auto window = FindNamedWindow(buffer, frame, button.suffix,
                                        runtime);
    runtime.WriteWord(frame + button.window_offset, window);

    const auto storage = runtime.Allocate(0x18u);
    std::uint32_t callback = 0u;
    if (storage != 0u) {
        if (button.use_constructor) {
            try {
                callback = runtime.ConstructClickCallback(
                    storage, button.callback, 0u, frame);
            } catch (...) {
                runtime.Free(storage);
                throw;
            }
        } else {
            runtime.WriteWord(storage + 8u, button.callback);
            runtime.WriteWord(storage + 0xcu, 0u);
            runtime.WriteWord(storage + 0x10u, frame);
            runtime.WriteWord(storage, ShopUiControls::kClickCallbackVtable);
            callback = storage;
        }
    }

    std::uint32_t connection = 0u;
    try {
        connection = runtime.BindClicked(
            window, callback, runtime.ReadWord(ShopUiControls::kClickedEvent));
    } catch (...) {
        if (callback != 0u) runtime.ReleaseCallback(callback, 1u);
        throw;
    }
    if (connection != 0u) runtime.ReleaseConnection(connection);

    const auto [x_bits, y_bits] = runtime.GetPositionBits(window);
    runtime.WriteWord(frame + button.position_offset, x_bits);
    runtime.WriteWord(frame + button.position_offset + 4u, y_bits);

    const auto database = runtime.GameDatabase();
    auto text = runtime.TriggerText(database + 0x34u, button.trigger);
    if (text == 0u) text = runtime.EmptyText();
    runtime.SetWindowText(window, runtime.ConvertToUtf8(text));
}

}  // namespace

std::uint32_t ShopUiControls::Initialize(
    const std::uint32_t frame, IShopUiControlsRuntime& runtime) {
    const auto buffer = runtime.InitializeNameBuffer();
    try {
        for (const auto& button : kButtons) {
            BindButton(frame, buffer, button, runtime);
        }
        runtime.WriteWord(frame + 0xc0u,
                          FindNamedWindow(buffer, frame, "PanelButton",
                                          runtime));
        runtime.WriteWord(frame + 0xc4u,
                          FindNamedWindow(buffer, frame, "imgBottom",
                                          runtime));
    } catch (...) {
        static_cast<void>(runtime.CleanupNameBuffer(buffer));
        throw;
    }
    return runtime.CleanupNameBuffer(buffer);
}

}  // namespace pal4
