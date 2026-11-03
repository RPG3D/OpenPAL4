#include "pal4/cegui_text_record_array.h"

#include <algorithm>

namespace pal4 {

void CeguiTextRecord::Copy(const std::uint32_t destination,
                           const std::uint32_t source,
                           ICeguiTextRecordRuntime& runtime) {
    if (destination == 0u) {
        return;
    }
    const auto flag = runtime.ReadByte(source);
    runtime.WriteDword(destination + 4u, 0u);
    runtime.WriteByte(destination, flag);
    runtime.WriteDword(destination + 8u, 0u);
    runtime.WriteDword(destination + 12u, 0u);
    const auto size = runtime.ReadDword(source + 8u);
    if (destination == source) {
        if (size != 0u) {
            runtime.ThrowRange();
        }
        runtime.DetachAndReset(destination);
        const auto remaining = runtime.ReadDword(destination + 8u) - size;
        const auto start = remaining;
        if (start != 0u) {
            const auto data = runtime.ReadDword(destination + 4u) + size;
            runtime.CopyBytes(data, data + start, remaining - start);
            const auto new_size = runtime.ReadDword(destination + 8u) - start;
            if (runtime.AllocateText(destination, new_size, 0u)) {
                runtime.SetTextSize(destination, new_size);
            }
        }
        runtime.DetachAndReset(destination);
        return;
    }

    if (size != 0u) {
        const auto original_text = runtime.ReadDword(source + 4u);
        const auto shared_text = original_text == 0u
                                     ? runtime.EmptyTextAddress() : original_text;
        if (runtime.ReadByte(shared_text - 1u) < 0xfeu) {
            runtime.SetErrorFlag(destination, 1u);
            runtime.WriteDword(destination + 4u, shared_text);
            runtime.WriteDword(destination + 8u, size);
            runtime.WriteDword(destination + 12u,
                               runtime.ReadDword(source + 12u));
            runtime.WriteByte(shared_text - 1u,
                              static_cast<std::uint8_t>(
                                  runtime.ReadByte(shared_text - 1u) + 1u));
            return;
        }
    }
    if (!runtime.AllocateText(destination, size, 1u)) {
        return;
    }
    const auto original_text = runtime.ReadDword(source + 4u);
    const auto shared_text = original_text == 0u
                                 ? runtime.EmptyTextAddress() : original_text;
    const auto data = runtime.ReadDword(destination + 4u);
    runtime.CopyBytes(data, shared_text, size);
    runtime.WriteDword(destination + 8u, size);
    runtime.WriteByte(data + size, 0u);
}

void CeguiTextRecordArray::Insert(
    CeguiTextRecordArrayState& state, const std::uint32_t position,
    const std::uint32_t count, const std::uint32_t value,
    ICeguiTextRecordArrayRuntime& runtime) {
    const auto available = static_cast<std::uint32_t>(
        static_cast<std::int32_t>(state.capacity - state.end) >> 4);
    if (available < count) {
        const auto old_count = state.begin == 0u ? 0u :
            static_cast<std::uint32_t>(
                static_cast<std::int32_t>(state.end - state.begin) >> 4);
        const auto new_count = old_count + std::max(old_count, count);
        const auto bytes = static_cast<std::int32_t>(new_count) < 0
                               ? 0u : new_count * 16u;
        const auto allocation = runtime.AllocateRecords(bytes);
        auto output = allocation;
        for (auto source = state.begin; source != position;
             source += 16u, output += 16u) {
            CeguiTextRecord::Copy(output, source, runtime);
        }
        for (std::uint32_t index = 0u; index < count;
             ++index, output += 16u) {
            CeguiTextRecord::Copy(output, value, runtime);
        }
        for (auto source = position; source != state.end;
             source += 16u, output += 16u) {
            CeguiTextRecord::Copy(output, source, runtime);
        }
        for (auto record = state.begin; record != state.end;
             record += 16u) {
            runtime.SetErrorFlag(record, 1u);
        }
        runtime.FreeRecords(state.begin);
        const auto old_begin = state.begin;
        const auto old_end = state.end;
        state.capacity = allocation + new_count * 16u;
        state.begin = allocation;
        state.end = allocation + count * 16u +
                    (old_begin == 0u ? 0u : old_end - old_begin);
        return;
    }

    const auto tail = static_cast<std::uint32_t>(
        static_cast<std::int32_t>(state.end - position) >> 4);
    const auto bytes = count * 16u;
    if (tail < count) {
        for (std::uint32_t index = 0u; index < tail; ++index) {
            CeguiTextRecord::Copy(position + bytes + index * 16u,
                                  position + index * 16u, runtime);
        }
        for (std::uint32_t index = 0u; index < count - tail; ++index) {
            CeguiTextRecord::Copy(state.end + index * 16u, value, runtime);
        }
        for (std::uint32_t index = 0u; index < tail; ++index) {
            runtime.AssignRecord(position + index * 16u, value);
        }
    } else if (count != 0u) {
        for (std::uint32_t index = 0u; index < count; ++index) {
            CeguiTextRecord::Copy(state.end + index * 16u,
                                  state.end - bytes + index * 16u, runtime);
        }
        for (auto source = state.end - bytes; source != position;) {
            source -= 16u;
            runtime.AssignRecord(source + bytes, source);
        }
        for (std::uint32_t index = 0u; index < count; ++index) {
            runtime.AssignRecord(position + index * 16u, value);
        }
    }
    state.end += bytes;
}

std::uint32_t CeguiTextRecordArray::EraseRange(
    CeguiTextRecordArrayState& state, const std::uint32_t destination,
    std::uint32_t source, ICeguiTextRecordArrayRuntime& runtime) {
    const auto old_end = state.end;
    auto output = destination;
    while (source != old_end) {
        const auto size = runtime.ReadDword(source + 8u);
        if (output == source) {
            runtime.EraseText(output, size, 0xffffffffu);
            runtime.EraseText(output, 0u, 0u);
        } else {
            const auto original = runtime.ReadDword(source + 4u);
            const auto text = original == 0u
                                  ? runtime.EmptyTextAddress() : original;
            if (size != 0u && runtime.ReadByte(text - 1u) < 0xfeu) {
                runtime.SetErrorFlag(output, 1u);
                runtime.WriteDword(output + 4u, text);
                runtime.WriteDword(output + 8u, size);
                runtime.WriteDword(output + 12u,
                                   runtime.ReadDword(source + 12u));
                runtime.WriteByte(text - 1u, static_cast<std::uint8_t>(
                    runtime.ReadByte(text - 1u) + 1u));
            } else if (runtime.AllocateText(output, size, 1u)) {
                runtime.CopyBytes(runtime.ReadDword(output + 4u),
                                  text, size);
                runtime.SetTextSize(output, size);
            }
        }
        output += 16u;
        source += 16u;
    }

    for (auto record = output; record != old_end; record += 16u) {
        const auto text = runtime.ReadDword(record + 4u);
        if (text != 0u) {
            const auto header = text - 1u;
            const auto count = runtime.ReadByte(header);
            if (count == 0u || count == 0xffu)
                runtime.FreeRecords(header);
            else
                runtime.WriteByte(header,
                    static_cast<std::uint8_t>(count - 1u));
        }
        runtime.WriteDword(record + 4u, 0u);
        runtime.WriteDword(record + 8u, 0u);
        runtime.WriteDword(record + 12u, 0u);
    }
    state.end = output;
    return destination;
}

}  // namespace pal4
