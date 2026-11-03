#include "pal4/bink_video.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace pal4 {

bool BinkVideo::Update(BinkVideoState& state, IBinkVideoRuntime& runtime) {
    if (state.handle == 0u)
        return false;
    if (!WaitFrame(state, runtime))
        (void)RenderFrame(state, runtime);
    return IsPlaying(state, runtime);
}

std::uint32_t BinkVideo::SetVolume(
    const BinkVideoState& state, const float volume,
    IBinkVideoRuntime& runtime) {
    if (state.handle == 0u)
        return 0u;
    const auto clamped = std::clamp(volume, 0.0f, 1.0f);
    const auto scaled = std::isnan(volume) ? 0x80000000u :
        static_cast<std::uint32_t>(
            static_cast<double>(clamped) * 65535.0);
    return runtime.SetBinkVolume(state.handle, 0u, scaled);
}

bool BinkVideo::IsPlaying(
    const BinkVideoState& state, IBinkVideoRuntime& runtime) {
    if (state.handle == 0u)
        return false;
    const auto frame = runtime.FrameNumber(state.handle);
    const auto count = runtime.FrameCount(state.handle);
    return frame != count;
}

bool BinkVideo::WaitFrame(
    const BinkVideoState& state, IBinkVideoRuntime& runtime) {
    return state.handle != 0u && runtime.WaitBink(state.handle) == 1u;
}

bool BinkVideo::RenderFrame(
    BinkVideoState& state, IBinkVideoRuntime& runtime) {
    if (state.handle == 0u || state.raster == 0u)
        return false;
    runtime.DoFrame(state.handle);
    const auto width = runtime.RasterWidth(state.raster);
    const auto pixels = runtime.LockRaster(state.raster, 0u, 1u);
    if (pixels == 0u)
        return false;
    runtime.CopyToBuffer(
        state.handle, pixels, runtime.RasterStride(state.raster), width,
        static_cast<std::int32_t>(std::bit_cast<float>(state.offset_x_bits)),
        static_cast<std::int32_t>(std::bit_cast<float>(state.offset_y_bits)),
        state.copy_mode | 0x80000000u);
    runtime.UnlockRaster(state.raster);
    const auto frame = runtime.FrameNumber(state.handle);
    const auto count = runtime.FrameCount(state.handle);
    if (frame == count)
        return false;
    runtime.NextFrame(state.handle);
    return true;
}

}  // namespace pal4
