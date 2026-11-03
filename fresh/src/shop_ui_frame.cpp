#include "pal4/shop_ui_frame.h"

#include <initializer_list>

namespace pal4 {
namespace {

void InitializeText(const std::uint32_t text,
                    const std::uint32_t name,
                    IShopUiFrameRuntime& runtime) {
    runtime.WriteByte(text, static_cast<std::uint8_t>(name));
    runtime.WriteWord(text + 4u, 0u);
    runtime.WriteWord(text + 8u, 0u);
    runtime.WriteWord(text + 0xcu, 0u);
}

void ReleaseText(const std::uint32_t data,
                 IShopUiFrameRuntime& runtime) {
    if (data == 0u) return;
    const auto reference = runtime.ReadByte(data - 1u);
    if (reference == 0u || reference == 0xffu) {
        runtime.Free(data - 1u);
    } else {
        runtime.WriteByte(data - 1u,
                          static_cast<std::uint8_t>(reference - 1u));
    }
}

}  // namespace

std::uint32_t ShopUiFrame::Construct(
    const std::uint32_t frame, const std::uint32_t name,
    IShopUiFrameRuntime& runtime) {
    runtime.LoadLayout(frame, name, 0u, 0u);
    auto stage = 1u;
    try {
        runtime.ConstructMouse(frame + 0x28u);
        stage = 2u;
        runtime.WriteWord(frame + 0x28u, kMouseInitialVtable);
        InitializeText(frame + 0x60u, name, runtime);
        InitializeText(frame + 0x70u, name, runtime);
        stage = 3u;
        for (std::uint32_t i = 0u; i < 8u; ++i) {
            runtime.ConstructVector2(frame + 0x80u + i * 8u);
        }
        runtime.WriteWord(frame, kFrameVtable);
        runtime.WriteWord(frame + 0x28u, kMouseFinalVtable);
        runtime.InitializeControls(frame);
    } catch (...) {
        if (stage >= 3u) {
            runtime.CleanupText(frame + 0x70u);
            runtime.CleanupText(frame + 0x60u);
        }
        if (stage >= 2u) runtime.CleanupMouse(frame + 0x28u);
        runtime.CleanupBase(frame);
        throw;
    }
    return frame;
}

std::uint32_t ShopUiFrame::ForwardVirtualSlotThree(
    const std::uint32_t frame, IShopUiFrameRuntime& runtime) {
    const auto vtable = runtime.ReadWord(frame);
    return runtime.CallVirtual(runtime.ReadWord(vtable + 0xcu), frame);
}

std::uint32_t ShopUiFrame::Destroy(
    const std::uint32_t frame, IShopUiFrameRuntime& runtime) {
    ReleaseText(runtime.ReadWord(frame + 0x74u), runtime);
    for (const auto offset : {0x74u, 0x78u, 0x7cu}) {
        runtime.WriteWord(frame + offset, 0u);
    }
    ReleaseText(runtime.ReadWord(frame + 0x64u), runtime);
    for (const auto offset : {0x64u, 0x68u, 0x6cu}) {
        runtime.WriteWord(frame + offset, 0u);
    }
    runtime.Free(runtime.ReadWord(frame + 0x30u));
    for (const auto offset : {0x30u, 0x34u, 0x38u}) {
        runtime.WriteWord(frame + offset, 0u);
    }
    return runtime.DestroyBase(frame);
}

}  // namespace pal4
