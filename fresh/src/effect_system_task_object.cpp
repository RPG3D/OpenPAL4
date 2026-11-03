#include "pal4/effect_system_task_object.h"

#include <bit>

namespace pal4 {

EffectSystemTaskObjectState& EffectSystemTaskObject::Construct(
    EffectSystemTaskObjectState& state) {
    base_.Construct(state.base);
    state.base.vtable = kVtable;
    state.enabled = 0u;
    state.copy_timing = 0u;
    state.reserved64 = 0u;
    state.property68 = 0u;
    return state;
}

std::uint32_t EffectSystemTaskObject::Cleanup(
    EffectSystemTaskObjectState& state) {
    state.base.vtable = kVtable;
    if (state.enabled != 0u) {
        state.enabled = 0u;
        state.copy_timing = 0u;
        state.reserved64 = 0u;
        state.property68 = 0u;
    }
    return base_.Destroy(state.base);
}

std::uint32_t EffectSystemTaskObject::Delete(
    EffectSystemTaskObjectState& state,
    std::uint32_t object_address, std::uint32_t flags) {
    (void)Cleanup(state);
    if ((flags & 1u) != 0u)
        (void)runtime_.FreeMemory(object_address);
    return object_address;
}

std::uint8_t EffectSystemTaskObject::SetupState(
    EffectSystemTaskObjectState& state) {
    if (state.copy_timing == 0u || state.base.previous == 0u) return 0u;
    const auto linked = state.base.previous;
    state.base.wait_mode = runtime_.ReadWord(linked + 12u);
    state.base.expect_time = std::bit_cast<float>(
        runtime_.ReadWord(linked + 20u));
    const auto wait_bits = runtime_.ReadWord(linked + 16u);
    state.base.wait_time = std::bit_cast<float>(wait_bits);
    return static_cast<std::uint8_t>(wait_bits);
}

}  // namespace pal4
