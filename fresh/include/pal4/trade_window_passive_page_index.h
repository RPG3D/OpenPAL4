#pragma once

#include <cstdint>

namespace pal4 {

class TradeWindowPassivePageIndex final {
public:
    // IDA 0x493ed0/0x493ee0.
    [[nodiscard]] static std::int32_t Next(std::int32_t index);
    [[nodiscard]] static std::int32_t Previous(std::int32_t index);
};

}  // namespace pal4
