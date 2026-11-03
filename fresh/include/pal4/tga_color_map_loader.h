#pragma once

#include <cstdint>

#include "pal4/tga_image_header_reader.h"

namespace pal4 {

class TgaColorMapLoader final {
public:
    // IDA 0x5f9270. Native return is always zero.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t stream, std::uint32_t metadata,
        ITgaImageHeaderRuntime& runtime);
};

}  // namespace pal4
