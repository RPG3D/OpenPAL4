#include "pal4/ai_action_pointer_array.h"

#include <new>

namespace pal4 {

std::uint32_t AiActionPointerArray::Capacity(
    const std::uint32_t array,
    IAiActionPointerArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4u);
    if (begin == 0u) return 0u;
    return static_cast<std::uint32_t>(
        static_cast<std::int32_t>(
            runtime.ReadWord(array + 12u) - begin) >> 2);
}

std::uint32_t AiActionPointerArray::InsertOne(
    const std::uint32_t array, const std::uint32_t position,
    const std::uint32_t source,
    IAiActionPointerArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4u);
    const auto end = runtime.ReadWord(array + 8u);
    const auto capacity_end = runtime.ReadWord(array + 12u);
    const auto index = (position - begin) / 4u;

    if (static_cast<std::int32_t>(capacity_end - end) >> 2) {
        if (position != end) {
            for (auto destination = end; destination != position;
                 destination -= 4u) {
                runtime.WriteWord(destination,
                                  runtime.ReadWord(destination - 4u));
            }
            runtime.WriteWord(position, runtime.ReadWord(source));
        } else if (end != 0u) {
            runtime.WriteWord(end, runtime.ReadWord(source));
        }
        runtime.WriteWord(array + 8u, end + 4u);
        return begin + 4u * index;
    }

    const auto old_count = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = old_count > 1u ? old_count : 1u;
    const auto new_capacity = old_count + growth;
    const auto replacement = runtime.Allocate(new_capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();

    auto destination = replacement;
    for (auto from = begin; from != position;
         from += 4u, destination += 4u)
        runtime.WriteWord(destination, runtime.ReadWord(from));
    runtime.WriteWord(destination, runtime.ReadWord(source));
    destination += 4u;
    for (auto from = position; from != end;
         from += 4u, destination += 4u)
        runtime.WriteWord(destination, runtime.ReadWord(from));

    runtime.Free(begin);
    runtime.WriteWord(array + 12u,
                      replacement + new_capacity * 4u);
    runtime.WriteWord(array + 4u, replacement);
    runtime.WriteWord(array + 8u,
                      replacement + (old_count + 1u) * 4u);
    return replacement + index * 4u;
}

}  // namespace pal4
