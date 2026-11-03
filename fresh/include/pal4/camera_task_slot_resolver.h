#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class CameraTaskSlotResolver final {
public:
    // IDA 0x6071b0. The returned slot address uses x86 arithmetic.
    [[nodiscard]] static bool Resolve(std::uint32_t object,
                                      std::string_view task_name,
                                      std::uint32_t& output);
};

}  // namespace pal4
