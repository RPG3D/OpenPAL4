#pragma once
#include "pal4/minimap_texture_state.h"

namespace pal4 {
// Borrows native Win32 raster/image handles; the game retains ownership.
// CommitRaster clears its dirty flag before this call, including SDK failure.
class NativeMinimapRasterUpload final : public IMinimapRasterUpload {
public:
    void SetRasterFromImage(std::uint32_t raster, std::uint32_t image) override;
};
}
