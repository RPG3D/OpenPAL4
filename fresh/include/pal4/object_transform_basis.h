#pragma once

#include "pal4/object_movement_processor.h"

namespace pal4 {

class IObjectOrientationFlagRuntime {
public:
    virtual ~IObjectOrientationFlagRuntime() = default;
    [[nodiscard]] virtual std::uint32_t OrientationFlag(
        ObjectMovementState& state) = 0;
};

class IObjectTransformRuntime : public IObjectOrientationFlagRuntime {
public:
    virtual void PrepareAxis(ObjectMovementState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessNormalized(
        ObjectMovementState& state, const BehaviorVector3& vector) = 0;
    virtual void MakeDifference(ObjectMovementState& state,
                                BehaviorVector3& result,
                                const BehaviorVector3& input) = 0;
    virtual void TransformDifference(ObjectMovementState& state,
                                     BehaviorVector3& result,
                                     const BehaviorVector3& input) = 0;
    virtual void ProcessTransformed(ObjectMovementState& state,
                                    BehaviorVector3& output,
                                    const BehaviorVector3& input) = 0;
};

class ObjectTransformRuntimeStub final : public IObjectTransformRuntime {
public:
    std::uint32_t OrientationFlag(ObjectMovementState& state) override;
    void PrepareAxis(ObjectMovementState& state) override;
    std::uint32_t ProcessNormalized(
        ObjectMovementState& state, const BehaviorVector3& vector) override;
    void MakeDifference(ObjectMovementState& state,
                        BehaviorVector3& result,
                        const BehaviorVector3& input) override;
    void TransformDifference(ObjectMovementState& state,
                             BehaviorVector3& result,
                             const BehaviorVector3& input) override;
    void ProcessTransformed(ObjectMovementState& state,
                            BehaviorVector3& output,
                            const BehaviorVector3& input) override;
};

class ObjectTransformBasis final {
public:
    static BehaviorVector3& Multiply(const ObjectMovementState& state,
                                      BehaviorVector3& output,
                                      const BehaviorVector3& vector);
    [[nodiscard]] static std::uint32_t NormalizeFirstAxis(
        ObjectMovementState& state, IObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetThirdAxis(
        ObjectMovementState& state, const BehaviorVector3& vector,
        IObjectTransformRuntime& runtime);
    [[nodiscard]] static std::uint32_t NormalizeAndProcess(
        ObjectMovementState& state, const BehaviorVector3& vector,
        IObjectTransformRuntime& runtime);
    static BehaviorVector3& ReorderAndNegate(
        ObjectMovementState& state, BehaviorVector3& output,
        const BehaviorVector3& vector, IObjectOrientationFlagRuntime& runtime);
    static BehaviorVector3& ProcessTransformations(
        ObjectMovementState& state, BehaviorVector3& output,
        const BehaviorVector3& input, IObjectTransformRuntime& runtime);
};

}  // namespace pal4
