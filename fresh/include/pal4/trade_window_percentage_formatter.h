#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class TradeWindowPercentageFormatter final {
public:
    // IDA 0x491470.
    [[nodiscard]] static std::string Format(std::int32_t value);
};

}  // namespace pal4
