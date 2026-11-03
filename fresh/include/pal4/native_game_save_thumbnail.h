#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>

namespace pal4 {
struct RwTextureDictionaryState;
struct NativeGameSaveThumbnailResult {
    std::uint8_t original_status{};
    bool image_written{},cache_removed{};
};
struct NativeGameSaveThumbnailServices {
    std::uint32_t ui_manager;
    // The UI's actual file cache. Filesystem entries use NativeFileTextureCacheKey
    // with the same archive root supplied to Capture.
    RwTextureDictionaryState& textures;
    std::function<void()> enter_filesystem;
    std::function<void()> leave_filesystem;
};
class NativeGameSaveThumbnail final {
public:
    // Reuses5CC5A0 with the borrowed cached SDK image at manager+148h.
    // SDK/PNG and filesystem must be initialized; no new snapshot is taken.
    // Original status stays1 even if PNG writing fails; report that separately.
    [[nodiscard]] static NativeGameSaveThumbnailResult Capture(
        const std::filesystem::path& root,std::int32_t slot,
        NativeGameSaveThumbnailServices services);
};
} // namespace pal4
