#include "pal4/background_music_player_lifecycle.h"

#include <initializer_list>

namespace pal4 {
namespace {

std::uint32_t ReleaseText(const std::uint32_t address,
                          IBackgroundMusicLifecycleRuntime& runtime) {
    const auto data = runtime.ReadWord(address + 4u);
    std::uint32_t result = 0u;
    if (data != 0u) {
        const auto header = data - 1u;
        const auto references = runtime.ReadByte(header);
        if (references == 0u || references == 0xffu)
            result = runtime.Free(header);
        else {
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(references - 1u));
            result = header;
        }
    }
    runtime.WriteWord(address + 4u, 0u);
    runtime.WriteWord(address + 8u, 0u);
    runtime.WriteWord(address + 12u, 0u);
    return result;
}

}  // namespace

std::uint32_t BackgroundMusicSingleton::GetInstance(
    IBackgroundMusicSingletonRuntime& runtime) {
    if ((state_.initialization_bits & 1u) != 0u) return state_.instance;
    state_.initialization_bits |= 1u;
    const auto manager = runtime.GetSingletonManager();
    state_.instance = runtime.Register(
        manager, "palBackgroundMusicPlayer", 1044u,
        kFactoryAddress, kDestructorAddress);
    return state_.instance;
}

std::uint32_t BackgroundMusicPlayerLifecycle::Create(
    IBackgroundMusicLifecycleRuntime& runtime) {
    const auto object = runtime.Allocate(kObjectBytes);
    return object == 0u ? 0u : Construct(object, runtime);
}

std::uint32_t BackgroundMusicPlayerLifecycle::Destroy(
    const std::uint32_t object,
    IBackgroundMusicLifecycleRuntime& runtime) {
    if (object == 0u) return 0u;
    static_cast<void>(CleanupResources(object, runtime));
    return runtime.Free(object);
}

std::uint32_t BackgroundMusicPlayerLifecycle::CleanupResources(
    const std::uint32_t object,
    IBackgroundMusicLifecycleRuntime& runtime) {
    static_cast<void>(ReleaseText(object + 32u, runtime));
    static_cast<void>(ReleaseText(object + 16u, runtime));
    return ReleaseText(object, runtime);
}

std::uint32_t BackgroundMusicPlayerLifecycle::Construct(
    const std::uint32_t object,
    IBackgroundMusicLifecycleRuntime& runtime) {
    for (const auto offset : {0u, 16u, 32u}) {
        runtime.WriteByte(object + offset, 0u);
        runtime.WriteWord(object + offset + 4u, 0u);
        runtime.WriteWord(object + offset + 8u, 0u);
        runtime.WriteWord(object + offset + 12u, 0u);
    }
    for (const auto offset : {48u, 52u, 56u, 60u})
        runtime.WriteWord(object + offset, 0u);
    return object;
}

}  // namespace pal4
