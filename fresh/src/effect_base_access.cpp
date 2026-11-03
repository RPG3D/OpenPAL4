#include "pal4/effect_base_access.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t Byte(const EffectSystemObjectFState& state,
                  const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     (8u * (offset % 4u)));
}

}  // namespace

std::uint8_t EffectBaseAccess::Byte7(const EffectSystemObjectFState& state) {
    return Byte(state, 7u);
}

std::uint8_t EffectBaseAccess::Byte8(const EffectSystemObjectFState& state) {
    return Byte(state, 8u);
}

std::uint8_t EffectBaseAccess::Byte9(const EffectSystemObjectFState& state) {
    return Byte(state, 9u);
}

std::uint8_t EffectBaseAccess::Byte10(const EffectSystemObjectFState& state) {
    return Byte(state, 10u);
}

std::uint8_t EffectBaseAccess::Byte11(const EffectSystemObjectFState& state) {
    return Byte(state, 11u);
}

double EffectBaseAccess::DefaultFactor() {
    return static_cast<double>(std::bit_cast<float>(0x3f4ccccdu));
}

std::uint32_t EffectBaseAccess::System(const EffectSystemObjectFState& state) {
    return state.words[4u];
}

}  // namespace pal4
