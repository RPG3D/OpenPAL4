#pragma once

#include <cstdint>

namespace pal4 {

class IStackTraceEntryRuntime {
public:
    virtual ~IStackTraceEntryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GenerateStackTraceString(
        std::uint32_t first_arg, std::uint32_t exception_pointers) = 0;
    [[nodiscard]] virtual std::uint32_t GetCurrentProcess() = 0;
    [[nodiscard]] virtual std::uint32_t ReadProcessMemory(
        std::uint32_t process, std::uint32_t source,
        std::uint32_t destination, std::uint32_t bytes,
        std::uint32_t bytes_read_pointer) = 0;
};

class StackTraceEntryPoints final {
public:
    static constexpr std::uint32_t kFrameAddress = 0x8df7c0u;
    static constexpr std::uint32_t kFrameBytes = 0xa4u;

    [[nodiscard]] static std::uint32_t Prepare(
        std::uint32_t first_arg, std::uint32_t exception_pointers,
        IStackTraceEntryRuntime& runtime);
    [[nodiscard]] static std::uint32_t Forward(
        std::uint32_t first_arg, std::uint32_t exception_pointers,
        IStackTraceEntryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReadCurrentProcessMemory(
        std::uint32_t ignored_process, std::uint32_t source,
        std::uint32_t destination, std::uint32_t bytes,
        std::uint32_t bytes_read_pointer,
        IStackTraceEntryRuntime& runtime);
};

}  // namespace pal4
