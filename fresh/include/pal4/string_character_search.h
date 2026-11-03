#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class StringCharacterSearch final {
public:
    [[nodiscard]] static std::int32_t FindFirstOf(
        std::string_view text, std::int32_t start,
        std::string_view characters);
};

}  // namespace pal4
