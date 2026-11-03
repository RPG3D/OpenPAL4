#pragma once

#include "pal4/base_object_lifecycle.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IBaseObjectSceneParameterRuntime {
public:
    virtual ~IBaseObjectSceneParameterRuntime() = default;
    [[nodiscard]] virtual std::array<float, 3> ClumpPosition(
        std::uint32_t clump) = 0;
};

class BaseObjectSceneParameter final {
public:
    static constexpr std::uint32_t kOffset = 0x1ecu;

    // IDA 0x443f90: refresh the scene parameter returned by vtable +4.
    [[nodiscard]] static std::array<float, 3> Refresh(
        BaseObjectState& actor, IBaseObjectSceneParameterRuntime& runtime);
};

}  // namespace pal4
