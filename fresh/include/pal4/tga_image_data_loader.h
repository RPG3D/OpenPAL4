#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITgaImageDataRuntime {
public:
    virtual ~ITgaImageDataRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void Seek(std::uint32_t stream, std::int32_t offset,
                      std::int32_t origin) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Reallocate(
        std::uint32_t address, std::uint32_t size,
        std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Read(
        std::uint32_t stream, std::uint32_t buffer,
        std::uint32_t size, std::uint32_t count) = 0;
    virtual void SwapBytes(std::uint32_t data, std::uint32_t limit,
                           std::uint32_t stride) = 0;
    [[nodiscard]] virtual bool HasErrorCallback(std::uint32_t stream) = 0;
    virtual void NotifyError(std::uint32_t stream, std::uint32_t code) = 0;
    virtual void LogError(std::string_view file, std::uint32_t line,
                          std::string_view message) = 0;
};

class TgaImageDataLoader final {
public:
    // IDA 0x5f9370. The positive 15/16-bit realloc path is nonterminating
    // in the native binary and is translated to a C++ exception.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t stream, std::uint32_t data_slot,
        std::uint32_t flags, ITgaImageDataRuntime& runtime);
};

}  // namespace pal4
