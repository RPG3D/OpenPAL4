#include "pal4/ui_text_image_animation.h"

#include <bit>
#include <cmath>

namespace pal4 {

std::uint32_t UiTextImageAnimation::Construct(
    const std::uint32_t object, const std::uint8_t seed,
    IUiTextImageOverlayRuntime& runtime) {
    (void)UiTextImageOverlay::Construct(object, seed, runtime);
    runtime.WriteWord(object, kVtable);
    ResetTiming(object, runtime);
    return object;
}

void UiTextImageAnimation::ResetTiming(
    const std::uint32_t object, IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteWord(object + 64u, 0u);
    runtime.WriteWord(object + 56u, 0x3f800000u);
    runtime.WriteWord(object + 60u, 0x40000000u);
}

std::uint32_t UiTextImageAnimation::SetTiming(
    const std::uint32_t object, const std::uint32_t start_bits,
    const std::uint32_t end_bits, IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteWord(object + 56u, start_bits);
    runtime.WriteWord(object + 60u, end_bits);
    return start_bits;
}

std::uint32_t UiTextImageAnimation::Advance(
    const std::uint32_t object, const float delta,
    IUiTextImageOverlayRuntime& runtime) {
    const double progress =
        static_cast<double>(delta) +
        std::bit_cast<float>(runtime.ReadWord(object + 64u));
    const float start = std::bit_cast<float>(runtime.ReadWord(object + 56u));
    runtime.WriteWord(object + 64u,
                      std::bit_cast<std::uint32_t>(
                          static_cast<float>(progress)));
    if (std::isnan(progress) || std::isnan(start)) return 0x4100u;
    if (progress < start) return 0x100u;
    if (progress == start) return 0x4000u;

    const float end = std::bit_cast<float>(runtime.ReadWord(object + 60u));
    const float alpha = static_cast<float>(
        (static_cast<double>(end) - progress) /
        (static_cast<double>(end) - static_cast<double>(start)));
    runtime.WriteWord(object + 44u, std::bit_cast<std::uint32_t>(alpha));
    return 0u;
}

bool UiTextImageAnimation::IsFinished(
    const std::uint32_t object, IUiTextImageOverlayRuntime& runtime) {
    const float progress =
        std::bit_cast<float>(runtime.ReadWord(object + 64u));
    const float end = std::bit_cast<float>(runtime.ReadWord(object + 60u));
    return !std::isnan(progress) && !std::isnan(end) &&
           !(progress < end);
}

std::uint32_t UiTextImageAnimation::ConstructDelayed(
    const std::uint32_t object, const std::uint8_t seed,
    IUiTextImageOverlayRuntime& runtime) {
    (void)Construct(object, seed, runtime);
    runtime.WriteWord(object, kDelayedVtable);
    runtime.WriteWord(object + 68u, 0u);
    return object;
}

std::uint32_t UiTextImageAnimation::ConstructMotion(
    const std::uint32_t object, const std::uint8_t seed,
    IUiTextImageOverlayRuntime& runtime) {
    (void)Construct(object, seed, runtime);
    runtime.WriteWord(object, kMotionVtable);
    runtime.WriteWord(object + 68u, 0x3f800000u);
    return object;
}

std::optional<std::uint32_t> UiTextImageAnimation::AdvanceDelayed(
    const std::uint32_t object, const float delta,
    IUiTextImageOverlayRuntime& runtime) {
    const double delay =
        std::bit_cast<float>(runtime.ReadWord(object + 68u)) -
        static_cast<double>(delta);
    runtime.WriteWord(object + 68u,
                      std::bit_cast<std::uint32_t>(
                          static_cast<float>(delay)));
    if (!(delay < 0.0)) return std::nullopt;
    return Advance(object, delta, runtime);
}

std::uint32_t UiTextImageAnimation::AdvanceMotion(
    const std::uint32_t object, const float delta,
    IUiTextImageOverlayRuntime& runtime) {
    const std::uint32_t result = Advance(object, delta, runtime);
    const float speed = std::bit_cast<float>(runtime.ReadWord(object + 68u));
    const float y = std::bit_cast<float>(runtime.ReadWord(object + 32u));
    runtime.WriteWord(object + 32u,
                      std::bit_cast<std::uint32_t>(y - delta * speed));
    return result;
}

std::uint32_t UiTextImageAnimation::ConstructPulse(
    const std::uint32_t object, const std::uint8_t seed,
    IUiTextImageOverlayRuntime& runtime) {
    (void)Construct(object, seed, runtime);
    runtime.WriteWord(object, kPulseVtable);
    runtime.WriteWord(object + 84u, 0u);
    runtime.WriteWord(object + 76u, 0u);
    runtime.WriteWord(object + 80u, 0u);
    runtime.WriteWord(object + 68u, 0u);
    runtime.WriteWord(object + 72u, 0u);
    return object;
}

std::uint32_t UiTextImageAnimation::SetPulseTiming(
    const std::uint32_t object, const std::uint32_t grow_rate_bits,
    const std::uint32_t grow_duration_bits,
    const std::uint32_t shrink_rate_bits,
    const std::uint32_t shrink_duration_bits,
    IUiTextImageOverlayRuntime& runtime) {
    runtime.WriteWord(object + 68u, grow_rate_bits);
    runtime.WriteWord(object + 76u, grow_duration_bits);
    runtime.WriteWord(object + 72u, shrink_rate_bits);
    runtime.WriteWord(object + 80u, shrink_duration_bits);
    runtime.WriteWord(object + 84u, 0u);
    return shrink_rate_bits;
}

std::uint32_t UiTextImageAnimation::AdvancePulse(
    const std::uint32_t object, const float delta,
    IUiTextImageOverlayRuntime& runtime) {
    (void)Advance(object, delta, runtime);
    const float timer = std::bit_cast<float>(runtime.ReadWord(object + 84u));
    if (std::isnan(timer) || timer < 0.0f) {
        const double new_timer = static_cast<double>(timer) - delta;
        runtime.WriteWord(object + 84u,
                          std::bit_cast<std::uint32_t>(
                              static_cast<float>(new_timer)));
        const float rate =
            std::bit_cast<float>(runtime.ReadWord(object + 72u));
        const float scale =
            std::bit_cast<float>(runtime.ReadWord(object + 40u));
        const float new_scale = scale - delta * rate;
        const std::uint32_t scale_bits =
            std::bit_cast<std::uint32_t>(new_scale);
        runtime.WriteWord(object + 40u, scale_bits);
        const float duration =
            std::bit_cast<float>(runtime.ReadWord(object + 80u));
        const double magnitude = -new_timer;
        std::uint32_t result = 0u;
        if (std::isnan(magnitude) || std::isnan(duration))
            result = 0x4100u;
        else if (magnitude < duration)
            result = 0x100u;
        else if (magnitude == duration)
            result = 0x4000u;
        if (result == 0u) {
            runtime.WriteWord(object + 84u, 0u);
            result = scale_bits;
        }
        runtime.WriteWord(object + 36u, scale_bits);
        return result;
    }

    const double new_timer = static_cast<double>(timer) + delta;
    runtime.WriteWord(object + 84u,
                      std::bit_cast<std::uint32_t>(
                          static_cast<float>(new_timer)));
    const float rate = std::bit_cast<float>(runtime.ReadWord(object + 68u));
    const float scale = std::bit_cast<float>(runtime.ReadWord(object + 40u));
    const float new_scale = delta * rate + scale;
    const std::uint32_t scale_bits = std::bit_cast<std::uint32_t>(new_scale);
    runtime.WriteWord(object + 40u, scale_bits);
    const float duration =
        std::bit_cast<float>(runtime.ReadWord(object + 76u));
    std::uint32_t status = 0u;
    if (std::isnan(new_timer) || std::isnan(duration))
        status = 0x4100u;
    else if (new_timer < duration)
        status = 0x100u;
    else if (new_timer == duration)
        status = 0x4000u;
    if (status == 0u) {
        runtime.WriteWord(object + 84u, 0xb8d1b717u);
        runtime.WriteWord(object + 36u, scale_bits);
        return 0u;
    }
    runtime.WriteWord(object + 36u, scale_bits);
    return scale_bits;
}

}  // namespace pal4
