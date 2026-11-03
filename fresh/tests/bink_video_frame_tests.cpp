#include "pal4/bink_video.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IBinkVideoRuntime {
public:
    std::uint32_t OpenBink(std::uint32_t, bool) override { return 0u; }
    std::uint32_t CloseBink(std::uint32_t) override { return 0u; }
    std::uint32_t SetBinkVolume(
        std::uint32_t handle, std::uint32_t track,
        std::uint32_t scaled) override {
        events.emplace_back("volume");
        observed_handle = handle;
        observed_track = track;
        observed_volume = scaled;
        return 0x41u;
    }
    std::uint32_t CreateRaster(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    std::uint32_t VideoWidth(std::uint32_t) override { return 0u; }
    std::uint32_t VideoHeight(std::uint32_t) override { return 0u; }
    std::uint8_t RasterFlags(std::uint32_t) override { return 0u; }
    void LogFailure(std::uint32_t) override {}
    bool IsPaused(std::uint32_t) override { return false; }
    std::uint32_t PauseBink(std::uint32_t, bool) override { return 0u; }
    std::uint32_t FrameNumber(std::uint32_t handle) override {
        events.emplace_back("frame-number");
        observed_handle = handle;
        return frame_number;
    }
    std::uint32_t FrameCount(std::uint32_t handle) override {
        events.emplace_back("frame-count");
        observed_handle = handle;
        return frame_count;
    }
    std::uint32_t WaitBink(std::uint32_t handle) override {
        events.emplace_back("wait");
        observed_handle = handle;
        return wait_result;
    }
    void DoFrame(std::uint32_t handle) override {
        events.emplace_back("decode");
        observed_handle = handle;
    }
    std::uint32_t RasterWidth(std::uint32_t raster) override {
        events.emplace_back("raster-width");
        observed_raster = raster;
        return 640u;
    }
    std::uint32_t RasterStride(std::uint32_t raster) override {
        events.emplace_back("raster-stride");
        observed_raster = raster;
        return 2560u;
    }
    std::uint32_t LockRaster(
        std::uint32_t raster, std::uint32_t level,
        std::uint32_t mode) override {
        events.emplace_back("lock");
        observed_raster = raster;
        Check(level == 0u && mode == 1u,
              "raster lock uses level zero and write mode");
        return locked_pixels;
    }
    void CopyToBuffer(
        std::uint32_t handle, std::uint32_t pixels,
        std::uint32_t stride, std::uint32_t width,
        std::int32_t x, std::int32_t y,
        std::uint32_t flags) override {
        events.emplace_back("copy");
        Check(handle == 0x100u && pixels == 0x300u &&
                  stride == 2560u && width == 640u &&
                  x == 80 && y == 60 && flags == 0x8000000au,
              "Bink copy forwards raster layout, offsets and high mode bit");
    }
    void UnlockRaster(std::uint32_t raster) override {
        events.emplace_back("unlock");
        observed_raster = raster;
    }
    void NextFrame(std::uint32_t handle) override {
        events.emplace_back("next");
        observed_handle = handle;
    }

    std::vector<std::string> events;
    std::uint32_t frame_number = 1u;
    std::uint32_t frame_count = 3u;
    std::uint32_t wait_result = 0u;
    std::uint32_t locked_pixels = 0x300u;
    std::uint32_t observed_handle = 0u;
    std::uint32_t observed_raster = 0u;
    std::uint32_t observed_track = 99u;
    std::uint32_t observed_volume = 0u;
};

