#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

struct JigsawResultOverlaySpec {
    std::uint32_t width_bits{};
    std::uint32_t height_bits{};
    std::string_view imageset;
    std::string_view image;
    std::uint32_t image_scale_bits{};
    std::uint32_t font_field_bits{};
    std::string_view font_name;
    std::string_view font_group;
    std::uint32_t font_size_bits{};
    std::uint32_t font_x_bits{};
    std::uint32_t font_y_bits{};
};

class IJigsawWindowResultRuntime {
public:
    virtual ~IJigsawWindowResultRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadCounter(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::optional<std::string_view> TriggerFormat(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string FormatText(
        std::string_view format, std::int32_t value) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void PlaySound(std::string_view file,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t CreateOverlay(
        std::uint32_t bytes, std::uint32_t vtable) = 0;
    virtual void ConfigureOverlay(std::uint32_t overlay,
                                  const JigsawResultOverlaySpec& spec) = 0;
    virtual void AddOverlay(std::uint32_t list,
                            std::uint8_t active,
                            std::uint32_t overlay) = 0;
    virtual void ReleaseOverlay(std::uint32_t overlay) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ReleaseCallback(std::uint32_t callback) = 0;
    virtual void QueueCloseEvent(
        std::uint32_t queue, std::string_view name,
        std::uint32_t period_bits, std::uint32_t callback,
        bool owns_callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseFormatted(
        std::string& text) = 0;
};

class JigsawWindowResult final {
public:
    static constexpr std::uint32_t kOverlayVtable = 0x843e84u;
    static constexpr std::uint32_t kCallbackVtable = 0x843e7cu;
    static constexpr std::uint32_t kCloseFunction = 0x4a09c0u;

    // IDA 0x4a04b0.
    [[nodiscard]] static std::uint32_t Show(
        std::uint32_t frame, std::uint8_t success,
        IJigsawWindowResultRuntime& runtime);
};

using IPuzzleWindowResultRuntime = IJigsawWindowResultRuntime;

class PuzzleWindowResult final {
public:
    static constexpr std::uint32_t kCallbackVtable = 0x843eccu;

    // IDA 0x4a1730.
    [[nodiscard]] static std::uint32_t Show(
        std::uint32_t frame, std::uint8_t success,
        IPuzzleWindowResultRuntime& runtime);
};

class IJigsawResultOverlayRuntime {
public:
    virtual ~IJigsawResultOverlayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateAnimationProgress(
        float delta) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    // The original fnstsw overwrites AX after an x87 comparison with one.
    [[nodiscard]] virtual std::uint16_t CompareStatusWord(
        double value) = 0;
};

class JigsawResultOverlay final {
public:
    // IDA 0x4a09d0, virtual update on the 0x843e84 overlay.
    [[nodiscard]] static std::uint32_t UpdateAnimation(
        std::uint32_t overlay, float delta,
        IJigsawResultOverlayRuntime& runtime);
};

}  // namespace pal4
