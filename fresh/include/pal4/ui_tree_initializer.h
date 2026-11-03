#pragma once

#include <cstdint>

namespace pal4 {

class IUiTreeInitializerRuntime {
public:
    virtual ~IUiTreeInitializerRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void InitializeTree(std::uint32_t tree) = 0;
};

class UiTreeInitializer final {
public:
    // IDA 0x592570.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t tree, std::uint32_t first_byte_source,
        std::uint8_t flag, std::uint32_t root_byte_source,
        IUiTreeInitializerRuntime& runtime);
};

}  // namespace pal4
