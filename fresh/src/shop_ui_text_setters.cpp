#include "pal4/shop_ui_text_setters.h"

namespace pal4 {
namespace {

std::uint8_t Set(const std::uint32_t event,
                 const std::uint32_t text_offset,
                 IShopUiTextRuntime& runtime) {
    auto manager = runtime.FrameManager();
    auto name = runtime.ReadWord(ShopUiTextSetters::kFrameNameSource);
    auto frame = runtime.FindFrame(manager, name);
    if (frame == 0u) {
        manager = runtime.FrameManager();
        name = runtime.ReadWord(ShopUiTextSetters::kFrameNameSource);
        runtime.RegisterFrame(manager, name);
        manager = runtime.FrameManager();
        name = runtime.ReadWord(ShopUiTextSetters::kFrameNameSource);
        frame = runtime.FindFrame(manager, name);
        if (frame == 0u) return 1u;
    }

    const auto text = frame + text_offset;
    const auto source = runtime.ReadWord(event + 4u);
    const auto length = runtime.CStringLength(source);
    if (length > 0xfffffffdu) runtime.ThrowLength();

    const auto data = runtime.ReadWord(text + 4u);
    if (data != 0u) {
        const auto reference = runtime.ReadByte(data - 1u);
        if (reference != 0u && reference != 0xffu) {
            if (length == 0u) {
                runtime.WriteByte(data - 1u,
                    static_cast<std::uint8_t>(reference - 1u));
                runtime.WriteWord(text + 4u, 0u);
                runtime.WriteWord(text + 8u, 0u);
                runtime.WriteWord(text + 0xcu, 0u);
                return 1u;
            }
            runtime.Reallocate(text, length);
        } else if (length == 0u) {
            runtime.SetErrorFlag(text, 1u);
            return 1u;
        } else {
            const auto capacity = runtime.ReadWord(text + 0xcu);
            if (capacity > 0x1fu || capacity < length) {
                runtime.SetErrorFlag(text, 1u);
                runtime.Reallocate(text, length);
            }
        }
    } else if (length == 0u) {
        runtime.SetErrorFlag(text, 1u);
        return 1u;
    } else {
        const auto capacity = runtime.ReadWord(text + 0xcu);
        if (capacity > 0x1fu || capacity < length) {
            runtime.SetErrorFlag(text, 1u);
            runtime.Reallocate(text, length);
        }
    }

    runtime.Copy(runtime.ReadWord(text + 4u), source, length);
    runtime.WriteWord(text + 8u, length);
    runtime.WriteByte(runtime.ReadWord(text + 4u) + length, 0u);
    return 1u;
}

}  // namespace

std::uint8_t ShopUiTextSetters::SetShopId(
    const std::uint32_t event, IShopUiTextRuntime& runtime) {
    return Set(event, 0x60u, runtime);
}

std::uint8_t ShopUiTextSetters::SetTradeTaskScript(
    const std::uint32_t event, IShopUiTextRuntime& runtime) {
    return Set(event, 0x70u, runtime);
}

}  // namespace pal4
