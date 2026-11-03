#pragma once

#include "pal4/actor_control_motion.h"
#include "pal4/pal_actor_control.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IActorControlActionRuntime {
public:
    virtual ~IActorControlActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t UpdateCameraAndWorldView(
        std::uint32_t* component, std::uint32_t actor,
        std::uint32_t position_address);
    [[nodiscard]] virtual std::uint32_t MovementAdapterState();
};

class PalActorControlActions final {
public:
    // IDA 0x4264c0. base_address models the original 32-bit this pointer.
    [[nodiscard]] static std::uint32_t BeginMotion(
        PalActorControlState& state, std::uint32_t base_address,
        const std::array<std::uint32_t, 3>& start,
        const std::array<std::uint32_t, 3>& finish,
        std::uint32_t alternate_height_bits, std::uint32_t reached,
        IActorControlMotionRuntime& runtime);
    // IDA 0x4264f0.
    [[nodiscard]] static std::uint32_t ClearTargets(
        PalActorControlState& state);
    // IDA 0x426510.
    [[nodiscard]] static std::uint32_t UpdateWorldView(
        PalActorControlState& state, std::uint32_t position_address,
        IActorControlActionRuntime& runtime);
    // IDA 0x4265e0.
    [[nodiscard]] static std::uint32_t SetWord28(
        PalActorControlState& state, std::uint32_t value);
    // IDA 0x426670.
    [[nodiscard]] static bool CanStartAction(
        const PalActorControlState& state,
        IActorControlActionRuntime& runtime);
};

}  // namespace pal4
