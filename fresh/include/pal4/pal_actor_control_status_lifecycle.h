#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorControlStatusLifecycleRuntime {
public:
    virtual ~IActorControlStatusLifecycleRuntime() = default;
    virtual void SetModeConditional(PalActorControlState& state,
                                    std::uint32_t mode);
    [[nodiscard]] virtual std::uint32_t UiFrameManager();
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::string_view name);
    virtual void UpdateCharacterMovementState(std::uint32_t frame);
    [[nodiscard]] virtual bool HandleNeedsUpdate(std::uint32_t* handle);
    [[nodiscard]] virtual std::uint8_t UpdateHandle(std::uint32_t* handle);
    virtual void HandleDamageOrStatus(PalActorControlState& state,
                                      float value);
    [[nodiscard]] virtual std::uint8_t UpdateEffectProperties(
        std::uint32_t* handle, std::uint32_t actor_position_address,
        std::uint32_t delta_bits);
};

class PalActorControlStatusLifecycle final {
public:
    // IDA 0x426750.
    static void BeginMovement(PalActorControlState& state,
                              std::uint32_t value,
                              IActorControlStatusLifecycleRuntime& runtime);
    // IDA 0x4267c0.
    [[nodiscard]] static std::uint8_t Clear(
        PalActorControlState& state,
        IActorControlStatusLifecycleRuntime& runtime);
    // IDA 0x4267f0. Only the low return byte is defined by the assembly.
    [[nodiscard]] static std::uint8_t Tick(
        PalActorControlState& state, float delta_seconds,
        IActorControlStatusLifecycleRuntime& runtime);
};

}  // namespace pal4
