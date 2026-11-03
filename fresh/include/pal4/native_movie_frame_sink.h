#pragma once

#include <cstdint>
#include <span>

namespace pal4 {

class INativeMovieFrameSink {
public:
    virtual ~INativeMovieFrameSink() = default;
    [[nodiscard]] virtual bool Upload(
        std::span<const std::uint8_t> bgra_pixels,
        std::uint32_t width, std::uint32_t height) = 0;
};

}  // namespace pal4
