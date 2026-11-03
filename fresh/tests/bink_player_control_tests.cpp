#include "pal4/bink_player.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void Put(std::byte* object, std::size_t offset, std::uint32_t value) {
    std::memcpy(object + offset, &value, sizeof(value));
}

std::uint32_t Get(const std::byte* object, std::size_t offset) {
    std::uint32_t value = 0u;
    std::memcpy(&value, object + offset, sizeof(value));
    return value;
}

class Runtime final : public pal4::IBinkPlayerPlaybackRuntime {
public:
    void EnsureSingletonManager() override {}
    std::uint32_t RegisterSingleton(
        std::string_view, std::uint32_t,
        std::function<std::byte*()>,
        std::function<std::uint32_t(std::byte*)>) override { return 0u; }
    std::byte* Allocate(std::size_t) override { return nullptr; }
    std::uint32_t Free(std::byte*) override { return 0u; }
    void InitializePackage(std::byte*) override {}
    std::uint32_t ClosePackage(std::byte* package) override {
        const auto first = package == object +
            pal4::BinkPlayer::kFirstPackageOffset;
        Check(first || package == object +
                  pal4::BinkPlayer::kSecondPackageOffset,
              "package close uses native slot offset");
        events.emplace_back(first ? "close-first" : "close-second");
        if (clear_package_flags)
            object[first ? pal4::BinkPlayer::kFirstPackageOpenFlag :
                           pal4::BinkPlayer::kSecondPackageOpenFlag] =
                std::byte{0};
        return first ? 0x71u : 0x72u;
    }
    std::uint32_t CleanupPackage(std::byte*) override { return 0u; }
    float VideoMasterVolume() override { return 0.0f; }
    bool UseVideoPackages() override { return false; }
    std::string ScriptVideoName(std::uint32_t) override { return {}; }
    void ResetPackageGlobalPointer(
        std::byte*, std::uint32_t) override {}
    bool FileExists(std::string_view) override { return false; }
    std::string InstallRoot() override { return {}; }
    void PromptForDisk(char) override {}
    bool OpenPackage(std::byte*, std::string_view) override { return false; }
    std::uint32_t OpenPackageFile(
        std::byte*, std::string_view) override { return 0u; }
    void SeekPackageFile(
        std::byte*, std::uint32_t, std::uint32_t,
        std::uint32_t) override {}
    std::uint32_t PackageBinkSource(std::byte*) override { return 0u; }
    void ClosePackageFile(std::byte*, std::uint32_t) override {}
    std::uint32_t ViewportWidth() override {
        events.emplace_back("viewport-width");
        return 800u;
    }
    std::uint32_t ViewportHeight() override {
        events.emplace_back("viewport-height");
        return 600u;
    }
    std::uint32_t CreateTexture(std::uint32_t raster) override {
        events.emplace_back("texture-create");
        Check(raster == 0x300u, "texture receives selected raster");
        return 0x400u;
    }
    void DestroyTexture(std::uint32_t texture) override {
        events.emplace_back("texture-destroy");
        Check(texture == 0x400u, "texture destroy receives owned texture");
    }
    void InvokeCloseCallback(std::uint32_t callback) override {
        events.emplace_back("callback");
        Check(callback == 0x800u, "close invokes stored callback token");
    }
    void LogSoundSetup(std::uint32_t phase) override {
        events.emplace_back(phase == 115u ? "sound-before" : "sound-after");
    }
    bool SetBinkSoundSystem(bool miles,
                            std::uint32_t device) override {
        events.emplace_back("sound-system");
        observed_miles = miles;
        observed_device = device;
        return sound_success;
    }
    void DrawTexturedRectangle(
        std::uint32_t context, std::uint32_t x,
        std::uint32_t y, std::uint32_t width,
        std::uint32_t height, std::uint32_t texture,
        std::uint32_t alpha, std::uint32_t red,
        std::uint32_t green, std::uint32_t blue) override {
        events.emplace_back("draw");
        Check(context == 0x900u && x == 0u && y == 0u &&
                  width == 800u && height == 600u &&
                  texture == 0x400u && alpha == 255u &&
                  red == 255u && green == 255u && blue == 255u,
              "video draw covers viewport with opaque white tint");
    }

