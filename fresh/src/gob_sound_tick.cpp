#include "pal4/gob_sound.h"
#include "pal4/gob_sound_tick.h"

#include <cstdint>

namespace pal4 {

std::uint32_t GobSound::TickAndMaybePlay3D(
    const std::uint32_t object, const float elapsed,
    IGobSoundTickRuntime& runtime) {
    const auto config = runtime.GetSoundConfigNode(object);
    if (config == 0u || runtime.ReadWord(object + 0x1dcu) != 0u)
        return config;
    const auto handle = object + kAudioHandleOffset;
    if (!runtime.IsAudioPlaying(handle))
        runtime.WriteWord(object + 0x204u, 1u);

    const auto camera = runtime.ActiveCameraPosition();
    const GobSoundVector emitter{
        runtime.ReadFloat(object + 0xacu),
        runtime.ReadFloat(object + 0xb0u),
        runtime.ReadFloat(object + 0xb4u)};
    const GobSoundVector relative{
        camera.x - emitter.x,
        emitter.y - emitter.y, // IDA5A61A5..5A61B1 measures horizontal distance.
        camera.z - emitter.z};
    static_cast<void>(runtime.CalculateVectorMagnitude(relative));
    const auto distance = runtime.CalculateVectorMagnitude(relative);
    if (runtime.AudibleRadius(object) >= distance &&
        runtime.ReadWord(object + 0x204u) != 0u) {
        const auto accumulated = runtime.ReadFloat(object + 0x200u) + elapsed;
        runtime.WriteFloat(object + 0x200u, accumulated);
        if (accumulated >= runtime.ReadFloat(object + 0x1fcu)) {
            runtime.WriteFloat(object + 0x200u, 0.0f);
            const auto delay = runtime.RandomDelay(
                runtime.ReadFloat(object + 0x1f4u),
                runtime.ReadFloat(object + 0x1f8u),
                static_cast<float>(runtime.TimeMilliseconds()));
            runtime.WriteFloat(object + 0x1fcu, delay);
            runtime.WriteWord(object + 0x204u, 0u);
            if (!runtime.IsAudioPlaying(handle)) {
                const auto name = runtime.ReadCString(
                    runtime.ReadWord(object + 0x1e8u));
                if (!name.empty()) {
                    const auto path = runtime.BuildSoundPath(name);
                    const auto signed_uses = static_cast<std::int32_t>(
                        runtime.ReadWord(object + 0x198u));
                    const auto source = signed_uses < 0 ? camera : relative;
                    const auto count = signed_uses < 0 ? 0u :
                        static_cast<std::uint32_t>(signed_uses);
                    const auto near_radius = runtime.AudibleRadius(object) *
                        0.25f;
                    const auto radius = runtime.AudibleRadius(object);
                    runtime.WriteWord(handle, runtime.Play3DSound(
                        source, path, emitter, count, radius, near_radius));
                }
            }
        }
    }
    runtime.ApplyMovement(object, {0.0f, elapsed * 200.0f, 0.0f}, 1u);
    return runtime.UpdateClumpIfVisible(object, elapsed);
}

}  // namespace pal4
