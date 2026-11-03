#pragma once

#include "pal4/effect_system_object_f.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IEffectSystemObjectFAccessRuntime {
public:
    virtual ~IEffectSystemObjectFAccessRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
};

class EffectSystemObjectFAccessRuntimeStub final
    : public IEffectSystemObjectFAccessRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
};

class EffectSystemObjectFAccess final {
public:
    // IDA 0x606020. Returns the low byte of the second scale word.
    [[nodiscard]] static std::uint8_t GetScale(
        const EffectSystemObjectFState& state,
        std::array<std::uint32_t, 3>& output,
        IEffectSystemObjectFAccessRuntime& runtime);
    // IDA 0x606050. EAX contains the third output pointer on return.
    [[nodiscard]] static std::uint8_t GetRotation(
        const EffectSystemObjectFState& state,
        std::array<std::uint32_t, 3>& output,
        std::uint8_t third_output_address_low_byte,
        IEffectSystemObjectFAccessRuntime& runtime);
    // IDA 0x606080. Returns the low byte of the third position word.
    [[nodiscard]] static std::uint8_t GetPosition(
        const EffectSystemObjectFState& state,
        std::array<std::uint32_t, 3>& output,
        IEffectSystemObjectFAccessRuntime& runtime);
};

}  // namespace pal4
