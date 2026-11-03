#pragma once

#include <cstdint>
#include <optional>

#include "pal4/ui_text_image_overlay.h"

namespace pal4 {

class UiTextImageAnimation final {
public:
    static constexpr std::uint32_t kVtable = 0x844450u;
    static constexpr std::uint32_t kDelayedVtable = 0x844460u;
    static constexpr std::uint32_t kMotionVtable = 0x844470u;
    static constexpr std::uint32_t kPulseVtable = 0x844480u;

    // IDA 0x4c2a40 and 0x4c2a90.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t seed,
        IUiTextImageOverlayRuntime& runtime);
    static void ResetTiming(std::uint32_t object,
                            IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2ab0.
    [[nodiscard]] static std::uint32_t SetTiming(
        std::uint32_t object, std::uint32_t start_bits,
        std::uint32_t end_bits, IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2ad0. Returns the masked x87 comparison status.
    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t object, float delta,
        IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2b00.
    [[nodiscard]] static bool IsFinished(
        std::uint32_t object, IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2b20 and 0x4c2bc0.
    [[nodiscard]] static std::uint32_t ConstructDelayed(
        std::uint32_t object, std::uint8_t seed,
        IUiTextImageOverlayRuntime& runtime);
    [[nodiscard]] static std::uint32_t ConstructMotion(
        std::uint32_t object, std::uint8_t seed,
        IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2b90. The no-update path leaves EAX unspecified.
    [[nodiscard]] static std::optional<std::uint32_t> AdvanceDelayed(
        std::uint32_t object, float delta,
        IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2be0.
    [[nodiscard]] static std::uint32_t AdvanceMotion(
        std::uint32_t object, float delta,
        IUiTextImageOverlayRuntime& runtime);
    // IDA 0x4c2c00, 0x4c2c30 and 0x4c2c60.
    [[nodiscard]] static std::uint32_t ConstructPulse(
        std::uint32_t object, std::uint8_t seed,
        IUiTextImageOverlayRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPulseTiming(
        std::uint32_t object, std::uint32_t grow_rate_bits,
        std::uint32_t grow_duration_bits,
        std::uint32_t shrink_rate_bits,
        std::uint32_t shrink_duration_bits,
        IUiTextImageOverlayRuntime& runtime);
    // Returns either the x87 comparison mask or the synchronized scale bits.
    [[nodiscard]] static std::uint32_t AdvancePulse(
        std::uint32_t object, float delta,
        IUiTextImageOverlayRuntime& runtime);
};

}  // namespace pal4
