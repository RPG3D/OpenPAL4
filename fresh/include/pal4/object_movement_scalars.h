#pragma once

#include "pal4/object_movement_processor.h"

namespace pal4 {

class IObjectMovementScalarRuntime {
public:
    virtual ~IObjectMovementScalarRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 ReadSlot10Vector(
        ObjectMovementState& state) = 0;
};

class ObjectMovementScalarRuntimeStub final
    : public IObjectMovementScalarRuntime {
public:
    BehaviorVector3 ReadSlot10Vector(ObjectMovementState& state) override;
};

class ObjectMovementScalars final {
public:
    [[nodiscard]] static float Get23(const ObjectMovementState& state);
    [[nodiscard]] static float Set23(ObjectMovementState& state, float value);
    static BehaviorVector3& ScaledSlot10Vector(
        ObjectMovementState& state, BehaviorVector3& output,
        IObjectMovementScalarRuntime& runtime);
    [[nodiscard]] static float Get25(const ObjectMovementState& state);
    [[nodiscard]] static float Set25(ObjectMovementState& state, float value);
    [[nodiscard]] static float Get24(const ObjectMovementState& state);
    [[nodiscard]] static float Set24(ObjectMovementState& state, float value);
    [[nodiscard]] static float Get26(const ObjectMovementState& state);
    [[nodiscard]] static float Set26(ObjectMovementState& state, float value);
    [[nodiscard]] static float Get27(const ObjectMovementState& state);
    [[nodiscard]] static float Set27(ObjectMovementState& state, float value);
};

}  // namespace pal4
