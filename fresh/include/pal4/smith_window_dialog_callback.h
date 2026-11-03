#pragma once

#include <cstdint>

namespace pal4 {

class ISmithWindowDialogCallbackRuntime {
public:
    virtual ~ISmithWindowDialogCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class SmithWindowDialogCallback final {
public:
    static constexpr std::uint32_t kSize = 0x18u;
    static constexpr std::uint32_t kVtable = 0x843e0cu;
    static constexpr std::uint32_t kBaseVtable = 0x8435d0u;

    // IDA 0x49f1c0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t handler,
        std::uint32_t argument, std::uint32_t owner,
        ISmithWindowDialogCallbackRuntime& runtime);
    // IDA 0x49f1e0.
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source,
        ISmithWindowDialogCallbackRuntime& runtime);
};

}  // namespace pal4
