#include "pal4/bink_video.h"

#include <cstdlib>
#include <iostream>
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
    std::uint32_t OpenBink(std::uint32_t source,
                           bool use_io_table) override {
        events.emplace_back("open");
        observed_source = source;
        observed_io_table = use_io_table;
        return opened_handle;
    }
    std::uint32_t CloseBink(std::uint32_t handle) override {
        events.emplace_back("close");
        observed_handle = handle;
        return 0x31u;
    }
    std::uint32_t SetBinkVolume(
        std::uint32_t handle, std::uint32_t track,
        std::uint32_t scaled_volume) override {
        events.emplace_back("volume");
        observed_volume = scaled_volume;
        observed_track = track;
        observed_handle = handle;
        return 0x33u;
    }
    std::uint32_t CreateRaster(std::uint32_t width,
                                std::uint32_t height,
                                std::uint32_t depth,
                                std::uint32_t type) override {
        events.emplace_back("raster-create");
        Check(width == 800u && height == 600u && depth == 0u && type == 5u,
              "raster creation uses requested dimensions and native flags");
        return created_raster;
    }
    std::uint32_t VideoWidth(std::uint32_t) override {
        events.emplace_back("width");
        return 640u;
    }
    std::uint32_t VideoHeight(std::uint32_t) override {
        events.emplace_back("height");
        return 480u;
    }
    std::uint8_t RasterFlags(std::uint32_t raster) override {
        events.emplace_back("raster-flags");
        Check(raster == created_raster, "format probe uses new raster");
        return raster_flags;
    }
    void LogFailure(std::uint32_t code) override {
        events.emplace_back("log");
        logged_code = code;
    }
    bool IsPaused(std::uint32_t) override {
        events.emplace_back("is-paused");
        return paused;
    }
    std::uint32_t PauseBink(std::uint32_t handle,
                             bool pause) override {
        events.emplace_back("pause");
        observed_handle = handle;
        requested_pause = pause;
        return 0x32u;
    }
    std::uint32_t FrameNumber(std::uint32_t) override { return 0u; }
    std::uint32_t FrameCount(std::uint32_t) override { return 0u; }
    std::uint32_t WaitBink(std::uint32_t) override { return 0u; }
    void DoFrame(std::uint32_t) override {}
    std::uint32_t RasterWidth(std::uint32_t) override { return 0u; }
    std::uint32_t RasterStride(std::uint32_t) override { return 0u; }
    std::uint32_t LockRaster(
        std::uint32_t, std::uint32_t, std::uint32_t) override { return 0u; }
    void CopyToBuffer(
        std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
        std::int32_t, std::int32_t, std::uint32_t) override {}
    void UnlockRaster(std::uint32_t) override {}
    void NextFrame(std::uint32_t) override {}

    std::vector<std::string> events;
    std::uint32_t opened_handle = 0x100u;
    std::uint32_t created_raster = 0x200u;
    std::uint8_t raster_flags = 5u;
    std::uint32_t observed_source = 0u;
    std::uint32_t observed_handle = 0u;
    std::uint32_t logged_code = 0u;
    std::uint32_t observed_volume = 0u;
    std::uint32_t observed_track = 99u;
    bool observed_io_table = false;
    bool paused = false;
    bool requested_pause = false;
};

void InitializeOpenAndClose() {
    pal4::BinkVideoState state{0x111u, 0x222u, 9u,
                                0x333u, 0x444u};
    Check(&pal4::BinkVideo::Initialize(state) == &state &&
              state.handle == 0u && state.raster == 0u &&
              state.copy_mode == 9u && state.offset_x_bits == 0u &&
              state.offset_y_bits == 0u,
          "video initializer clears four words and preserves mode");
    Runtime runtime;
    state.handle = 0x55u;
    state.raster = 0x66u;
    Check(pal4::BinkVideo::OpenDefault(
              state, 0x700u, 0.25f, runtime) &&
              state.handle == 0x100u && state.raster == 0u &&
              runtime.observed_io_table &&
              runtime.observed_source == 0x700u &&
              runtime.observed_handle == 0x100u &&
              runtime.observed_volume == 16383u &&
              runtime.observed_track == 0u &&
              runtime.events == std::vector<std::string>{
                  "close", "open", "volume"},
          "default open closes prior handle and applies volume");
    runtime.events.clear();
    runtime.opened_handle = 0u;
    Check(!pal4::BinkVideo::Open(
              state, 0x701u, 0.5f, false, runtime) &&
              state.handle == 0u && !runtime.observed_io_table &&
              runtime.events == std::vector<std::string>{
                  "close", "open"},
          "failed direct open leaves empty state without volume call");
    runtime.events.clear();
    Check(pal4::BinkVideo::Close(state, runtime) == 0u &&
              runtime.events.empty(),
          "closing empty state does not call Bink");
    state.handle = 0x80u;
    state.raster = 0x90u;
    Check(pal4::BinkVideo::Close(state, runtime) == 0x31u &&
              state.handle == 0u && state.raster == 0u &&
              runtime.observed_handle == 0x80u,
          "close forwards result and clears handle plus raster");
}

