#include "pal4/utilities_pair_array.h"

namespace pal4 {
namespace {

void CopyPair(std::uint32_t destination, std::uint32_t source,
              IUtilitiesPairArrayRuntime& runtime) {
    if (destination != 0) {
        runtime.WriteWord(destination, runtime.ReadWord(source));
        runtime.WriteWord(destination + 4,
                          runtime.ReadWord(source + 4));
    }
}

void CopyRange(std::uint32_t begin, std::uint32_t end,
               std::uint32_t destination,
               IUtilitiesPairArrayRuntime& runtime) {
    for (auto source = begin; source != end;
         source += 8, destination += 8)
        CopyPair(destination, source, runtime);
}

}  // namespace

std::uint32_t UtilitiesPairArray::CopyRangePlain(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination, IUtilitiesPairArrayRuntime& runtime) {
    for (auto source = begin; source != end; source += 8u, destination += 8u)
        CopyPair(destination, source, runtime);
    return destination;
}

std::uint32_t UtilitiesPairArray::FillPlain(
    std::uint32_t destination, const std::uint32_t count,
    const std::uint32_t source, IUtilitiesPairArrayRuntime& runtime) {
    if (count == 0u)
        return 0u;
    for (std::uint32_t index = 0u; index < count; ++index, destination += 8u)
        CopyPair(destination, source, runtime);
    return destination;
}

std::uint32_t UtilitiesPairArray::SelectUnique(
    const std::uint32_t begin, const std::uint32_t end,
    std::uint32_t destination,
    const std::function<bool(std::uint32_t, std::uint32_t)>& equals,
    IUtilitiesPairArrayRuntime& runtime) {
    if (begin == end) return destination;
    CopyPair(destination, begin, runtime);
    destination += 8u;
    auto previous = begin;
    for (auto candidate = begin + 8u; candidate != end; candidate += 8u) {
        if (!equals(previous, candidate)) {
            CopyPair(destination, candidate, runtime);
            destination += 8u;
            previous = candidate;
        }
    }
    return destination;
}

std::uint32_t UtilitiesPairArray::Insert(
    std::uint32_t array, std::uint32_t position,
    std::uint32_t source, IUtilitiesPairArrayRuntime& runtime) {
    const auto begin = runtime.ReadWord(array + 4);
    const auto end = runtime.ReadWord(array + 8);
    const auto capacity_end = runtime.ReadWord(array + 12);
    const auto index = (position - begin) / 8;
    if ((capacity_end - end) / 8 != 0) {
        if ((end - position) / 8 != 0) {
            CopyPair(end, end - 8, runtime);
            for (auto at = end - 8; at != position; at -= 8)
                CopyPair(at, at - 8, runtime);
            CopyPair(position, source, runtime);
        } else {
            CopyPair(end, source, runtime);
        }
        runtime.WriteWord(array + 8, end + 8);
        return begin + index * 8;
    }

    const auto used = begin ? (end - begin) / 8 : 0;
    const auto growth = used > 1 ? used : 1;
    const auto new_capacity = used + growth;
    const auto replacement = runtime.Allocate(new_capacity * 8);
    auto destination = replacement;
    CopyRange(begin, position, destination, runtime);
    destination += position - begin;
    CopyPair(destination, source, runtime);
    destination += 8;
    CopyRange(position, end, destination, runtime);
    runtime.Free(begin);
    runtime.WriteWord(array + 12, replacement + new_capacity * 8);
    runtime.WriteWord(array + 4, replacement);
    runtime.WriteWord(array + 8, replacement + (used + 1) * 8);
    return replacement + index * 8;
}

}  // namespace pal4
