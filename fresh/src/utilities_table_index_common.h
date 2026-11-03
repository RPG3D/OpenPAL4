#pragma once

#include <cstdint>

namespace pal4::detail {

template <class Runtime>
std::uint32_t FindSignedUtilitiesKey(
    std::uint32_t begin, std::uint32_t end,
    std::int32_t key, Runtime& runtime) {
    auto position = begin;
    auto count = (end - begin) / 8;
    while (count != 0) {
        const auto half = count / 2;
        const auto probe = position + 8 * half;
        if (static_cast<std::int32_t>(runtime.ReadWord(probe)) < key) {
            position = probe + 8;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    return position;
}

}  // namespace pal4::detail
