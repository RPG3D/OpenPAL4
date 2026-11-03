#include "pal4/condition_value_array.h"

namespace pal4 {

std::uint32_t ConditionValueArrayRuntimeStub::ReadDword(std::uint32_t) {
    return 0u;
}
void ConditionValueArrayRuntimeStub::WriteDword(std::uint32_t, std::uint32_t) {}
std::uint32_t ConditionValueArrayRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void ConditionValueArrayRuntimeStub::Free(std::uint32_t) {}
std::uint32_t ConditionValueIteratorRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void ConditionValueIteratorRuntimeStub::Store(
    std::uint32_t, const ConditionValueIteratorImage&) {}
void ConditionValueIteratorRuntimeStub::WriteOutput(
    std::uint32_t, std::uint32_t) {}

bool ConditionValueArray::Contains(const ConditionValueArrayState& state,
                                   std::uint32_t value,
                                   IConditionValueArrayRuntime& runtime) {
    const auto initial_begin = state.begin;
    std::uint32_t index = 0u;
    while (initial_begin != 0u) {
        const auto count = static_cast<std::uint32_t>(
            static_cast<std::int32_t>(state.end - initial_begin) >> 2);
        if (index >= count)
            break;
        if (runtime.ReadDword(state.begin + index * 4u) == value)
            return true;
        ++index;
    }
    return false;
}

std::uint32_t ConditionValueArray::SetPresent(
    ConditionValueArrayState& state, std::uint32_t value,
    std::uint32_t enabled, IConditionValueArrayRuntime& runtime) {
    auto cursor = state.begin;
    while (cursor != state.end && runtime.ReadDword(cursor) != value)
        cursor += 4u;

    if (cursor != state.end) {
        if (static_cast<std::uint8_t>(enabled) != 0u)
            return (state.end & 0xffffff00u) | (enabled & 0xffu);
        for (auto source = cursor + 4u; source != state.end; source += 4u)
            runtime.WriteDword(source - 4u, runtime.ReadDword(source));
        state.end -= 4u;
        return state.end;
    }

    if (static_cast<std::uint8_t>(enabled) == 0u)
        return (state.end & 0xffffff00u) | (enabled & 0xffu);

    const auto old_end = state.end;
    const auto size = state.begin == 0u ? 0u : (state.end - state.begin) / 4u;
    if (state.capacity_end != state.end) {
        runtime.WriteDword(state.end, value);
        state.end += 4u;
        return old_end;
    }

    const auto capacity = size + (size > 1u ? size : 1u);
    const auto replacement = runtime.Allocate(capacity * 4u);
    for (std::uint32_t index = 0; index < size; ++index)
        runtime.WriteDword(replacement + index * 4u,
                           runtime.ReadDword(state.begin + index * 4u));
    runtime.WriteDword(replacement + size * 4u, value);
    runtime.Free(state.begin);
    state.begin = replacement;
    state.end = replacement + (size + 1u) * 4u;
    state.capacity_end = replacement + capacity * 4u;
    return state.end;
}

std::uint32_t ConditionValueArray::GetIterator(
    const ConditionValueArrayState& state, std::uint32_t object_address,
    std::uint32_t output_address, IConditionValueIteratorRuntime& runtime) {
    const auto iterator = runtime.Allocate(12u);
    if (iterator != 0u)
        runtime.Store(iterator, {kIteratorVtable, state.begin,
                                 object_address + 0xb0u});
    runtime.WriteOutput(output_address, iterator);
    return output_address;
}

}  // namespace pal4
