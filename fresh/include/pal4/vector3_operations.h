#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <cstdint>

namespace pal4 {

class IVectorRandomSource {
public:
    virtual ~IVectorRandomSource() = default;
    [[nodiscard]] virtual std::int32_t Rand() = 0;
};

class VectorRandomSourceStub final : public IVectorRandomSource {
public:
    std::int32_t Rand() override;
};

class Vector3Operations final {
public:
    static BehaviorVector3& SetXYZBits(
        BehaviorVector3& output, std::uint32_t x,
        std::uint32_t y, std::uint32_t z);
    static BehaviorVector3& Add(const BehaviorVector3& first,
                                BehaviorVector3& output,
                                const BehaviorVector3& second);
    static BehaviorVector3& Subtract(const BehaviorVector3& first,
                                     BehaviorVector3& output,
                                     const BehaviorVector3& second);
    static BehaviorVector3& Scale(const BehaviorVector3& input,
                                  BehaviorVector3& output, float factor);
    static BehaviorVector3& Normalize(const BehaviorVector3& input,
                                      BehaviorVector3& output);
    static BehaviorVector3& NormalizeByLength(
        const BehaviorVector3& input, BehaviorVector3& output,
        float length);
    static BehaviorVector3& CopyThrough(BehaviorVector3& intermediate,
                                        BehaviorVector3& output,
                                        const BehaviorVector3& input);
    static BehaviorVector3& AddInPlaceAndCopy(
        BehaviorVector3& value, BehaviorVector3& output,
        const BehaviorVector3& added);
    static BehaviorVector3& ScaleInPlaceAndCopy(
        BehaviorVector3& value, BehaviorVector3& output,
        const float& factor);
    [[nodiscard]] static bool Equals(const BehaviorVector3& first,
                                      const BehaviorVector3& second);
    static BehaviorVector3& SubtractProjection(
        const BehaviorVector3& value, BehaviorVector3& output,
        const BehaviorVector3& axis);
    static BehaviorVector3& CopyXZTo2D(const BehaviorVector3& input,
                                       BehaviorVector3& output);
    [[nodiscard]] static float RandomRangeClamped(
        float center, float radius, float minimum, float maximum,
        IVectorRandomSource& random);
};

}  // namespace pal4