void VolumeAndStatus() {
    Runtime runtime;
    pal4::BinkVideoState state{};
    Check(pal4::BinkVideo::SetVolume(state, 0.5f, runtime) == 0u &&
              !pal4::BinkVideo::IsPlaying(state, runtime) &&
              !pal4::BinkVideo::WaitFrame(state, runtime) &&
              runtime.events.empty(),
          "empty handle gates volume, status and wait");
    state.handle = 0x100u;
    Check(pal4::BinkVideo::SetVolume(state, -1.0f, runtime) == 0x41u &&
              runtime.observed_volume == 0u &&
              runtime.observed_track == 0u &&
              runtime.observed_handle == 0x100u,
          "negative volume clamps to zero");
    Check(pal4::BinkVideo::SetVolume(state, 0.5f, runtime) == 0x41u &&
              runtime.observed_volume == 32767u,
          "half volume truncates scaled Bink units");
    Check(pal4::BinkVideo::SetVolume(state, 2.0f, runtime) == 0x41u &&
              runtime.observed_volume == 65535u,
          "volume above one clamps to maximum");
    Check(pal4::BinkVideo::SetVolume(
              state, std::numeric_limits<float>::quiet_NaN(),
              runtime) == 0x41u &&
              runtime.observed_volume == 0x80000000u,
          "NaN volume has defined integer conversion sentinel");
    runtime.events.clear();
    Check(pal4::BinkVideo::IsPlaying(state, runtime) &&
              runtime.events == std::vector<std::string>{
                  "frame-number", "frame-count"},
          "unequal frame number and count is playing");
    runtime.frame_number = runtime.frame_count;
    Check(!pal4::BinkVideo::IsPlaying(state, runtime),
          "last frame reports stopped");
    runtime.events.clear();
    runtime.wait_result = 1u;
    Check(pal4::BinkVideo::WaitFrame(state, runtime) &&
              runtime.events == std::vector<std::string>{"wait"},
          "wait recognizes Bink result one");
    runtime.wait_result = 2u;
    Check(!pal4::BinkVideo::WaitFrame(state, runtime),
          "other wait result means frame is ready");
}

void RenderAndUpdate() {
    Runtime runtime;
    pal4::BinkVideoState state{};
    Check(!pal4::BinkVideo::RenderFrame(state, runtime) &&
              !pal4::BinkVideo::Update(state, runtime) &&
              runtime.events.empty(),
          "empty state cannot render or update");
    state.handle = 0x100u;
    Check(!pal4::BinkVideo::RenderFrame(state, runtime) &&
              runtime.events.empty(),
          "missing raster prevents decode");
    state.raster = 0x200u;
    state.copy_mode = 10u;
    state.offset_x_bits = 0x42a00000u;
    state.offset_y_bits = 0x42700000u;
    runtime.locked_pixels = 0u;
    Check(!pal4::BinkVideo::RenderFrame(state, runtime) &&
              runtime.events == std::vector<std::string>{
                  "decode", "raster-width", "lock"},
          "lock failure stops before copy and unlock");
    runtime.events.clear();
    runtime.locked_pixels = 0x300u;
    Check(pal4::BinkVideo::RenderFrame(state, runtime) &&
              runtime.events == std::vector<std::string>{
                  "decode", "raster-width", "lock", "raster-stride",
                  "copy", "unlock", "frame-number", "frame-count", "next"} &&
              runtime.observed_raster == 0x200u,
          "render copies and unlocks before advancing a nonfinal frame");
    runtime.events.clear();
    runtime.frame_number = runtime.frame_count;
    Check(!pal4::BinkVideo::RenderFrame(state, runtime) &&
              runtime.events.back() == "frame-count",
          "last rendered frame does not advance");
    runtime.events.clear();
    runtime.wait_result = 1u;
    Check(!pal4::BinkVideo::Update(state, runtime) &&
              runtime.events == std::vector<std::string>{
                  "wait", "frame-number", "frame-count"},
          "waiting update skips render and reports final status");
    runtime.events.clear();
    runtime.wait_result = 0u;
    runtime.frame_number = 1u;
    Check(pal4::BinkVideo::Update(state, runtime) &&
              runtime.events.front() == "wait" &&
              runtime.events[runtime.events.size() - 3u] == "next" &&
              runtime.events[runtime.events.size() - 2u] == "frame-number" &&
              runtime.events.back() == "frame-count",
          "ready update renders then queries playback status");
}

}  // namespace

int main() {
    VolumeAndStatus();
    RenderAndUpdate();
    std::cout << "Bink video frame tests passed\n";
}