void RasterModeAndParameters() {
    Runtime runtime;
    pal4::BinkVideoState state{};
    Check(pal4::BinkVideo::SetPlaybackMode(
              state, 0x200u, runtime) == 0u &&
              runtime.events.empty(),
          "no handle prevents raster mode changes");
    state.handle = 0x100u;
    Check(pal4::BinkVideo::SetPlaybackMode(
              state, 0u, runtime) == 0u &&
              runtime.logged_code == 118u && state.raster == 0u,
          "null raster reports setup failure");
    runtime.events.clear();
    runtime.raster_flags = 5u;
    Check(pal4::BinkVideo::SetPlaybackMode(
              state, 0x200u, runtime) == 0x200u &&
              state.copy_mode == 3u,
          "raster flags one or four choose copy mode three");
    runtime.raster_flags = 2u;
    Check(pal4::BinkVideo::SetPlaybackMode(
              state, 0x200u, runtime) == 0x200u &&
              state.copy_mode == 10u,
          "raster flag two chooses copy mode ten");
    runtime.raster_flags = 0u;
    state.copy_mode = 7u;
    Check(pal4::BinkVideo::SetPlaybackMode(
              state, 0x200u, runtime) == 0x200u &&
              state.copy_mode == 7u && runtime.logged_code == 119u,
          "unsupported raster flags log and preserve prior mode");
    runtime.events.clear();
    Check(pal4::BinkVideo::SetParameters(
              state, 639u, 600u, true, runtime) == 0u &&
              runtime.logged_code == 117u &&
              runtime.events == std::vector<std::string>{
                  "width", "height", "log"},
          "too small target does not create raster");
    runtime.events.clear();
    runtime.raster_flags = 2u;
    Check(pal4::BinkVideo::SetParameters(
              state, 800u, 600u, true, runtime) == 0x200u &&
              state.offset_x_bits == 0x42a00000u &&
              state.offset_y_bits == 0x42700000u &&
              state.copy_mode == 10u &&
              runtime.events == std::vector<std::string>{
                  "width", "height", "raster-create", "raster-flags"},
          "centered raster stores half gaps and chooses format");
    runtime.events.clear();
    state.offset_x_bits = 0x111u;
    state.offset_y_bits = 0x222u;
    Check(pal4::BinkVideo::SetParameters(
              state, 800u, 600u, false, runtime) == 0x200u &&
              state.offset_x_bits == 0x111u &&
              state.offset_y_bits == 0x222u,
          "noncentered raster preserves previous offsets");
}

void PauseToggle() {
    Runtime runtime;
    pal4::BinkVideoState state{};
    Check(pal4::BinkVideo::TogglePause(state, runtime) == 0u &&
              runtime.events.empty(),
          "pause without handle is inert");
    state.handle = 0x123u;
    Check(pal4::BinkVideo::TogglePause(state, runtime) == 0x32u &&
              runtime.requested_pause && runtime.observed_handle == 0x123u,
          "unpaused video requests pause");
    runtime.paused = true;
    Check(pal4::BinkVideo::TogglePause(state, runtime) == 0x32u &&
              !runtime.requested_pause,
          "paused video requests resume");
}

}  // namespace

int main() {
    InitializeOpenAndClose();
    RasterModeAndParameters();
    PauseToggle();
    std::cout << "Bink video tests passed\n";
}
