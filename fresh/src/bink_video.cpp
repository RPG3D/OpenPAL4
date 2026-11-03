#include "pal4/bink_video.h"

#include <bit>

namespace pal4 {

BinkVideoState& BinkVideo::Initialize(BinkVideoState& state) {
    state.handle = 0u;
    state.raster = 0u;
    state.offset_x_bits = 0u;
    state.offset_y_bits = 0u;
    return state;
}

bool BinkVideo::Open(
    BinkVideoState& state, const std::uint32_t source,
    const float volume, const bool use_io_table,
    IBinkVideoRuntime& runtime) {
    (void)Close(state, runtime);
    state.handle = runtime.OpenBink(source, use_io_table);
    if (state.handle == 0u)
        return false;
    (void)SetVolume(state, volume, runtime);
    return true;
}

bool BinkVideo::OpenDefault(
    BinkVideoState& state, const std::uint32_t source,
    const float volume, IBinkVideoRuntime& runtime) {
    return Open(state, source, volume, true, runtime);
}

std::uint32_t BinkVideo::SetPlaybackMode(
    BinkVideoState& state, const std::uint32_t raster,
    IBinkVideoRuntime& runtime) {
    if (state.handle == 0u)
        return 0u;
    if (raster == 0u) {
        runtime.LogFailure(118u);
        return 0u;
    }
    state.raster = raster;
    const auto flags = runtime.RasterFlags(raster);
    if ((flags & 5u) != 0u)
        state.copy_mode = 3u;
    else if ((flags & 2u) != 0u)
        state.copy_mode = 10u;
    else
        runtime.LogFailure(119u);
    return raster;
}

std::uint32_t BinkVideo::SetParameters(
    BinkVideoState& state, const std::uint32_t width,
    const std::uint32_t height, const bool center,
    IBinkVideoRuntime& runtime) {
    if (state.handle == 0u)
        return 0u;
    const auto video_width = runtime.VideoWidth(state.handle);
    const auto video_height = runtime.VideoHeight(state.handle);
    if (width < video_width || height < video_height) {
        runtime.LogFailure(117u);
        return 0u;
    }
    const auto raster = runtime.CreateRaster(width, height, 0u, 5u);
    if (center) {
        state.offset_x_bits = std::bit_cast<std::uint32_t>(
            static_cast<float>((width - video_width) >> 1u));
        state.offset_y_bits = std::bit_cast<std::uint32_t>(
            static_cast<float>((height - video_height) >> 1u));
    }
    return SetPlaybackMode(state, raster, runtime);
}

std::uint32_t BinkVideo::TogglePause(
    const BinkVideoState& state, IBinkVideoRuntime& runtime) {
    return state.handle == 0u ? 0u :
           runtime.PauseBink(state.handle,
                             !runtime.IsPaused(state.handle));
}

std::uint32_t BinkVideo::Close(
    BinkVideoState& state, IBinkVideoRuntime& runtime) {
    if (state.handle == 0u)
        return 0u;
    const auto result = runtime.CloseBink(state.handle);
    state.raster = 0u;
    state.handle = 0u;
    return result;
}

}  // namespace pal4
