#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IObjectClumpCollisionRuntime {
public:
    virtual ~IObjectClumpCollisionRuntime() = default;
    [[nodiscard]] virtual std::int32_t ProcessClumpCollision(
        std::uint32_t vector_address, std::uint32_t clump_address,
        std::array<std::uint8_t, 24>& scratch) = 0;
};

class ObjectClumpCollisionRuntimeStub final
    : public IObjectClumpCollisionRuntime {
public:
    std::int32_t ProcessClumpCollision(
        std::uint32_t vector_address, std::uint32_t clump_address,
        std::array<std::uint8_t, 24>& scratch) override;
};

class ObjectClumpCollision final {
public:
    // IDA 0x433650: the second argument gates the external collision call.
    [[nodiscard]] static std::int32_t ProcessConditional(
        std::uint32_t vector_address, std::uint32_t clump_address,
        IObjectClumpCollisionRuntime& runtime);
};

}  // namespace pal4
