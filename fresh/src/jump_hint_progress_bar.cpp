#include "pal4/jump_hint_progress_bar.h"

namespace pal4 {

std::uint32_t JumpHintProgressBar::Construct(
    const std::uint32_t object, IJumpHintProgressRuntime& runtime) {
    runtime.LoadLayout(object, "JumpHintWindow");
    runtime.WriteWord(object, kVtable);
    const auto image_name = runtime.ReadName(object + 20u) + "ImgProgess";
    const auto image = runtime.GetWindow(object, image_name);
    runtime.WriteWord(object + 40u, image);
    runtime.WriteFloat(object + 44u, runtime.GetX(image));
    runtime.WriteFloat(object + 48u, runtime.GetWidth(image));
    (void)SetProgress(object, 0.0f, runtime);
    runtime.SetAlwaysOnTop(runtime.ReadWord(object + 36u), true);
    return object;
}

std::uint32_t JumpHintProgressBar::Destruct(
    const std::uint32_t object, IJumpHintProgressRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    return runtime.DestructBase(object);
}

std::uint32_t JumpHintProgressBar::SetVisible(
    const std::uint32_t object, const std::uint32_t visible,
    IJumpHintProgressDisplayRuntime& runtime) {
    const auto result = runtime.SetVisibleBase(object, visible, 0u);
    if ((visible & 0xffu) != 0u)
        return SetProgress(object, 0.0f, runtime);
    return result;
}

std::uint32_t JumpHintProgressBar::Update(
    const std::uint32_t object, const float delta,
    IJumpHintProgressDisplayRuntime& runtime) {
    const auto visible = runtime.IsVisible(object);
    if ((visible & 0xffu) == 0u) return visible;
    const auto value = static_cast<double>(GetProgress(object, runtime)) +
        static_cast<double>(delta) *
            static_cast<double>(0.40000001f);
    const auto progress = value > 1.0 ? 0.0f : static_cast<float>(value);
    return SetProgress(object, progress, runtime);
}

std::uint32_t JumpHintProgressBar::SetProgress(
    const std::uint32_t object, const float progress,
    IJumpHintProgressDisplayRuntime& runtime) {
    const auto image = runtime.ReadWord(object + 40u);
    const auto width = runtime.ReadFloat(object + 48u);
    runtime.WriteFloat(object + 52u, progress);
    const auto filled_width = static_cast<float>(
        static_cast<double>(progress) * static_cast<double>(width));
    runtime.SetWidth(image, width - filled_width);
    return runtime.SetX(image, filled_width + runtime.ReadFloat(object + 44u));
}

float JumpHintProgressBar::GetProgress(
    const std::uint32_t object, IJumpHintProgressDisplayRuntime& runtime) {
    return runtime.ReadFloat(object + 52u);
}

}  // namespace pal4
