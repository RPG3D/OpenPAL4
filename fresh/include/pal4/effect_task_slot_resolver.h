#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class EffectTaskSlotResolver final {
public:
    // IDA 0x605470. The output is an address inside the original x86 object.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
