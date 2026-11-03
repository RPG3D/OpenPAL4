#pragma once

#include <cstdint>

namespace pal4 {

class ISmithWindowQuantityRuntime {
public:
    virtual ~ISmithWindowQuantityRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class SmithWindowItemQuantity final {
public:
    // IDA 0x49a050.
    [[nodiscard]] static std::int32_t FirstPositive(
        std::uint32_t item, ISmithWindowQuantityRuntime& runtime);
};

}  // namespace pal4
