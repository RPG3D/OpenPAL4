#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class TimeFormatter final {
public:
    [[nodiscard]] static std::string FormatSeconds(
        std::int32_t seconds);
};

}  // namespace pal4
