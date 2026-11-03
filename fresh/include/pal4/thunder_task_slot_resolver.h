#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class ThunderTaskSlotResolver final {
public:
    // IDA 0x61b500.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
