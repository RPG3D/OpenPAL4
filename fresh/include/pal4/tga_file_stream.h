#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ITgaFileStreamRuntime {
public:
    virtual ~ITgaFileStreamRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t OpenHandle(
        std::uint32_t filename, std::uint32_t mode) = 0;
    virtual void CloseHandle(std::uint32_t handle) = 0;
    virtual void SeekHandle(std::uint32_t handle, std::int32_t offset,
                            std::int32_t origin) = 0;
    [[nodiscard]] virtual std::uint32_t Position(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t ReadHandle(
        std::uint32_t buffer, std::uint32_t size, std::uint32_t count,
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void NotifyError(std::uint32_t stream, std::uint32_t code) = 0;
    virtual void LogError(std::string_view file, std::uint32_t line,
                          std::string_view message) = 0;
};

class TgaFileStream final {
public:
    // IDA 0x5f8be0.
    [[nodiscard]] static std::uint32_t Open(
        std::uint32_t filename, std::uint32_t mode,
        ITgaFileStreamRuntime& runtime);
    // IDA 0x5f8c90.
    [[nodiscard]] static std::uint32_t Close(
        std::uint32_t stream, ITgaFileStreamRuntime& runtime);
    // IDA 0x5f8cc0; the argument is one byte in the original ABI.
    [[nodiscard]] static std::string_view ErrorMessage(std::uint8_t code);
    // IDA 0x5f8ce0 / 0x5f8d50.
    [[nodiscard]] static std::uint32_t Seek(
        std::uint32_t stream, std::int32_t offset, std::int32_t origin,
        ITgaFileStreamRuntime& runtime);
    [[nodiscard]] static std::uint32_t Read(
        std::uint32_t stream, std::uint32_t buffer, std::uint32_t size,
        std::uint32_t count, ITgaFileStreamRuntime& runtime);
    // IDA 0x5f8d10. limit is a byte offset, not an element count.
    static void SwapBytes(std::uint32_t data, std::uint32_t limit,
                          std::uint32_t stride,
                          ITgaFileStreamRuntime& runtime);
};

}  // namespace pal4
