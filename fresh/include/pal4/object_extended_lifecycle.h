#pragma once

#include "pal4/object_movement_processor.h"

namespace pal4 {

class IObjectExtendedRuntime {
public:
    virtual ~IObjectExtendedRuntime() = default;
    virtual void ConstructBase(ObjectMovementState& state) = 0;
    virtual void CleanupBase(ObjectMovementState& state) = 0;
    virtual void Free(std::uint32_t object_address) = 0;
    virtual void CallCapturedSlotB0(std::uint32_t vtable,
                                     std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CallCurrentSlotA0(
        ObjectMovementState& state) = 0;
};

class ObjectExtendedRuntimeStub final : public IObjectExtendedRuntime {
public:
    void ConstructBase(ObjectMovementState& state) override;
    void CleanupBase(ObjectMovementState& state) override;
    void Free(std::uint32_t object_address) override;
    void CallCapturedSlotB0(std::uint32_t vtable,
                             std::uint32_t value) override;
    std::uint32_t CallCurrentSlotA0(ObjectMovementState& state) override;
};

class ObjectExtendedLifecycle final {
public:
    static constexpr std::uint32_t kVtable = 0x842978u;

    static ObjectMovementState& Construct(ObjectMovementState& state,
                                           IObjectExtendedRuntime& runtime);
    static void Cleanup(ObjectMovementState& state,
                        IObjectExtendedRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        ObjectMovementState& state, std::uint32_t object_address,
        std::uint32_t flags, IObjectExtendedRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPropertyAndDispatch(
        ObjectMovementState& state, std::uint32_t first,
        std::uint32_t property, IObjectExtendedRuntime& runtime);
};

}  // namespace pal4
