#pragma once

#include <cstdint>

namespace pal4 {

class IGlobalMemoryPoolRuntime {
public:
    virtual ~IGlobalMemoryPoolRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterExitHandler(
        std::uint32_t function_address) = 0;
};

class MemoryPoolInitializer final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t pool, std::uint32_t ignored,
        std::uint32_t requested_size, std::uint32_t alignment,
        IGlobalMemoryPoolRuntime& runtime);
};

class GlobalMemoryPool final {
public:
    static constexpr std::uint32_t kAddress = 0x8dd844u;
    static constexpr std::uint32_t kCleanupEntry = 0x403860u;

    [[nodiscard]] static std::uint32_t Initialize(
        IGlobalMemoryPoolRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeAndRegisterExit(
        IGlobalMemoryPoolRuntime& runtime);
    static void Cleanup(IGlobalMemoryPoolRuntime& runtime);
};

}  // namespace pal4
