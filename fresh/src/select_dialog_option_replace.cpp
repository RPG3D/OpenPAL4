#include "pal4/select_dialog_option_records.h"

#include <cstdint>

namespace pal4 {

namespace {

void ReleaseBuffer(const std::uint32_t record,
                   ISelectDialogReplaceRuntime& runtime) {
    const auto pointer = runtime.ReadWord(record + 4u);
    if (pointer != 0u) {
        const auto reference = runtime.ReadByte(pointer - 1u);
        if (reference == 0u || reference == 0xffu) {
            runtime.Free(pointer - 1u);
        } else {
            runtime.WriteByte(pointer - 1u,
                              static_cast<std::uint8_t>(
                                  reference - 1u));
        }
    }
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
    runtime.WriteWord(record + 12u, 0u);
}

void ReplaceRecord(const std::uint32_t destination,
                   const std::uint32_t source,
                   ISelectDialogReplaceRuntime& runtime) {
    const auto source_length = runtime.ReadWord(source + 8u);
    if (destination == source) {
        if (runtime.ReadWord(destination + 8u) < source_length)
            runtime.ThrowRangeError();
        runtime.DetachAndReset(destination);
        const auto difference =
            runtime.ReadWord(destination + 8u) - source_length;
        if (difference != 0u) {
            const auto new_size = difference == 0xffffffffu ?
                runtime.ReadWord(destination + 8u) + 1u :
                source_length;
            if (runtime.AllocateBuffer(destination, new_size, false))
                runtime.SetSize(destination, new_size);
        }
        runtime.DetachAndReset(destination);
        return;
    }

    const auto source_pointer = runtime.ReadWord(source + 4u);
    const auto text = source_pointer == 0u ?
        runtime.EmptyStringPointer() : source_pointer;
    if (source_length != 0u &&
        runtime.ReadByte(text - 1u) < 0xfeu) {
        ReleaseBuffer(destination, runtime);
        runtime.WriteWord(destination + 4u, text);
        runtime.WriteWord(destination + 8u, source_length);
        runtime.WriteWord(destination + 12u,
                          runtime.ReadWord(source + 12u));
        runtime.WriteByte(text - 1u,
                          static_cast<std::uint8_t>(
                              runtime.ReadByte(text - 1u) + 1u));
        return;
    }

    if (runtime.AllocateBuffer(destination, source_length, true)) {
        const auto allocated = runtime.ReadWord(destination + 4u);
        runtime.CopyBytes(allocated, text, source_length);
        runtime.WriteWord(destination + 8u, source_length);
        runtime.WriteByte(allocated + source_length, 0u);
    }
}

}  // namespace

std::uint32_t SelectDialogOptionRecords::ReplaceRange(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t source,
    ISelectDialogReplaceRuntime& runtime) {
    for (auto record = begin; record != end; record += 16u)
        ReplaceRecord(record, source, runtime);
    return end;
}

std::uint32_t SelectDialogOptionRecords::ReplaceRangeBackward(
    const std::uint32_t begin, const std::uint32_t end,
    const std::uint32_t destination_end,
    ISelectDialogReplaceRuntime& runtime) {
    auto target = destination_end;
    for (auto record = end; record != begin;) {
        record -= 16u;
        target -= 16u;
        ReplaceRecord(target, record, runtime);
    }
    return target;
}

}  // namespace pal4
