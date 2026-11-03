#include "pal4/grid_manager_members.h"
#include "pal4/dword_slot_fill.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace pal4 {
namespace {

bool AppendElement(
    GridManagerMembersState& state, const std::uint32_t element,
    IGridManagerMembersRuntime& runtime) {
    if (state.elements_end != state.elements_capacity) {
        (void)DwordSlotFill::Fill(state.elements_end, 1u, element, runtime);
        state.elements_end += 4u;
        return true;
    }
    const std::uint32_t size = state.elements_begin == 0u ? 0u :
        (state.elements_end - state.elements_begin) / 4u;
    const auto growth = size > 1u ? size : 1u;
    if (size > (std::numeric_limits<std::uint32_t>::max() / 4u) - growth)
        return false;
    const auto new_capacity = size + growth;
    const auto next = runtime.Allocate(4u * new_capacity);
    if (next == 0u)
        return false;
    for (std::uint32_t index = 0u; index < size; ++index)
        runtime.WriteWord(next + 4u * index,
                          runtime.ReadWord(state.elements_begin + 4u * index));
    (void)DwordSlotFill::Fill(next + 4u * size, 1u, element, runtime);
    if (state.elements_begin != 0u)
        (void)runtime.Free(state.elements_begin);
    state.elements_begin = next;
    state.elements_end = next + 4u * (size + 1u);
    state.elements_capacity = next + 4u * new_capacity;
    return true;
}

}  // namespace

std::vector<std::uint32_t> GridManagerMembers::SecondaryElements(
    const GridManagerMembersState& state, IGridManagerMembersRuntime& runtime) {
    const auto begin = state.secondary_begin;
    const auto end = state.secondary_end;
    const auto capacity = state.secondary_capacity;
    if (end < begin || capacity < end || (begin % 4u) ||
        ((end - begin) % 4u) || ((capacity - begin) % 4u) ||
        (!begin && (end || capacity)))
        throw std::logic_error("grid secondary pointer vector");
    std::vector<std::uint32_t> result;
    result.reserve((end - begin) / 4u);
    for (auto cursor = begin; cursor != end; cursor += 4u)
        result.push_back(runtime.ReadWord(cursor));
    return result;
}

bool GridManagerMembers::AddElement(
    GridManagerMembersState& state, const std::uint32_t element,
    IGridManagerMembersRuntime& runtime) {
    return state.grid_wrapper != 0u && element != 0u &&
           AppendElement(state, element, runtime);
}

std::uint32_t GridManagerMembers::AppendVisitedElement(
    GridManagerMembersState& state, const std::uint32_t element,
    const std::uint32_t, IGridManagerMembersRuntime& runtime) {
    if (!AppendElement(state, element, runtime))
        return 0u;
    return state.elements_end;
}

bool GridManagerMembers::RemoveElement(
    GridManagerMembersState& state, const std::uint32_t element,
    IGridManagerMembersRuntime& runtime) {
    for (auto cursor = state.elements_begin;
         cursor != state.elements_end; cursor += 4u) {
        if (runtime.ReadWord(cursor) != element)
            continue;
        for (auto next = cursor + 4u; next != state.elements_end;
             next += 4u)
            runtime.WriteWord(next - 4u, runtime.ReadWord(next));
        state.elements_end -= 4u;
        runtime.ReleaseElement(element);
        return true;
    }
    return false;
}

std::uint32_t GridManagerMembers::ClearElements(
    GridManagerMembersState& state, IGridManagerMembersRuntime& runtime) {
    for (auto cursor = state.elements_begin;
         cursor != state.elements_end; cursor += 4u)
        runtime.ReleaseElement(runtime.ReadWord(cursor));
    const auto previous_end = state.elements_end;
    state.elements_end = state.elements_begin;
    return previous_end;
}

std::uint32_t GridManagerMembers::PreviousElement(
    GridManagerMembersState& state) {
    const auto previous = state.secondary_end;
    state.secondary_end = state.secondary_begin;
    return previous;
}

void GridManagerMembers::EnableUpdate(GridManagerMembersState& state) {
    state.update_disabled = 1u;
}

void GridManagerMembers::DisableUpdate(GridManagerMembersState& state) {
    state.update_disabled = 0u;
}

std::uint8_t GridManagerMembers::UpdateElements(
    GridManagerMembersState& state, const std::uint32_t argument,
    IGridManagerMembersRuntime& runtime) {
    if (state.update_disabled != 0u)
        return state.update_disabled;
    for (auto cursor = state.elements_begin;
         cursor != state.elements_end; cursor += 4u)
        runtime.UpdateElement(runtime.ReadWord(cursor), state, argument);
    return static_cast<std::uint8_t>(state.elements_end);
}

}  // namespace pal4
