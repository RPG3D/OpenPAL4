#pragma once

#include <cstdint>

namespace pal4 {

class IExceptionHandlerPointerArrayRuntime {
public:
    virtual ~IExceptionHandlerPointerArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CopyDwords(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t count) = 0;
    [[nodiscard]] virtual std::uint32_t GetProcessHeap() = 0;
    [[nodiscard]] virtual std::uint32_t HeapAllocate(
        std::uint32_t heap, std::uint32_t flags,
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t HeapFree(
        std::uint32_t heap, std::uint32_t flags,
        std::uint32_t address) = 0;
};

class ExceptionHandlerPointerArray final {
public:
    static constexpr std::uint32_t kBuffer = 0x8df86cu;
    static constexpr std::uint32_t kCount = 0x8df870u;

    [[nodiscard]] static std::uint32_t Append(
        std::uint32_t element, IExceptionHandlerPointerArrayRuntime& runtime);
};

}  // namespace pal4
