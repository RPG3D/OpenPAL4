#pragma once

#include <cstdint>

namespace pal4 {

class IRenderWareImageApi {
public:
    virtual ~IRenderWareImageApi() = default;
    [[nodiscard]] virtual std::uint32_t Create(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth) = 0;
    virtual void Destroy(std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t AllocatePixels(
        std::uint32_t image) = 0;
    [[nodiscard]] virtual std::uint32_t SetFromRaster(
        std::uint32_t image, std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t Write(
        std::uint32_t image, const char* path) = 0;
};

}  // namespace pal4
