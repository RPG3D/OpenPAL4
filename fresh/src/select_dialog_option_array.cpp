#include "pal4/select_dialog_option_records.h"

#include <algorithm>

namespace pal4 {

std::uint32_t SelectDialogOptionArray::Size(
    const std::uint32_t array, ISelectDialogOptionArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4u);
    if (begin == 0u) return 0u;
    return (runtime.ReadWord(array + 8u) - begin) / 16u;
}

std::uint32_t SelectDialogOptionArray::Insert(
    const std::uint32_t array, const std::uint32_t position,
    const std::uint32_t source,
    ISelectDialogOptionArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4u);
    const auto end = runtime.ReadWord(array + 8u);
    const auto capacity = runtime.ReadWord(array + 12u);
    const auto index = (position - begin) / 16u;
    if (capacity != end) {
        const auto tail = (end - position) / 16u;
        if (tail != 0u) {
            (void)SelectDialogOptionRecords::CopyRange(
                end - 16u, end, end, runtime);
            (void)SelectDialogOptionRecords::ReplaceRangeBackward(
                position, end - 16u, end, runtime);
            (void)SelectDialogOptionRecords::ReplaceRange(
                position, position + 16u, source, runtime);
        } else {
            (void)SelectDialogOptionRecords::CopyRange(
                position, end, position + 16u, runtime);
            SelectDialogOptionRecords::CopyRepeated(
                end, 1u, source, runtime);
            (void)SelectDialogOptionRecords::ReplaceRange(
                position, end, source, runtime);
        }
        runtime.WriteWord(array + 8u, end + 16u);
        return begin + 16u * index;
    }

    const auto old_size = begin == 0u ? 0u : (end - begin) / 16u;
    const auto new_capacity = old_size + std::max(1u, old_size);
    const auto new_begin = runtime.Allocate(16u * new_capacity);
    auto destination = SelectDialogOptionRecords::CopyRange(
        begin, position, new_begin, runtime);
    runtime.CopyRecord(destination, source);
    destination += 16u;
    destination = SelectDialogOptionRecords::CopyRange(
        position, end, destination, runtime);
    SelectDialogOptionRecords::Cleanup(begin, end, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12u, new_begin + 16u * new_capacity);
    runtime.WriteWord(array + 8u, destination);
    runtime.WriteWord(array + 4u, new_begin);
    return new_begin + 16u * index;
}

}  // namespace pal4
