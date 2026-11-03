#pragma once

#include "pal4/effect_camera_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectCameraPropertiesRuntime {
public:
    virtual ~IEffectCameraPropertiesRuntime() = default;
    virtual void CalculateAngles(std::uint32_t matrix_address,
                                 EffectCameraObjectState& state,
                                 std::uint32_t output_byte_offset) = 0;
};

class EffectCameraPropertiesRuntimeStub final
    : public IEffectCameraPropertiesRuntime {
public:
    void CalculateAngles(std::uint32_t, EffectCameraObjectState&,
                         std::uint32_t) override {}
};

class EffectCameraProperties final {
public:
    // IDA 0x608660/0x6086b0. Native zero-output returns the third
    // output pointer's low byte.
    [[nodiscard]] static std::uint8_t GetPosition(
        const EffectCameraObjectState& state,
        std::array<std::uint32_t, 3>& output,
        std::uint8_t third_output_address_low_byte);
    [[nodiscard]] static std::uint8_t GetRotation(
        const EffectCameraObjectState& state,
        std::array<std::uint32_t, 3>& output,
        std::uint8_t third_output_address_low_byte);
    // IDA 0x608700/0x6087a0. Inputs preserve raw x86 float words.
    [[nodiscard]] static std::uint8_t SetPosition(
        EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& values,
        std::uint8_t relative);
    [[nodiscard]] static std::uint8_t SetRotation(
        EffectCameraObjectState& state,
        const std::array<std::uint32_t, 3>& values,
        std::uint8_t relative,
        IEffectCameraPropertiesRuntime& runtime);
};

}  // namespace pal4
