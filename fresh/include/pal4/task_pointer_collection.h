#pragma once

#include <cstdint>
#include <new>

namespace pal4 {

class TaskPointerCollection final {
public:
    // The helper append in IDA 0x5936a0 and 0x593c70.
    template <typename Runtime>
    static void Append(const std::uint32_t collection,
                       const std::uint32_t task, Runtime& runtime) {
        const auto begin = runtime.ReadWord(collection + 4u);
        const auto end = runtime.ReadWord(collection + 8u);
        const auto capacity = runtime.ReadWord(collection + 12u);
        if (capacity - end >= 4u) {
            runtime.WriteWord(end, task);
            runtime.WriteWord(collection + 8u, end + 4u);
            return;
        }

        const auto size = begin == 0u ? 0u : (end - begin) / 4u;
        const auto new_capacity = size + (size > 1u ? size : 1u);
        const auto memory = runtime.AllocateMemory(new_capacity * 4u);
        if (memory == 0u) throw std::bad_alloc{};
        for (std::uint32_t index = 0u; index < size; ++index)
            runtime.WriteWord(memory + index * 4u,
                              runtime.ReadWord(begin + index * 4u));
        runtime.WriteWord(memory + size * 4u, task);
        runtime.FreeMemory(begin);
        runtime.WriteWord(collection + 12u,
                          memory + new_capacity * 4u);
        runtime.WriteWord(collection + 4u, memory);
        runtime.WriteWord(collection + 8u,
                          memory + (size + 1u) * 4u);
    }
};

}  // namespace pal4
