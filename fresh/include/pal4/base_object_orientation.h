#pragma once

#include <cstdint>

#include "pal4/base_object_motion.h"

namespace pal4 {

class IBaseObjectOrientationRuntime {
public:
    virtual ~IBaseObjectOrientationRuntime() = default;
    virtual void NormalizeAndClamp(BaseObjectVector3&) {}
    [[nodiscard]] virtual double AngleFromCosine(double) { return 0.0; }
    virtual void SetPose(BaseObjectState&, const BaseObjectVector3&,
                         std::uint32_t) {}
    virtual void SetDirection(BaseObjectState&, const BaseObjectVector3&,
                              std::uint32_t) {}
};

class BaseObjectOrientation final {
public:
    // IDA 0x4440c0. The callers do not consume the decompiler's x87 result.
    static void Update(BaseObjectState& state,
                       const BaseObjectVector3& direction,
                       std::uint32_t unused0, std::uint32_t unused1,
                       const BaseObjectVector3& next_direction,
                       IBaseObjectOrientationRuntime& runtime);
};

}  // namespace pal4
