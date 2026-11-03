#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IPuzzleWindowCounterRuntime {
public:
    virtual ~IPuzzleWindowCounterRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(
        std::uint32_t address) = 0;
    virtual void WriteSignedWord(std::uint32_t address,
                                 std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetTextColours(
        std::uint32_t window,
        const std::array<float, 4>& colour) = 0;
    virtual void PlaySound(std::string_view file,
                           std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::optional<std::string_view> TriggerFormat(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string FormatText(
        std::string_view format, std::int32_t value) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
    virtual void SetText(std::uint32_t window,
                         std::u32string_view text) = 0;
    virtual void ShowResult(std::uint32_t frame,
                            std::uint32_t success) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseFormatted(
        std::string& text) = 0;
};

class PuzzleWindowCounter final {
public:
    // IDA 0x4a14c0.
    [[nodiscard]] static std::uint8_t UpdateDisplay(
        std::uint32_t frame, IPuzzleWindowCounterRuntime& runtime);
    // IDA 0x4a1710.
    [[nodiscard]] static std::uint8_t Decrement(
        std::uint32_t frame, IPuzzleWindowCounterRuntime& runtime);
};

}  // namespace pal4
