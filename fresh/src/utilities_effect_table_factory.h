#pragma once

#include <array>
#include <cstdint>

namespace pal4::detail {

template <class Runtime>
std::uint32_t CreateUtilitiesEffectTable(
    std::uint32_t source, std::uint32_t vtable,
    Runtime& runtime) {
    const auto object = runtime.Allocate(0x8c);
    if (object == 0) return 0;
    runtime.ConstructEffect(object, source);
    for (const auto offset : std::array{92u, 108u, 124u}) {
        runtime.WriteByte(object + offset,
                          static_cast<std::uint8_t>(source));
        runtime.WriteWord(object + offset + 4, 0);
        runtime.WriteWord(object + offset + 8, 0);
        runtime.WriteWord(object + offset + 12, 0);
    }
    runtime.WriteWord(object, vtable);
    return object;
}

}  // namespace pal4::detail
