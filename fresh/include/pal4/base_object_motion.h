#pragma once

#include <cstdint>

#include "pal4/base_object_lifecycle.h"

namespace pal4 {

struct BaseObjectVector3 {
    float x{};
    float y{};
    float z{};
};

class IBaseObjectMotionRuntime {
public:
    virtual ~IBaseObjectMotionRuntime() = default;
    virtual void UpdateAndRenderClump(float) {}
    virtual void UpdateInterpolator(BaseObjectState&, std::uint32_t,
                                    float) {}
    virtual void UpdateCharacter(BaseObjectState&, std::uint32_t,
                                 std::uint32_t, float, float) {}
    virtual void NormalizeAndClamp(BaseObjectVector3&) {}
    [[nodiscard]] virtual double AngleFromCosine(float) { return 0.0; }
    virtual void SetPose(BaseObjectState&, const BaseObjectVector3&,
                         std::uint32_t) {}
    [[nodiscard]] virtual std::uint32_t SetDirection(
        BaseObjectState&, const BaseObjectVector3&) { return 0u; }
};

class BaseObjectMotion final {
public:
    // Shared transform stage after animation and CollisionActor have advanced.
    [[nodiscard]] static std::uint32_t ApplyTransform(BaseObjectState& state,
        std::uint32_t address,IBaseObjectMotionRuntime& runtime);
    // IDA 0x442970.
    [[nodiscard]] static std::uint32_t UpdatePosition(
        BaseObjectState& state, std::uint32_t address,
        float elapsed, std::uint32_t argument,
        IBaseObjectMotionRuntime& runtime);
};

}  // namespace pal4
