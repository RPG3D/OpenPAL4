#pragma once

#include "pal4/object_movement_processor.h"

namespace pal4 {

class IObjectMovementVectorRuntime {
public:
    virtual ~IObjectMovementVectorRuntime() = default;
    virtual void ProcessDifference(ObjectMovementState& state,
                                   BehaviorVector3& output,
                                   const BehaviorVector3& difference) = 0;
    [[nodiscard]] virtual BehaviorVector3 TransformVector(
        ObjectMovementState& state, std::uint32_t input) = 0;
};

class ObjectMovementVectorRuntimeStub final : public IObjectMovementVectorRuntime {
public:
    void ProcessDifference(ObjectMovementState& state,
                           BehaviorVector3& output,
                           const BehaviorVector3& difference) override;
    BehaviorVector3 TransformVector(ObjectMovementState& state,
                                    std::uint32_t input) override;
};

class ObjectMovementVectors final {
public:
    static void IgnoreFourArguments(std::uint32_t, std::uint32_t,
                                    std::uint32_t, std::uint32_t);
    // IDA 0x42B070, helper vtable slot zero.
    static BehaviorVector3& CopyAt4(const ObjectMovementState& state,
                                     BehaviorVector3& output);
    static BehaviorVector3& CopyAt16(const ObjectMovementState& state,
                                      BehaviorVector3& output);
    static BehaviorVector3& CopyAt28(const ObjectMovementState& state,
                                      BehaviorVector3& output);
    static BehaviorVector3& CopyAt40(const ObjectMovementState& state,
                                      BehaviorVector3& output);
    static BehaviorVector3& SetAt4(ObjectMovementState& state,
                                    BehaviorVector3& output,
                                    const BehaviorVector3& value);
    static BehaviorVector3& SetAt40(ObjectMovementState& state,
                                     BehaviorVector3& output,
                                     const BehaviorVector3& value);
    static BehaviorVector3& DifferenceAndProcess(
        ObjectMovementState& state, BehaviorVector3& output,
        const BehaviorVector3& target, IObjectMovementVectorRuntime& runtime);
    static BehaviorVector3& TransformAndAdd(
        ObjectMovementState& state, BehaviorVector3& output,
        std::uint32_t input, IObjectMovementVectorRuntime& runtime);
};

}  // namespace pal4
