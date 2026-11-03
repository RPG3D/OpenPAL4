#pragma once

#include "pal4/effect_system_object_f.h"

#include <cstdint>

namespace pal4 {

class EffectBaseAccess final {
public:
    // IDA 0x606b80..0x606bc0: bytes +7 through +11.
    [[nodiscard]] static std::uint8_t Byte7(
        const EffectSystemObjectFState& state);
    [[nodiscard]] static std::uint8_t Byte8(
        const EffectSystemObjectFState& state);
    [[nodiscard]] static std::uint8_t Byte9(
        const EffectSystemObjectFState& state);
    [[nodiscard]] static std::uint8_t Byte10(
        const EffectSystemObjectFState& state);
    [[nodiscard]] static std::uint8_t Byte11(
        const EffectSystemObjectFState& state);
    // IDA 0x606bd0: x87 loads the single-precision constant at 0x842634.
    [[nodiscard]] static double DefaultFactor();
    // IDA 0x606be0: returns owner word at +16.
    [[nodiscard]] static std::uint32_t System(
        const EffectSystemObjectFState& state);
};

}  // namespace pal4
