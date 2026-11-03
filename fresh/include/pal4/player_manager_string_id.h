#pragma once

#include <array>
#include <cstdint>

#include "pal4/string_buffer.h"

namespace pal4 {

class PlayerManagerStringId final {
public:
    static constexpr std::array<std::uint32_t, 4> kCandidatePointers{
        0x8a7414u, 0x8a7418u, 0x8a741cu, 0x8a7420u};

    // IDA 0x441550. Returns 4 when none of the four names match.
    [[nodiscard]] static std::uint32_t Lookup(
        std::uint32_t input_c_string, IStringBufferRuntime& runtime);
};

}  // namespace pal4
