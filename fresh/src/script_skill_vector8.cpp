#include "pal4/script_skill_vector8.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kWidth = 8u;

std::array<std::uint32_t, 2> ReadItem(
    std::uint32_t address, IScriptSkillVector8Runtime& runtime) {
    return {runtime.ReadWord(address), runtime.ReadWord(address + 4u)};
}

void WriteItem(std::uint32_t address,
               const std::array<std::uint32_t, 2>& item,
               IScriptSkillVector8Runtime& runtime) {
    runtime.WriteWord(address, item[0]);
    runtime.WriteWord(address + 4u, item[1]);
}

}  // namespace

std::uint32_t ScriptSkillVector8::Insert(
    std::uint32_t vector_header, std::uint32_t position,
    std::uint32_t count, const std::array<std::uint32_t, 2>& item,
    IScriptSkillVector8Runtime& runtime) {
    const auto begin = runtime.ReadWord(vector_header + 4u);
    const auto end = runtime.ReadWord(vector_header + 8u);
    const auto capacity = runtime.ReadWord(vector_header + 12u);
    if (end < begin || capacity < end || position < begin || position > end ||
        (end - begin) % kWidth != 0u ||
        (position - begin) % kWidth != 0u)
        throw std::invalid_argument("invalid 8-byte skill vector range");
    if (count == 0u) return end;
    if (count > 0xffffffffu / kWidth)
        throw std::length_error("skill vector insert count overflows address span");
    const auto bytes = count * kWidth;
    const auto tail = (end - position) / kWidth;

    if (capacity - end >= bytes) {
        if (tail >= count) {
            for (std::uint32_t i = 0u; i < count; ++i)
                WriteItem(end + i * kWidth,
                          ReadItem(end - bytes + i * kWidth, runtime), runtime);
            for (auto i = tail - count; i > 0u; --i) {
                const auto source = position + (i - 1u) * kWidth;
                WriteItem(source + bytes, ReadItem(source, runtime), runtime);
            }
        } else {
            for (std::uint32_t i = 0u; i < tail; ++i)
                WriteItem(position + bytes + i * kWidth,
                          ReadItem(position + i * kWidth, runtime), runtime);
        }
        for (std::uint32_t i = 0u; i < count; ++i)
            WriteItem(position + i * kWidth, item, runtime);
        runtime.WriteWord(vector_header + 8u, end + bytes);
        return end + bytes;
    }

    const auto old_size = begin == 0u ? 0u : (end - begin) / kWidth;
    const auto growth = old_size > count ? old_size : count;
    if (old_size > 0xffffffffu / kWidth - growth)
        throw std::length_error("skill vector capacity overflows address span");
    const auto new_capacity = old_size + growth;
    const auto allocated = runtime.Allocate(new_capacity * kWidth);
    if (allocated == 0u) return 0u;

    const auto prefix = begin == 0u ? 0u : (position - begin) / kWidth;
    for (std::uint32_t i = 0u; i < prefix; ++i)
        WriteItem(allocated + i * kWidth,
                  ReadItem(begin + i * kWidth, runtime), runtime);
    for (std::uint32_t i = 0u; i < count; ++i)
        WriteItem(allocated + (prefix + i) * kWidth, item, runtime);
    for (std::uint32_t i = prefix; i < old_size; ++i)
        WriteItem(allocated + (count + i) * kWidth,
                  ReadItem(begin + i * kWidth, runtime), runtime);
    runtime.Free(begin);
    runtime.WriteWord(vector_header + 12u,
                      allocated + new_capacity * kWidth);
    runtime.WriteWord(vector_header + 4u, allocated);
    const auto new_end = allocated + (old_size + count) * kWidth;
    runtime.WriteWord(vector_header + 8u, new_end);
    return new_end;
}

}  // namespace pal4
