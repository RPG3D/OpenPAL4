#include "pal4/bink_player.h"

#include "pal4/bink_video.h"

#include <cstring>

namespace pal4 {

std::uint32_t BinkPlayer::InitializeSingleton(
    std::uint8_t& guard, std::uint32_t& registration,
    IBinkPlayerRuntime& runtime) {
    if ((guard & 1u) != 0u)
        return registration;
    guard = static_cast<std::uint8_t>(guard | 1u);
    runtime.EnsureSingletonManager();
    registration = runtime.RegisterSingleton(
        "bnkBinkPlayer", 1007u,
        [&runtime]() { return Create(runtime); },
        [&runtime](std::byte* object) { return Destroy(object, runtime); });
    return registration;
}

std::byte* BinkPlayer::Create(IBinkPlayerRuntime& runtime) {
    auto* object = runtime.Allocate(kSize);
    return object == nullptr ? nullptr : Initialize(object, runtime);
}

std::uint32_t BinkPlayer::Destroy(
    std::byte* object, IBinkPlayerRuntime& runtime) {
    if (object == nullptr)
        return 0u;
    (void)Cleanup(object, runtime);
    return runtime.Free(object);
}

std::byte* BinkPlayer::Initialize(
    std::byte* object, IBinkPlayerRuntime& runtime) {
    object[0] = std::byte{0};
    BinkVideoState video{};
    std::memcpy(&video, object + kVideoOffset, sizeof(video));
    (void)BinkVideo::Initialize(video);
    std::memcpy(object + kVideoOffset, &video, sizeof(video));
    const std::uint32_t zero = 0u;
    std::memcpy(object + 24u, &zero, sizeof(zero));
    runtime.InitializePackage(object + kFirstPackageOffset);
    runtime.InitializePackage(object + kSecondPackageOffset);
    std::memcpy(object + kSize - sizeof(zero), &zero, sizeof(zero));
    return object;
}

std::uint32_t BinkPlayer::Cleanup(
    std::byte* object, IBinkPlayerRuntime& runtime) {
    if (object[kFirstPackageOpenFlag] != std::byte{0})
        (void)runtime.ClosePackage(object + kFirstPackageOffset);
    if (object[kSecondPackageOpenFlag] != std::byte{0})
        (void)runtime.ClosePackage(object + kSecondPackageOffset);
    (void)runtime.CleanupPackage(object + kSecondPackageOffset);
    return runtime.CleanupPackage(object + kFirstPackageOffset);
}

}  // namespace pal4
