#include "pal4/audio_sample.h"

#include <bit>

namespace pal4 {

AudioSampleState& AudioSample::Initialize(
    AudioSampleState& sample, const std::uint32_t parent,
    IAudioSampleRuntime& runtime) {
    sample.parent = parent;
    sample.vtable = kBaseVtable;
    sample.audio_file = 0u;
    sample.sample = 0u;
    sample.loop_count = 1u;
    sample.volume_bits = 0x3f800000u;
    runtime.InitializePath(sample);
    sample.vtable = kSampleVtable;
    return sample;
}

std::uint32_t AudioSample::Cleanup(
    AudioSampleState& sample, IAudioSampleControlRuntime& runtime) {
    sample.vtable = kSampleVtable;
    (void)Stop(sample, runtime);
    if (sample.audio_file != 0u) {
        const auto crc = runtime.FileCrc(sample.audio_file);
        runtime.ReleasePoolFile(crc);
        sample.audio_file = 0u;
    }
    const auto result = runtime.CleanupPath(sample);
    sample.vtable = kBaseVtable;
    return result;
}

std::uint32_t AudioSample::Stop(
    AudioSampleState& sample, IAudioSampleControlRuntime& runtime) {
    const auto handle = sample.sample;
    if (handle == 0u)
        return 0u;
    runtime.EndMilesSample(handle);
    const auto result = runtime.RecycleMilesSample(handle);
    sample.sample = 0u;
    return result;
}

std::uint32_t AudioSample::StopImmediate(
    const AudioSampleState& sample, IAudioSampleControlRuntime& runtime) {
    return runtime.StopMilesSample(sample.sample);
}

std::uint32_t AudioSample::Resume(
    const AudioSampleState& sample, IAudioSampleControlRuntime& runtime) {
    if (sample.sample == 0u)
        return 0u;
    return runtime.ResumeMilesSample(sample.sample);
}

bool AudioSample::IsPlaying(
    const AudioSampleState& sample, IAudioSampleControlRuntime& runtime) {
    return runtime.MilesSampleStatus(sample.sample) == 4u;
}

std::uint32_t AudioSample::ApplyVolume(
    const AudioSampleState& sample, IAudioSampleControlRuntime& runtime) {
    if (sample.sample == 0u)
        return 0u;
    const auto volume = runtime.SampleMasterVolume() *
        std::bit_cast<float>(sample.volume_bits);
    const auto bits = std::bit_cast<std::uint32_t>(volume);
    return runtime.SetMilesSampleVolume(sample.sample, bits, bits);
}

}  // namespace pal4
