#include "pal4/cegui_wide_buffer.h"

namespace pal4 {

std::uint32_t CeguiWideBuffer::Insert(
    StaticCharacterBufferState& state, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t fill_address,
    ICeguiWideBufferRuntime& runtime) {
    const auto old_begin = state.wide_begin;
    const auto old_end = state.wide_end;
    const auto free_slots = (state.wide_capacity_end - old_end) / 2u;
    const auto tail = (old_end - position) / 2u;

    if (free_slots < count) {
        const auto old_size = old_begin == 0u ? 0u : (old_end - old_begin) / 2u;
        const auto new_capacity = old_size + (count >= old_size ? count : old_size);
        const auto new_begin = runtime.Allocate(2u * new_capacity);
        auto target = new_begin;
        for (auto source = old_begin; source != position; source += 2u, target += 2u) {
            runtime.WriteWord(target, runtime.ReadWord(source));
        }
        for (std::uint32_t index = 0; index < count; ++index, target += 2u) {
            runtime.WriteWord(target, runtime.ReadWord(fill_address));
        }
        for (auto source = position; source != old_end; source += 2u, target += 2u) {
            runtime.WriteWord(target, runtime.ReadWord(source));
        }
        runtime.Free(old_begin);
        state.wide_begin = new_begin;
        state.wide_end = new_begin + 2u * (old_size + count);
        state.wide_capacity_end = new_begin + 2u * new_capacity;
        return state.wide_end;
    }

    if (tail < count) {
        for (std::uint32_t index = 0; index < tail; ++index) {
            runtime.WriteWord(position + 2u * (count + index),
                              runtime.ReadWord(position + 2u * index));
        }
        for (std::uint32_t index = 0; index < count - tail; ++index) {
            runtime.WriteWord(old_end + 2u * index,
                              runtime.ReadWord(fill_address));
        }
        for (auto target = position; target != old_end; target += 2u) {
            runtime.WriteWord(target, runtime.ReadWord(fill_address));
        }
    } else if (count != 0u) {
        for (std::uint32_t index = 0; index < count; ++index) {
            runtime.WriteWord(old_end + 2u * index,
                              runtime.ReadWord(old_end - 2u * count + 2u * index));
        }
        for (auto source = old_end - 2u * count; source != position;) {
            source -= 2u;
            runtime.WriteWord(source + 2u * count, runtime.ReadWord(source));
        }
        for (std::uint32_t index = 0; index < count; ++index) {
            runtime.WriteWord(position + 2u * index,
                              runtime.ReadWord(fill_address));
        }
    }
    state.wide_end = old_end + 2u * count;
    return tail < count || count == 0u
               ? state.wide_end
               : position + 2u * count;
}

std::uint32_t CeguiWideBuffer::CopyTo(
    StaticCharacterBufferState& state, const std::uint32_t destination,
    const std::uint32_t source, ICeguiWideBufferRuntime& runtime) {
    auto target = destination;
    for (auto cursor = source; cursor != state.wide_end; cursor += 2u, target += 2u) {
        runtime.WriteWord(target, runtime.ReadWord(cursor));
    }
    state.wide_end = target;
    return destination;
}

}  // namespace pal4
