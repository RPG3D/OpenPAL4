#include "pal4/repeat_game_window.h"

#include <cmath>
#include <string>

namespace pal4 {
namespace {

void Subscribe(const std::uint32_t object,
               const RepeatGameWindowEvent event,
               const std::uint32_t handler,
               IRepeatGameWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(0xcu);
    if (callback != 0u) {
        runtime.WriteWord(callback + 4u, handler);
        runtime.WriteWord(callback + 8u, object);
        runtime.WriteWord(callback, RepeatGameWindow::kCallbackVtable);
    }
    const auto connection = runtime.Subscribe(
        runtime.ReadWord(object + 0x24u), event, callback);
    if (connection != 0u) runtime.ReleaseSubscription(connection);
}

}  // namespace

std::uint32_t RepeatGameWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    IRepeatGameWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x34u, 0u);

    std::string pointer_name(runtime.WindowName(object));
    pointer_name += "ImgPointer";
    runtime.WriteWord(object + 0x28u,
                      runtime.GetWindow(object, pointer_name));
    runtime.WriteWord(object + 0x2cu, 0x41600000u);
    runtime.WriteWord(object + 0x30u, 0x43868000u);

    Subscribe(object, RepeatGameWindowEvent::KeyDown,
              0x4a21b0u, runtime);
    Subscribe(object, RepeatGameWindowEvent::MouseClick,
              0x4a2190u, runtime);
    return object;
}

std::uint32_t RepeatGameWindow::Destroy(
    const std::uint32_t object,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    return runtime.DestroyBase(object);
}

void RepeatGameWindow::SetVisible(
    const std::uint32_t object, const bool visible,
    const std::uint32_t parent,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    runtime.ProcessUiEvents(runtime.FrameManager(), !visible);
    runtime.SetParent(object, visible, parent);
    if (visible) (void)SetPosition(object, 0.0f, runtime);
    runtime.SetModal(runtime.ReadWord(object + 0x24u), visible);
}

std::uint32_t RepeatGameWindow::UpdatePosition(
    const std::uint32_t object, const double elapsed,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    if (!runtime.IsVisible(object)) return 0u;
    const auto progress = static_cast<float>(
        elapsed * 0.6666666865348816 +
        static_cast<double>(runtime.ReadFloat(object + 0x34u)));
    runtime.WriteFloat(object + 0x34u, progress);
    return SetPosition(object, progress, runtime);
}

std::uint32_t RepeatGameWindow::SetPosition(
    const std::uint32_t object, const float progress,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    const auto start = runtime.ReadFloat(object + 0x2cu);
    const auto end = runtime.ReadFloat(object + 0x30u);
    runtime.WriteFloat(object + 0x34u, progress);
    const auto x = static_cast<float>(
        (static_cast<double>(end) - start) * progress + start);
    runtime.SetXPosition(runtime.ReadWord(object + 0x28u), 1u, x);
    if (progress < 1.0f || std::isnan(progress)) return 0x100u;
    runtime.WriteFloat(object + 0x34u, 1.0f);
    return runtime.Close(object);
}

bool RepeatGameWindow::CheckPositionRange(
    const std::uint32_t object,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    const auto x = runtime.GetXPosition(
        runtime.ReadWord(object + 0x28u), 1u);
    return x >= 133.0f && x <= 153.0f;
}

std::uint8_t RepeatGameWindow::OnMouseClick(
    const std::uint32_t object, const std::uint32_t event,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    if (runtime.ReadWord(event + 0x1cu) == 0u)
        (void)runtime.Close(object);
    return 1u;
}

std::uint8_t RepeatGameWindow::OnKeyDown(
    const std::uint32_t object, const std::uint32_t event,
    IRepeatGameWindowLifecycleRuntime& runtime) {
    if (runtime.ReadWord(event + 0x10u) == 28u)
        (void)runtime.Close(object);
    return 1u;
}

}  // namespace pal4
