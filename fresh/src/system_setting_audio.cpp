#include "pal4/system_setting_audio.h"

namespace pal4 {
namespace {

void VisitStreams(const std::uint32_t audio,
                  const std::uint32_t begin_offset,
                  ISystemSettingAudioRuntime& runtime,
                  const std::uint32_t type,
                  const float volume) {
    const auto begin = runtime.ReadWord(audio + begin_offset);
    if (begin == 0u) {
        return;
    }
    const auto end = runtime.ReadWord(audio + begin_offset + 4u);
    for (auto current = begin; current < end; current += 4u) {
        const auto stream = runtime.ReadWord(current);
        if (stream == 0u) {
            continue;
        }
        if (type == 0u) {
            runtime.SetStreamVolume(stream, volume);
        } else if (runtime.StreamType(stream) == type) {
            runtime.RefreshStream(stream);
        }
    }
}

}  // namespace

std::uint8_t SystemSettingAudio::OnBgmVolumeChanged(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingAudioRuntime& runtime) {
    (void)event;
    const auto slider = runtime.ReadWord(frame + 496u);
    const auto value = 1.0f - runtime.ReadFloat(slider + 1200u);
    const auto audio = runtime.AudioManager();
    runtime.WriteFloat(audio, value);
    VisitStreams(audio, 36u, runtime, 0u, value);
    return 1u;
}

std::uint8_t SystemSettingAudio::OnSfxVolumeChanged(
    const std::uint32_t frame, const std::uint32_t event,
    ISystemSettingAudioRuntime& runtime) {
    (void)event;
    auto slider = runtime.ReadWord(frame + 500u);
    auto value = 1.0f - runtime.ReadFloat(slider + 1200u);
    auto audio = runtime.AudioManager();
    runtime.WriteFloat(audio + 8u, value);
    VisitStreams(audio, 68u, runtime, 2u, value);

    slider = runtime.ReadWord(frame + 500u);
    value = 1.0f - runtime.ReadFloat(slider + 1200u);
    audio = runtime.AudioManager();
    runtime.WriteFloat(audio + 4u, value);
    VisitStreams(audio, 52u, runtime, 1u, value);
    return 1u;
}

std::uint8_t SystemSettingAudio::PlayButtonSound(
    const std::uint32_t event, ISystemSettingAudioRuntime& runtime) {
    (void)event;
    runtime.PlayMusic("WA01.wav", 1u);
    return 1u;
}

}  // namespace pal4
