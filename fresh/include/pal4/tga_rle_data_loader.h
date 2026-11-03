#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

class ITgaRleRuntime {
public:
    virtual ~ITgaRleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadItem(
        std::uint32_t handle, std::span<std::uint8_t> buffer) = 0;
};

class TgaRleDataLoader final {
public:
    // IDA 0x5f9570. Zero means a complete row; one means null output
    // or a short file read.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t stream, std::uint32_t destination,
        ITgaRleRuntime& runtime);
};

}  // namespace pal4
