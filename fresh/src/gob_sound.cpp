#include "pal4/gob_sound.h"

#include <array>

namespace pal4 {

std::uint32_t GobSound::Create(IGobSoundRuntime& runtime) {
    const auto object = runtime.Allocate(kSize);
    if (object == 0u) return 0u;
    try {
        return Construct(object, runtime);
    } catch (...) {
        runtime.Free(object);
        throw;
    }
}

std::uint32_t GobSound::Construct(
    const std::uint32_t object, IGobSoundConstructRuntime& runtime) {
    runtime.ConstructBase(object);
    runtime.WriteWord(object + kAudioHandleOffset, 0u);
    runtime.InitializeNameRecord(object + kNameRecordOffset);
    for (const auto offset : std::array<std::uint32_t, 3>{
             0x1e8u, 0x1ecu, 0x1f0u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object + 0x170u, 0u);
    for (const auto offset : std::array<std::uint32_t, 4>{
             0x1f4u, 0x1fcu, 0x200u, 0x204u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object + 0x1dcu, 0u);
    runtime.WriteWord(object, kVtable);
    runtime.WriteWord(object + 0x1f8u, 0x3f800000u);
    return object;
}

void GobSound::NoOp(std::uint32_t) noexcept {}


std::uint32_t GobSound::TypeKey() noexcept { return 3u; }

void GobSound::Destruct(const std::uint32_t object,
                        IGobSoundConstructRuntime& runtime) {
    runtime.WriteWord(object, kVtable);
    runtime.ReleaseNameRecord(object + kNameRecordOffset);
    runtime.WriteWord(object + 0x1e8u, 0u);
    runtime.WriteWord(object + 0x1ecu, 0u);
    runtime.WriteWord(object + 0x1f0u, 0u);
    runtime.DestructBase(object);
}

std::uint32_t GobSound::CleanupResources(
    const std::uint32_t object, IGobSoundCleanupRuntime& runtime) {
    for (const auto offset : std::array<std::uint32_t, 4>{
             0x1f4u, 0x1fcu, 0x200u, 0x204u})
        runtime.WriteWord(object + offset, 0u);
    runtime.WriteWord(object + 0x1f8u, 0x3f800000u);
    runtime.CleanupBaseResources(object);
    return runtime.StopAndRemoveAudioHandle(object + kAudioHandleOffset);
}

}  // namespace pal4
