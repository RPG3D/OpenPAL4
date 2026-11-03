#pragma once

#include <optional>

struct RwTexture;

namespace pal4 {

// Reads Alpha from a loaded DXT texture through public RenderWare and D3D9 APIs.
// An empty result means the texture could not be inspected safely.
[[nodiscard]] std::optional<bool> NativeCompressedRasterHasAlpha(
    RwTexture* texture);

// SDK raster alpha classification for the compressed rendering path. This is
// distinct from a scan of decoded pixels; opaque DXT5 contents can retain alpha.
[[nodiscard]] std::optional<bool> NativeCompressedRasterAlphaFlag(RwTexture* texture);

}  // namespace pal4
