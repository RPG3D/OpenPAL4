#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <cstdint>

namespace pal4 {

struct BehaviorPositionResult {
    std::uint32_t address = 0u;
    BehaviorVector3 value{};
};

class IBehaviorPathMovementRuntime {
public:
    virtual ~IBehaviorPathMovementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ProcessCollision(
        BehaviorPathEndpointState& state, BehaviorContext3& context,
        std::uint32_t input, float delta) = 0;
    virtual void UpdateBase(BehaviorPathEndpointState& state,
                            std::uint32_t collision_result,
                            float delta) = 0;
    [[nodiscard]] virtual BehaviorPositionResult ReadCurrentPosition(
        BehaviorPathEndpointState& state,
        BehaviorContext3& context) = 0;
    [[nodiscard]] virtual float ReadPathRadius(
        std::uint32_t path_address) = 0;
    [[nodiscard]] virtual std::uint32_t Read32(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t NotifyTargetSlot04(
        std::uint32_t captured_vtable, std::uint32_t current_target,
        std::uint32_t position_address) = 0;
};

class BehaviorPathMovementRuntimeStub final
    : public IBehaviorPathMovementRuntime {
public:
    std::uint32_t ProcessCollision(
        BehaviorPathEndpointState& state, BehaviorContext3& context,
        std::uint32_t input, float delta) override;
    void UpdateBase(BehaviorPathEndpointState& state,
                    std::uint32_t collision_result, float delta) override;
    BehaviorPositionResult ReadCurrentPosition(
        BehaviorPathEndpointState& state,
        BehaviorContext3& context) override;
    float ReadPathRadius(std::uint32_t path_address) override;
    std::uint32_t Read32(std::uint32_t address) override;
    std::uint32_t NotifyTargetSlot04(
        std::uint32_t captured_vtable, std::uint32_t current_target,
        std::uint32_t position_address) override;
};

class BehaviorPathEndpointMovement final {
public:
    [[nodiscard]] static std::uint32_t Update(
        BehaviorPathEndpointState& state, std::uint32_t input,
        float delta, IBehaviorPathMovementRuntime& runtime);
};

class IBehaviorPathAssociationRuntime {
public:
    virtual ~IBehaviorPathAssociationRuntime() = default;
    virtual void ReleaseAssociation(std::uint32_t object,
                                     std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t BindAssociation(
        std::uint32_t binder, BehaviorPathEndpointState& state) = 0;
};

class IBehaviorPathCallbackRuntime : public IBehaviorPathAssociationRuntime {
public:
    ~IBehaviorPathCallbackRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t InvokeCallback(
        std::uint32_t callback, BehaviorPathEndpointState& state,
        std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t DeleteObject(
        std::uint32_t object, std::uint32_t flags) = 0;
};

class BehaviorPathCallbackRuntimeStub final
    : public IBehaviorPathCallbackRuntime {
public:
    std::uint32_t InvokeCallback(
        std::uint32_t callback, BehaviorPathEndpointState& state,
        std::uint32_t argument) override;
    std::uint32_t DeleteObject(
        std::uint32_t object, std::uint32_t flags) override;
    void ReleaseAssociation(std::uint32_t object,
                             std::uint32_t flags) override;
    std::uint32_t BindAssociation(
        std::uint32_t binder, BehaviorPathEndpointState& state) override;
};

class BehaviorPathEndpointCallbacks final {
public:
    [[nodiscard]] static std::uint32_t Call(
        BehaviorPathEndpointState& state, std::uint32_t argument,
        IBehaviorPathCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteObject(
        std::uint32_t object, std::uint32_t incoming_eax,
        IBehaviorPathCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetAssociation(
        BehaviorPathEndpointState& state, std::uint32_t binder,
        IBehaviorPathAssociationRuntime& runtime);
};

}  // namespace pal4
