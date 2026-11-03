#include "pal4/word_vector_storage.h"

#include <new>

namespace pal4 {

std::uint32_t WordVectorStorage::Clear(
    std::uint32_t vector, IWordVectorStorageRuntime& runtime) {
    const auto old_end = runtime.ReadWord(vector + 8u);
    runtime.WriteWord(vector + 8u, runtime.ReadWord(vector + 4u));
    return old_end;
}

std::uint32_t WordVectorStorage::Insert(
    std::uint32_t vector, std::uint32_t position,
    std::uint32_t source, IWordVectorStorageRuntime& runtime) {
    const auto begin = runtime.ReadWord(vector + 4u);
    const auto end = runtime.ReadWord(vector + 8u);
    const auto capacity_end = runtime.ReadWord(vector + 12u);
    const auto index = (position - begin) / 4u;

    if (capacity_end != end) {
        if (position != end) {
            runtime.WriteWord(end, runtime.ReadWord(end - 4u));
            for (auto current = end - 4u; current != position;
                 current -= 4u)
                runtime.WriteWord(current,
                                  runtime.ReadWord(current - 4u));
        }
        runtime.WriteWord(position, runtime.ReadWord(source));
        runtime.WriteWord(vector + 8u, end + 4u);
        return begin + index * 4u;
    }

    const auto count = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = count > 1u ? count : 1u;
    const auto new_capacity = count + growth;
    const auto replacement = runtime.Allocate(new_capacity * 4u);
    if (replacement == 0u) throw std::bad_alloc();
    for (std::uint32_t i = 0u; i < index; ++i)
        runtime.WriteWord(replacement + i * 4u,
                          runtime.ReadWord(begin + i * 4u));
    runtime.WriteWord(replacement + index * 4u,
                      runtime.ReadWord(source));
    for (std::uint32_t i = index; i < count; ++i)
        runtime.WriteWord(replacement + (i + 1u) * 4u,
                          runtime.ReadWord(begin + i * 4u));
    runtime.Free(begin);
    runtime.WriteWord(vector + 12u, replacement + new_capacity * 4u);
    runtime.WriteWord(vector + 4u, replacement);
    runtime.WriteWord(vector + 8u, replacement + (count + 1u) * 4u);
    return replacement + index * 4u;
}

}  // namespace pal4
