#include "pal4/base_object_animation_selection.h"

#include <bit>

namespace pal4 {

std::uint32_t BaseObjectAnimationSelection::CompareAndSet(
    BaseObjectState& state, std::uint32_t requested_name,
    IBaseObjectAnimationSelectionRuntime& runtime) {
    const auto x = std::bit_cast<float>(state.Read(0x40cu));
    const auto z = std::bit_cast<float>(state.Read(0x414u));
    if ((state.Read(0x41cu) & 0xffu) != 0u || x != 0.0f || z != 0.0f) {
        (void)runtime.CompareIgnoreCase(requested_name, 0x8a6884u);
    }
    const auto current = runtime.CurrentAnimationName(state);
    if (runtime.CompareIgnoreCase(current, requested_name) != 0) {
        return runtime.SetCurrentAnimation(state, requested_name);
    }
    return 1u;
}

std::uint32_t BaseObjectAnimationSelection::CompareAndSetWithFlag(
    BaseObjectState& state, std::uint32_t requested_name,
    std::uint32_t flag, IBaseObjectAnimationSelectionRuntime& runtime) {
    const auto current = runtime.CurrentAnimationName(state);
    if (runtime.CompareIgnoreCase(current, requested_name) != 0) {
        return runtime.SetCurrentAnimationWithFlag(state, requested_name,
                                                   flag);
    }
    return 1u;
}

}  // namespace pal4
