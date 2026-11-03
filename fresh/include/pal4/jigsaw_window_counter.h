#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IJigsawWindowCounterRuntime {
public:
    virtual ~IJigsawWindowCounterRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t ReadSignedWord(std::uint32_t address) = 0;
    virtual void WriteSignedWord(std::uint32_t address,
                                 std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
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

class JigsawWindowCounter final {
public:
    // IDA 0x4a02b0.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t frame, IJigsawWindowCounterRuntime& runtime);
    // IDA 0x4a0490; the tail jump preserves Update's low return byte.
    [[nodiscard]] static std::uint8_t Decrement(
        std::uint32_t frame, IJigsawWindowCounterRuntime& runtime);
};

}  // namespace pal4
