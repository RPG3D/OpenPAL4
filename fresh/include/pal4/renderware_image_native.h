#pragma once
#include "pal4/renderware_image.h"

namespace pal4 {
// Win32 bridge for the existing game-facing image interface. Handles are
// native pointers in this host, never addresses inside the original EXE.
// The owner must complete RwEngineStart before use and destroy all images
// before RwEngineStop. RenderWare owns image and pixel allocation/deallocation.
class NativeRenderWareImageApi final : public IRenderWareImageApi {
public:
    [[nodiscard]] std::uint32_t Create(std::uint32_t width, std::uint32_t height,
                                       std::uint32_t depth) override;
    void Destroy(std::uint32_t image) override;
    [[nodiscard]] std::uint32_t AllocatePixels(std::uint32_t image) override;
    [[nodiscard]] std::uint32_t SetFromRaster(std::uint32_t image, std::uint32_t raster) override;
    [[nodiscard]] std::uint32_t Write(std::uint32_t image, const char* path) override;
};
}  // namespace pal4
