#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

class StringCaseInsensitiveComparator final {
public:
    // 0x5eaec0: construct two temporary strings, compare and release them.
    [[nodiscard]] static std::int32_t Compare(
        std::uint32_t left, std::uint32_t right,
        IStringBufferRuntime& runtime);
    // IDA 0x5fc630: same two-temporary comparison for manager keys.
    [[nodiscard]] static std::int32_t CompareManagerKey(
        std::uint32_t left, std::uint32_t right,
        IStringBufferRuntime& runtime);
};

}  // namespace pal4
