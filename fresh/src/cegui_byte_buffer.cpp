#include "pal4/cegui_byte_buffer.h"

namespace pal4 {

std::uint32_t CeguiByteBuffer::Insert(
    StaticCharacterBufferState& state, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t fill_address,
    ICeguiByteBufferRuntime& runtime) {
    const auto old_begin = state.multibyte_begin;
    const auto old_end = state.multibyte_end;
    const auto free_slots = state.multibyte_capacity_end - old_end;
    const auto tail = old_end - position;

    if (free_slots < count) {
        const auto old_size = old_begin == 0u ? 0u : old_end - old_begin;
        const auto new_capacity = old_size + (count >= old_size ? count : old_size);
        const auto new_begin = runtime.Allocate(new_capacity);
        auto target = new_begin;
        for (auto source = old_begin; source != position; ++source, ++target) {
            runtime.WriteByte(target, runtime.ReadByte(source));
        }
        for (std::uint32_t index = 0; index < count; ++index, ++target) {
            runtime.WriteByte(target, runtime.ReadByte(fill_address));
        }
        for (auto source = position; source != old_end; ++source, ++target) {
            runtime.WriteByte(target, runtime.ReadByte(source));
        }
        runtime.Free(old_begin);
        state.multibyte_begin = new_begin;
        state.multibyte_end = new_begin + old_size + count;
        state.multibyte_capacity_end = new_begin + new_capacity;
        return state.multibyte_end;
    }

    if (tail < count) {
        for (std::uint32_t index = 0; index < tail; ++index) {
            runtime.WriteByte(position + count + index,
                              runtime.ReadByte(position + index));
        }
        for (std::uint32_t index = 0; index < count - tail; ++index) {
            runtime.WriteByte(old_end + index, runtime.ReadByte(fill_address));
        }
        for (auto target = position; target != old_end; ++target) {
            runtime.WriteByte(target, runtime.ReadByte(fill_address));
        }
        state.multibyte_end = old_end + count;
        return position == old_end ? position : old_end;
    }
    if (count == 0u) return old_end;

    for (std::uint32_t index = 0; index < count; ++index) {
        runtime.WriteByte(old_end + index,
                          runtime.ReadByte(old_end - count + index));
    }
    for (auto source = old_end - count; source != position;) {
        --source;
        runtime.WriteByte(source + count, runtime.ReadByte(source));
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        runtime.WriteByte(position + index, runtime.ReadByte(fill_address));
    }
    state.multibyte_end = old_end + count;
    return position + count;
}

}  // namespace pal4