    std::uint32_t OpenBink(std::uint32_t, bool) override { return 0u; }
    std::uint32_t CloseBink(std::uint32_t handle) override {
        events.emplace_back("bink-close");
        observed_handle = handle;
        return 0x41u;
    }
    std::uint32_t SetBinkVolume(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t type) override {
        events.emplace_back("raster-create");
        raster_dimensions = {width, height};
        Check(depth == 0u && type == 5u,
              "video raster uses depth zero and type five");
        return 0x300u;
    }
    std::uint32_t VideoWidth(std::uint32_t) override {
        events.emplace_back("video-width");
        return 640u;
    }
    std::uint32_t VideoHeight(std::uint32_t) override {
        events.emplace_back("video-height");
        return 480u;
    }
    std::uint8_t RasterFlags(std::uint32_t) override {
        events.emplace_back("raster-flags");
        return 5u;
    }
    void LogFailure(std::uint32_t) override {
        events.emplace_back("log-failure");
    }
    bool IsPaused(std::uint32_t handle) override {
        events.emplace_back("is-paused");
        observed_handle = handle;
        return paused;
    }
    std::uint32_t PauseBink(
        std::uint32_t handle, bool pause) override {
        events.emplace_back("pause");
        observed_handle = handle;
        requested_pause = pause;
        return 0x42u;
    }
    std::uint32_t FrameNumber(std::uint32_t) override {
        events.emplace_back("frame-number");
        return frame_number;
    }
    std::uint32_t FrameCount(std::uint32_t) override {
        events.emplace_back("frame-count");
        return frame_count;
    }
    std::uint32_t WaitBink(std::uint32_t) override {
        events.emplace_back("wait");
        return 1u;
    }
    void DoFrame(std::uint32_t) override {}
    std::uint32_t RasterWidth(std::uint32_t) override { return 0u; }
    std::uint32_t RasterStride(std::uint32_t) override { return 0u; }
    std::uint32_t LockRaster(
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    void CopyToBuffer(
        std::uint32_t, std::uint32_t, std::uint32_t,
        std::uint32_t, std::int32_t, std::int32_t,
        std::uint32_t) override {}
    void UnlockRaster(std::uint32_t) override {}
    void NextFrame(std::uint32_t) override {}

    std::byte* object = nullptr;
    std::vector<std::string> events;
    std::array<std::uint32_t, 2u> raster_dimensions{};
    std::uint32_t observed_handle = 0u;
    std::uint32_t observed_device = 0u;
    std::uint32_t frame_number = 1u;
    std::uint32_t frame_count = 3u;
    bool paused = false;
    bool requested_pause = false;
    bool observed_miles = false;
    bool sound_success = true;
    bool clear_package_flags = true;
};

void PauseSoundAndParameters() {
    std::vector<std::byte> storage(pal4::BinkPlayer::kSize);
    auto* object = storage.data();
    Runtime runtime;
    runtime.object = object;
    Check(pal4::BinkPlayer::TogglePause(object, runtime) == 0u &&
              runtime.events.empty(),
          "empty nested video cannot pause");
    Put(object, 4u, 0x100u);
    Check(pal4::BinkPlayer::TogglePause(object, runtime) == 0x42u &&
              runtime.requested_pause &&
              runtime.observed_handle == 0x100u,
          "player pause forwards nested Bink handle");
    runtime.paused = true;
    Check(pal4::BinkPlayer::TogglePause(object, runtime) == 0x42u &&
              !runtime.requested_pause,
          "player pause toggles active Bink state");
    runtime.events.clear();
    Check(pal4::BinkPlayer::SetSoundSystem(
              object, true, 0x777u, runtime) &&
              object[0] == std::byte{1} &&
              runtime.observed_miles &&
              runtime.observed_device == 0x777u &&
              runtime.events == std::vector<std::string>{
                  "sound-before", "sound-system", "sound-after"},
          "Miles sound setup sets player flag and logs around API call");
    runtime.events.clear();
    runtime.sound_success = false;
    Check(!pal4::BinkPlayer::SetSoundSystem(
              object, false, 0x778u, runtime) &&
              !runtime.observed_miles &&
              runtime.observed_device == 0x778u,
          "DirectSound setup forwards failure");
    runtime.events.clear();
    Check(pal4::BinkPlayer::SetVideoParameters(
              object, true, runtime) == 0x400u &&
              runtime.raster_dimensions ==
                  std::array<std::uint32_t, 2u>{640u, 480u} &&
              Get(object, 24u) == 0x400u &&
              Get(object, 12u) == 3u &&
              Get(object, 16u) == 0u &&
              Get(object, 20u) == 0u,
          "native-size setup creates texture at video dimensions");
    runtime.events.clear();
    Check(pal4::BinkPlayer::SetVideoParameters(
              object, false, runtime) == 0x400u &&
              runtime.raster_dimensions ==
                  std::array<std::uint32_t, 2u>{800u, 600u} &&
              Get(object, 16u) == 0x42a00000u &&
              Get(object, 20u) == 0x42700000u,
          "viewport setup uses max dimensions and centered half gaps");
}

void CloseCleanupAndRender() {
    std::vector<std::byte> storage(pal4::BinkPlayer::kSize);
    auto* object = storage.data();
    Runtime runtime;
    runtime.object = object;
    Put(object, 4u, 0x100u);
    Put(object, 8u, 0x300u);
    Put(object, 24u, 0x400u);
    Put(object, pal4::BinkPlayer::kSize - 4u, 0x800u);
    object[pal4::BinkPlayer::kFirstPackageOpenFlag] = std::byte{1};
    object[pal4::BinkPlayer::kSecondPackageOpenFlag] = std::byte{1};
    Check(pal4::BinkPlayer::CloseVideo(object, runtime) == 0x72u &&
              runtime.events == std::vector<std::string>{
                  "bink-close", "texture-destroy", "callback",
                  "close-first", "close-second"} &&
              Get(object, 4u) == 0u && Get(object, 8u) == 0u &&
              Get(object, 24u) == 0u &&
              Get(object, pal4::BinkPlayer::kSize - 4u) == 0u,
          "close releases video, texture, callback then both packages");
    runtime.events.clear();
    Put(object, pal4::BinkPlayer::kSize - 4u, 0x800u);
    Put(object, 24u, 0x400u);
    object[pal4::BinkPlayer::kFirstPackageOpenFlag] = std::byte{1};
    object[pal4::BinkPlayer::kSecondPackageOpenFlag] = std::byte{1};
    Check(pal4::BinkPlayer::CleanupPlayback(object, runtime) == 0u &&
              std::find(runtime.events.begin(), runtime.events.end(),
                        "callback") == runtime.events.end() &&
              runtime.events == std::vector<std::string>{
                  "texture-destroy", "close-first", "close-second"},
          "playback cleanup clears callback before close and rechecks flags");
    runtime.events.clear();
    object[0] = std::byte{0};
    pal4::BinkPlayer::UpdateAndRender(object, 0x900u, runtime);
    Check(runtime.events.empty(), "inactive player skips update and draw");
    object[0] = std::byte{1};
    Put(object, 4u, 0x100u);
    Put(object, 24u, 0x400u);
    pal4::BinkPlayer::UpdateAndRender(object, 0x900u, runtime);
    Check(runtime.events == std::vector<std::string>{
              "wait", "frame-number", "frame-count",
              "viewport-width", "viewport-height", "draw"},
          "active player draws current video across viewport");
    runtime.events.clear();
    runtime.frame_number = runtime.frame_count;
    pal4::BinkPlayer::UpdateAndRender(object, 0x900u, runtime);
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "texture-destroy") != runtime.events.end() &&
              Get(object, 24u) == 0u,
          "ended video closes owned texture");
}

}  // namespace

int main() {
    PauseSoundAndParameters();
    CloseCleanupAndRender();
    std::cout << "Bink player control tests passed\n";
}
