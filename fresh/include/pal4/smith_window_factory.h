#pragma once

#include <cstdint>

namespace pal4 {

class ISmithWindowFactoryRuntime {
public:
    virtual ~ISmithWindowFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Construct(
        std::uint32_t object, std::uint32_t layout_name) = 0;
    virtual void Free(std::uint32_t object) = 0;
};

class SmithWindowFactory final {
public:
    // IDA 0x496ae0.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout_name, ISmithWindowFactoryRuntime& runtime);
};

}  // namespace pal4
