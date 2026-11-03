#include "pal4/audio_3d_sample.h"

#include <bit>

namespace pal4 {

std::uint32_t Audio3DSample::Play(
    Audio3DSampleState& sample, const std::uint32_t miles_sample,
    const char* path, const void* device, const std::uint32_t loop_count,
    IAudio3DSamplePlayRuntime& runtime) {
    return PlayWithDistances(sample, miles_sample, path, device, loop_count,
                             sample.range60_bits, sample.range64_bits, runtime);
}

std::uint32_t Audio3DSample::PlayWithDistances(
    Audio3DSampleState& sample, const std::uint32_t miles_sample,
    const char* path, const void* device, const std::uint32_t loop_count,
    const std::uint32_t near_bits, const std::uint32_t far_bits,
    IAudio3DSamplePlayRuntime& runtime) {
    sample.range60_bits = near_bits;
    sample.range64_bits = far_bits;
    if (device == nullptr)
        return 3u;
    if (sample.sample != 0u)
        (void)Stop(sample, runtime);
    sample.sample = miles_sample;
    if (miles_sample == 0u)
        return 2u;

    if (sample.audio_file != 0u && runtime.FilePath(sample.audio_file) != path) {
        runtime.ReleasePoolFile(runtime.FileCrc(sample.audio_file));
        sample.audio_file = 0u;
    }
    if (sample.audio_file == 0u)
        sample.audio_file = runtime.AddPoolFile(path);
    if (sample.audio_file == 0u) {
        (void)Stop(sample, runtime);
        return 1u;
    }

    const auto data = runtime.FileData(sample.audio_file);
    std::uint32_t channels = 0u;
    std::uint32_t failure = 0u;
    if (!runtime.ReadWavInfo(data, channels))
        failure = 1u;
    else if (channels > 1u)
        failure = 2u;
    else if (!runtime.SetMiles3DSampleFile(sample.sample, data))
        failure = 3u;
    if (failure != 0u) {
        runtime.LogWavFailure(failure);
        runtime.ReleaseMiles3DSample(sample.sample);
        sample.sample = 0u;
        runtime.ReleasePoolFile(runtime.FileCrc(sample.audio_file));
        sample.audio_file = 0u;
        return failure == 3u ? 4u : 1u;
    }

    const auto volume = runtime.SampleMasterVolume() *
                        std::bit_cast<float>(sample.volume_bits);
    (void)runtime.SetMiles3DSampleVolume(
        sample.sample, std::bit_cast<std::uint32_t>(volume));
    sample.loop_count = loop_count;
    runtime.SetMiles3DSampleLoop(sample.sample, loop_count);
    (void)runtime.SetMiles3DPosition(
        sample.sample, sample.vector24[0], sample.vector24[1],
        std::bit_cast<std::uint32_t>(-
            std::bit_cast<float>(sample.vector24[2])));
    runtime.SetMiles3DSampleDistances(sample.sample, near_bits, far_bits);
    runtime.SetMiles3DSampleCone(sample.sample, 1119092736u, 1123024896u, 0u);
    runtime.SetMiles3DVelocity(
        sample.sample, sample.vector36[0], sample.vector36[1],
        std::bit_cast<std::uint32_t>(-
            std::bit_cast<float>(sample.vector36[2])), 0x3f800000u);
    runtime.AutoUpdateMiles3DPosition(sample.sample, 0u);
    runtime.StartMiles3DSample(sample.sample);
    const auto counter = runtime.PerformanceCounter();
    sample.started_at_words[0] = static_cast<std::uint32_t>(counter);
    sample.started_at_words[1] = static_cast<std::uint32_t>(counter >> 32u);
    runtime.AssignPath(sample, path);
    return 0u;
}

}  // namespace pal4
