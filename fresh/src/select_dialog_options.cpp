#include "pal4/select_dialog.h"

#include <cstdint>

namespace pal4 {

std::uint32_t SelectDialog::AddButton(
    const std::uint32_t object,
    const std::uint32_t source,
    ISelectDialogOptionRuntime& runtime) {
    const auto begin = runtime.ReadWord(object + 72u);
    const auto end = runtime.ReadWord(object + 76u);
    const auto capacity_end = runtime.ReadWord(object + 80u);
    if (capacity_end - end >= 16u) {
        runtime.CopyBuffer(end, end, end + 16u);
        runtime.CopyMultipleBuffers(end, 1u, source);
        runtime.RemoveAndReplaceBuffer(end, end, source);
        const auto new_end = end + 16u;
        runtime.WriteWord(object + 76u, new_end);
        return new_end;
    }

    const auto length = begin == 0u ? 0u :
        (end - begin) / 16u;
    const auto growth = length <= 1u ? 1u : length;
    const auto capacity = length + growth;
    const auto new_begin = runtime.Allocate(capacity * 16u);
    for (std::uint32_t index = 0u; index < length; ++index)
        runtime.CopyRecord(new_begin + index * 16u,
                           begin + index * 16u);
    const auto new_item = new_begin + length * 16u;
    runtime.CopyMultipleBuffers(new_item, 1u, source);
    runtime.CopyBuffer(end, end, new_item + 16u);
    runtime.CleanupStrings(begin, end);
    runtime.Free(begin);
    runtime.WriteWord(object + 80u,
                      new_begin + capacity * 16u);
    runtime.WriteWord(object + 72u, new_begin);
    runtime.WriteWord(object + 76u, new_item + 16u);
    return begin;
}

}  // namespace pal4
