#include "pal4/bink_player.h"

#include <algorithm>
#include <cstring>

namespace pal4 {
namespace {

std::uint32_t ReadWord(const std::byte* object, const std::size_t offset) {
    std::uint32_t value = 0u;
    std::memcpy(&value, object + offset, sizeof(value));
    return value;
}

void WriteWord(std::byte* object, const std::size_t offset,
               const std::uint32_t value) {
    std::memcpy(object + offset, &value, sizeof(value));
}

BinkVideoState ReadVideo(const std::byte* object) {
    BinkVideoState video{};
    std::memcpy(&video, object + BinkPlayer::kVideoOffset,
                sizeof(video));
    return video;
}

void WriteVideo(std::byte* object, const BinkVideoState& video) {
    std::memcpy(object + BinkPlayer::kVideoOffset, &video,
                sizeof(video));
}

}  // namespace

std::uint32_t BinkPlayer::TogglePause(
    const std::byte* object, IBinkVideoRuntime& runtime) {
    return BinkVideo::TogglePause(ReadVideo(object), runtime);
}

std::uint8_t BinkPlayer::CloseVideo(
    std::byte* object, IBinkPlayerPlaybackRuntime& runtime) {
    auto video = ReadVideo(object);
    (void)BinkVideo::Close(video, runtime);
    WriteVideo(object, video);
    const auto texture = ReadWord(object, 24u);
    if (texture != 0u) {
        runtime.DestroyTexture(texture);
        WriteWord(object, 24u, 0u);
    }
    const auto callback = ReadWord(object, kSize - 4u);
    if (callback != 0u) {
        runtime.InvokeCloseCallback(callback);
        WriteWord(object, kSize - 4u, 0u);
    }
    if (object[kFirstPackageOpenFlag] != std::byte{0})
        (void)runtime.ClosePackage(object + kFirstPackageOffset);
    if (object[kSecondPackageOpenFlag] != std::byte{0})
        return static_cast<std::uint8_t>(
            runtime.ClosePackage(object + kSecondPackageOffset));
    return 0u;
}

bool BinkPlayer::SetSoundSystem(
    std::byte* object, const bool miles,
    const std::uint32_t device,
    IBinkPlayerPlaybackRuntime& runtime) {
    object[0] = std::byte{1};
    runtime.LogSoundSetup(115u);
    const auto success = runtime.SetBinkSoundSystem(miles, device);
    runtime.LogSoundSetup(116u);
    return success;
}

std::uint8_t BinkPlayer::CleanupPlayback(
    std::byte* object, IBinkPlayerPlaybackRuntime& runtime) {
    WriteWord(object, kSize - 4u, 0u);
    (void)CloseVideo(object, runtime);
    if (object[kFirstPackageOpenFlag] != std::byte{0})
        (void)runtime.ClosePackage(object + kFirstPackageOffset);
    if (object[kSecondPackageOpenFlag] != std::byte{0})
        return static_cast<std::uint8_t>(
            runtime.ClosePackage(object + kSecondPackageOffset));
    return 0u;
}

void BinkPlayer::UpdateAndRender(
    std::byte* object, const std::uint32_t context,
    IBinkPlayerPlaybackRuntime& runtime) {
    if (object[0] == std::byte{0})
        return;
    auto video = ReadVideo(object);
    const auto playing = BinkVideo::Update(video, runtime);
    WriteVideo(object, video);
    const auto texture = ReadWord(object, 24u);
    if (playing) {
        const auto width = runtime.ViewportWidth();
        const auto height = runtime.ViewportHeight();
        runtime.DrawTexturedRectangle(
            context, 0u, 0u, width, height, texture,
            255u, 255u, 255u, 255u);
    } else if (texture != 0u) {
        (void)CloseVideo(object, runtime);
    }
}

std::uint32_t BinkPlayer::SetVideoParameters(
    std::byte* object, const bool native_size,
    IBinkPlayerOpenRuntime& runtime) {
    auto video = ReadVideo(object);
    const auto width = runtime.VideoWidth(video.handle);
    const auto height = runtime.VideoHeight(video.handle);
    const auto target_width = native_size ? width :
        std::max(runtime.ViewportWidth(), width);
    const auto target_height = native_size ? height :
        std::max(runtime.ViewportHeight(), height);
    const auto raster = BinkVideo::SetParameters(
        video, target_width, target_height, true, runtime);
    WriteVideo(object, video);
    const auto texture = runtime.CreateTexture(raster);
    WriteWord(object, 24u, texture);
    return texture;
}

}  // namespace pal4
