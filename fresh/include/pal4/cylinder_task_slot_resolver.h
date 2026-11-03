#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class CylinderTaskSlotResolver final {
public:
    // IDA 0x60c770. Returns x86 facet address and native boolean result.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
