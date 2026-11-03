#include "pal4/click_game_window_lifecycle.h"

namespace pal4 {
namespace {

constexpr float kProgressStep = 0.027777778f;

}  // namespace

std::uint32_t ClickGameWindowLifecycle::Destruct(
    const std::uint32_t object,
    IClickGameWindowLifecycleRuntime& runtime) {
    runtime.WriteWord(object, 0x843e20u);
    return runtime.DestroyBase(object);
}

void ClickGameWindowLifecycle::SetModal(
    const std::uint32_t object, const std::uint8_t modal,
    const std::uint32_t parent,
    IClickGameWindowLifecycleRuntime& runtime) {
    runtime.ProcessUIEvents(runtime.FrameManager(), modal == 0u);
    runtime.SetParent(object, modal, parent);
    (void)UpdateProgress(object, 0.0f, runtime);
    const auto root = runtime.ReadWord(object + 0x24u);
    runtime.WriteWord(object + 0x34u, 0u);
    runtime.SetModalState(root, modal);
}

std::uint32_t ClickGameWindowLifecycle::UpdateTimer(
    const std::uint32_t object, const double elapsed,
    IClickGameWindowLifecycleRuntime& runtime) {
    if (runtime.IsVisible(object) == 0u) return 0u;
    const auto timer = static_cast<double>(
        runtime.ReadFloat(object + 0x34u)) + elapsed;
    runtime.WriteFloat(object + 0x34u, static_cast<float>(timer));
    if (timer >= 30.0) return runtime.Close(object, 0u, 0u);
    return 0x100u;
}

std::uint32_t ClickGameWindowLifecycle::UpdateProgress(
    const std::uint32_t object, const float progress,
    IClickGameWindowLifecycleRuntime& runtime) {
    const auto width = static_cast<float>(
        static_cast<double>(progress) *
        static_cast<double>(runtime.ReadFloat(object + 0x2cu)));
    const auto image_bar = runtime.ReadWord(object + 0x28u);
    runtime.WriteFloat(object + 0x30u, progress);
    runtime.SetWidth(image_bar, width);
    if (progress >= 1.0f) return runtime.Close(object, 0u, 0u);
    return 0x100u;
}

std::uint8_t ClickGameWindowLifecycle::OnProgressClick(
    const std::uint32_t object, const std::uint32_t event,
    IClickGameWindowLifecycleRuntime& runtime) {
    if (runtime.ReadWord(event + 0x1cu) == 0u) {
        const auto progress = runtime.ReadFloat(object + 0x30u) +
                              kProgressStep;
        runtime.WriteFloat(object + 0x30u, progress);
        (void)UpdateProgress(object, progress, runtime);
    }
    return 1u;
}

std::uint8_t ClickGameWindowLifecycle::OnKeyUp(
    const std::uint32_t object, const std::uint32_t event,
    IClickGameWindowLifecycleRuntime& runtime) {
    if (runtime.ReadWord(event + 0x10u) == 28u) {
        const auto progress = runtime.ReadFloat(object + 0x30u) +
                              kProgressStep;
        runtime.WriteFloat(object + 0x30u, progress);
        (void)UpdateProgress(object, progress, runtime);
    }
    return 1u;
}

}  // namespace pal4
