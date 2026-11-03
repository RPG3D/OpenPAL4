#include "pal4/game_data_ai_key_array.h"

#include <new>

namespace pal4 {

std::uint32_t GameDataAiKeyArray::Insert(
    std::uint32_t array, std::uint32_t position,
    const GameDataAiKeyEntry& entry, IGameDataAiKeyArrayRuntime& runtime) {
    constexpr std::uint32_t stride = 8;
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto insertion_index = (position - begin) / stride;

    if (capacity_end - end >= stride) {
        for (auto source = end; source != position; source -= stride)
            runtime.WriteKey(source, runtime.ReadKey(source - stride));
        runtime.WriteKey(position, entry);
        runtime.WriteWord(array + 8, end + stride);
        return begin + insertion_index * stride;
    }

    const auto old_count = begin ? (end - begin) / stride : 0;
    const auto growth = old_count > 1 ? old_count : 1;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * stride);
    if (!replacement) throw std::bad_alloc();
    for (std::uint32_t i = 0; i < insertion_index; ++i)
        runtime.WriteKey(replacement + i * stride,
                         runtime.ReadKey(begin + i * stride));
    runtime.WriteKey(replacement + insertion_index * stride, entry);
    for (std::uint32_t i = insertion_index; i < old_count; ++i)
        runtime.WriteKey(replacement + (i + 1) * stride,
                         runtime.ReadKey(begin + i * stride));
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * stride);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8, replacement + (old_count + 1) * stride);
    return replacement + insertion_index * stride;
}

std::uint32_t GameDataAiKeyArray::CopyRange(
    std::uint32_t begin, std::uint32_t end, std::uint32_t destination,
    IGameDataAiKeyArrayRuntime& runtime) {
    for (auto source = begin; source != end; source += 8) {
        runtime.WriteKey(destination, runtime.ReadKey(source));
        destination += 8;
    }
    return destination;
}

}  // namespace pal4
