#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class EffectObjectTaskSlotResolver final {
public:
    // IDA 0x610040.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
