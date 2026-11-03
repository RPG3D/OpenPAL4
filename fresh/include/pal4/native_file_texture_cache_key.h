#pragma once

#include <filesystem>
#include <string>

namespace pal4 {

// File-backed textures need their source directory in the cache identity when
// assets and writable saves live in different roots. CRC/refcount operations
// remain owned by the existing RenderWare texture dictionary implementation.
[[nodiscard]] inline std::string NativeFileTextureCacheKey(
    const std::filesystem::path& root, const std::filesystem::path& file) {
    return (std::filesystem::absolute(root) / file).lexically_normal().make_preferred().string();
}

}  // namespace pal4
