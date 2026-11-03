#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pal4 {

struct NumberedGameRecord {
    std::int32_t threshold;
    std::string first_text;
    std::string second_text;
};

enum class NumberedTextEncoding { RawBytes, Utf8 };

// Game data and icon services borrowed by the native DLL rendering boundary.
class INumberedListRenderDataRuntime {
public:
    virtual ~INumberedListRenderDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetIconImage(
        std::uint32_t slot, std::uint32_t category, std::uint32_t variant) = 0;
    [[nodiscard]] virtual std::string FormatItemCount(
        std::uint32_t trigger, std::uint32_t count) = 0;
    [[nodiscard]] virtual std::vector<NumberedGameRecord> GetGameRecords() = 0;
    [[nodiscard]] virtual bool TriggerObject(std::uint32_t id) = 0;
    virtual void TriggerPlayer(std::uint32_t id) = 0;
};

class INumberedListRenderRuntime : public INumberedListRenderDataRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadCString(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t GetSystemFont() = 0;
    [[nodiscard]] virtual std::uint32_t ModulatedBlack(
        std::uint32_t item, float modulation) = 0;
    virtual void CacheWhiteImage(
        std::uint32_t cache, std::uint32_t image,
        const std::array<float, 4>& rect, float alpha,
        const std::array<float, 4>& clip,
        bool first_flag, bool second_flag) = 0;
    virtual void CacheItemText(
        std::uint32_t cache, std::uint32_t text_address,
        std::uint32_t font, std::uint32_t format,
        const std::array<float, 4>& rect, float alpha,
        const std::array<float, 4>& clip,
        std::uint32_t modulated_colour) = 0;
    virtual void CacheBlackText(
        std::uint32_t cache, std::string_view text,
        NumberedTextEncoding encoding, std::uint32_t font,
        std::uint32_t format, const std::array<float, 4>& rect,
        float alpha, const std::array<float, 4>& clip) = 0;
    [[nodiscard]] virtual std::array<float, 4> IntersectRect(
        const std::array<float, 4>& rect,
        const std::array<float, 4>& clip) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view text) = 0;
};

}  // namespace pal4
