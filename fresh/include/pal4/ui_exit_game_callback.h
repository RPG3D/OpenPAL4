#pragma once

#include <cstdint>

namespace pal4 {

class IUiExitGameCallbackRuntime {
public:
    virtual ~IUiExitGameCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(
        std::uint32_t size) = 0;
};

class UiExitGameCallback final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x843500u;
    static constexpr std::uint32_t kVtable = 0x844440u;

    // IDA 0x4c1870.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t self, std::uint32_t command,
        std::uint32_t first, std::uint32_t second,
        IUiExitGameCallbackRuntime& runtime);

    // IDA 0x4c1890.
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t self, IUiExitGameCallbackRuntime& runtime);
};

}  // namespace pal4
