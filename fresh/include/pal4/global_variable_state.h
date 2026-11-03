#pragma once

#include <cstdint>

namespace pal4 {

class IGlobalVariableStateRuntime {
public:
    virtual ~IGlobalVariableStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RegisterExitHandler(
        std::uint32_t function_address) = 0;
};

class GlobalVariableState final {
public:
    static constexpr std::uint32_t kAddress = 0x8dec90u;
    static constexpr std::uint32_t kCleanupEntry = 0x4039d0u;

    [[nodiscard]] static std::uint32_t Setup(
        std::uint32_t entry_ecx, IGlobalVariableStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeAndRegisterExit(
        std::uint32_t entry_ecx, IGlobalVariableStateRuntime& runtime);
    static void Cleanup(IGlobalVariableStateRuntime& runtime);
};

}  // namespace pal4
