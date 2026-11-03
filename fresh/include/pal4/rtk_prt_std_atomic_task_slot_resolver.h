#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class RtkPrtStdAtomicTaskSlotResolver final {
public:
    // IDA 0x6125f0.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
