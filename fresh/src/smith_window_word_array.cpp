#include "pal4/smith_window_word_array.h"

#include <limits>
#include <new>
#include <stdexcept>

namespace pal4 {

void SmithWindowWordArray::Clear(
    const std::uint32_t adapter,
    ISmithWindowWordArrayRuntime& runtime) {
    runtime.WriteWord(adapter + 8u, runtime.ReadWord(adapter + 4u));
}

void SmithWindowWordArray::Append(
    const std::uint32_t adapter, const std::uint32_t value,
    ISmithWindowWordArrayRuntime& runtime) {
    const auto end = runtime.ReadWord(adapter + 8u);
    const auto capacity_end = runtime.ReadWord(adapter + 12u);
    if (end != capacity_end) {
        runtime.WriteWord(end, value);
        runtime.WriteWord(adapter + 8u, end + 4u);
        return;
    }

    const auto begin = runtime.ReadWord(adapter + 4u);
    const auto size = begin == 0u ? 0u : (end - begin) / 4u;
    const auto growth = size > 1u ? size : 1u;
    if (size > std::numeric_limits<std::uint32_t>::max() - growth)
        throw std::length_error("SmithWindow word array capacity");
    const auto new_capacity = size + growth;
    if (new_capacity > std::numeric_limits<std::uint32_t>::max() / 4u)
        throw std::length_error("SmithWindow word array capacity");
    const auto memory = runtime.AllocateBytes(new_capacity * 4u);
    if (memory == 0u) throw std::bad_alloc();

    for (std::uint32_t i = 0u; i < size; ++i)
        runtime.WriteWord(memory + i * 4u,
                          runtime.ReadWord(begin + i * 4u));
    runtime.WriteWord(memory + size * 4u, value);
    runtime.FreeBytes(begin);
    runtime.WriteWord(adapter + 12u, memory + new_capacity * 4u);
    runtime.WriteWord(adapter + 4u, memory);
    runtime.WriteWord(adapter + 8u, memory + (size + 1u) * 4u);
}

}  // namespace pal4
