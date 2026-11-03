#pragma once

#include <array>
#include <cstdint>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

class IBaseObjectChildTransformRuntime {
public:
    virtual ~IBaseObjectChildTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::uint32_t GetObjectTransform(
        std::uint32_t) { return 0u; }
    [[nodiscard]] virtual std::array<float, 3> ReadVector3(
        std::uint32_t) { return {}; }
};

class BaseObjectChildTransform final {
public:
    // IDA 0x443f80; the original this pointer is base +0x1d8.
    [[nodiscard]] static double GetDoubleProperty(const BaseObjectState& state);
    // Same IDA443F80 getter with an explicit this adjustment. NPC behavior
    // calls it at object+0; the child-transform wrapper above uses +1D8.
    [[nodiscard]] static double GetDoublePropertyAtThis(
        const BaseObjectState& state,std::uint32_t this_offset);
    // IDA 0x443f90; returns the original 32-bit address of base +0x1e0.
    [[nodiscard]] static std::uint32_t Update(
        BaseObjectState& state, std::uint32_t object_address,
        IBaseObjectChildTransformRuntime& runtime);
    // IDA 0x443fe0.
    [[nodiscard]] static std::uint32_t GetParent(const BaseObjectState& state);
};

}  // namespace pal4
