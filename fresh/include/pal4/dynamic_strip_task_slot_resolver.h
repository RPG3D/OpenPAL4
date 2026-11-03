#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class DynamicStripTaskSlotResolver final {
public:
    // IDA 0x60e800. Returns x86 facet address and native boolean result.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
