#pragma once

#include <cstdint>

namespace pal4 {

class IExceptionHandlerRuntime {
public:
    virtual ~IExceptionHandlerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t GetProcessHeap() = 0;
    [[nodiscard]] virtual std::uint32_t HeapFree(
        std::uint32_t heap, std::uint32_t flags,
        std::uint32_t address) = 0;
    virtual void SetUnhandledExceptionFilter(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterExitHandler(
        std::uint32_t function_address) = 0;
};

class ExceptionHandlerLifecycle final {
public:
    static constexpr std::uint32_t kPreviousFilter = 0x8df868u;
    static constexpr std::uint32_t kBuffer = 0x8df86cu;
    static constexpr std::uint32_t kCleanupEntry = 0x403a80u;

    static void InitializeEmpty();
    [[nodiscard]] static std::uint32_t InitializeAndRegisterExit(
        IExceptionHandlerRuntime& runtime);
    static void Cleanup(IExceptionHandlerRuntime& runtime);
};

}  // namespace pal4
