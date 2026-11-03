#include "pal4/frame_toolbar.h"

#include <string>

namespace pal4 {

std::uint32_t FrameToolbar::Construct(
    const std::uint32_t object, const std::string_view layout_name,
    IFrameToolbarRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    try {
        runtime.LoadLayout(object + 0x2cu, "decorator");
    } catch (...) {
        (void)runtime.DestroyBase(object);
        throw;
    }
    try {
        runtime.WriteWord(object, kVtable);
        (void)InitializeControls(object, runtime);
    } catch (...) {
        (void)Destroy(object, runtime);
        throw;
    }
    return object;
}

std::uint32_t FrameToolbar::Destroy(
    const std::uint32_t object, IFrameToolbarRuntime& runtime) {
    try {
        (void)runtime.DestroyBase(object + 0x2cu);
    } catch (...) {
        (void)runtime.DestroyBase(object);
        throw;
    }
    return runtime.DestroyBase(object);
}

std::uint32_t FrameToolbar::InitializeControls(
    const std::uint32_t object, IFrameToolbarRuntime& runtime) {
    const std::string close_name =
        std::string(runtime.WindowName(object)) + "btnClose";
    try {
        const auto button = runtime.GetWindow(object, close_name);
        runtime.WriteWord(object + 0x28u, button);
        const auto callback = runtime.Allocate(0xcu);
        if (callback != 0u) {
            runtime.WriteWord(callback + 4u, 0x4a63c0u);
            runtime.WriteWord(callback + 8u, object);
            runtime.WriteWord(callback, kCloseCallbackVtable);
        }
        const auto connection = runtime.SubscribeClicked(button, callback);
        if (connection != 0u) runtime.ReleaseConnection(connection);
        runtime.SetEnabled(runtime.ReadWord(object + 0x50u), false);
    } catch (...) {
        (void)runtime.ReleaseNameBuffer();
        throw;
    }
    return runtime.ReleaseNameBuffer();
}

std::uint32_t FrameToolbar::SetVisible(
    const std::uint32_t object, const bool visible,
    const std::uint32_t parent, IFrameToolbarRuntime& runtime) {
    runtime.SetParent(object, visible, parent);
    return runtime.SetNestedVisible(object + 0x2cu, visible, 0u);
}

std::uint8_t FrameToolbar::OnCloseButtonClick(
    IFrameToolbarRuntime& runtime) {
    runtime.ProcessUiEvent(runtime.FrameManager() + 0x104u,
                           kCloseEventKey, 0u, 0u);
    return 1u;
}

}  // namespace pal4
