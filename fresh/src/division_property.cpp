#include "pal4/division_property.h"

namespace pal4 {

std::uint32_t DivisionProperty::SetDivisor(
    DivisionPropertyState& state, std::uint32_t divisor) {
    state.count = 0u;
    state.inverse_divisor = divisor == 0u
        ? 0.0f
        : static_cast<float>(1.0 / static_cast<double>(divisor));
    return divisor;
}

std::uint8_t DivisionProperty::SetFlag(
    DivisionPropertyState& state, std::uint8_t flag) {
    state.header[1] = (state.header[1] & 0xffffff00u) | flag;
    return flag;
}

}  // namespace pal4
