#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITgaHeaderRuntime {
public:
    virtual ~ITgaHeaderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t LoadImage(std::uint32_t stream) = 0;
    [[nodiscard]] virtual std::uint32_t LoadColorMap(
        std::uint32_t stream, std::uint32_t metadata) = 0;
    [[nodiscard]] virtual std::uint32_t LoadImageData(
        std::uint32_t stream, std::uint32_t color_map_slot,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t LoadRows(
        std::uint32_t stream, std::uint32_t pixels,
        std::uint32_t first_row, std::uint32_t row_count,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual bool HasErrorCallback(std::uint32_t stream) = 0;
    virtual void NotifyError(std::uint32_t stream, std::uint32_t code) = 0;
    virtual void LogError(std::string_view file, std::uint32_t line,
                          std::string_view message) = 0;
};

class TgaHeaderLoader final {
public:
    // IDA 0x5f8d90. Native return is always zero; inspect metadata flags.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t stream, std::uint32_t metadata,
        ITgaHeaderRuntime& runtime);
};

}  // namespace pal4
