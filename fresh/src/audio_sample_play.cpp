#include "pal4/audio_sample.h"

#include <string_view>

namespace pal4 {

std::uint32_t AudioSample::Play(
    AudioSampleState& sample, const std::uint32_t miles_sample,
    const char* path, const void* device,
    const std::uint32_t loop_count, IAudioSamplePlayRuntime& runtime) {
    if (device == nullptr)
        return 3u;
    if (sample.sample != 0u)
        (void)Stop(sample, runtime);
    sample.sample = miles_sample;
    if (miles_sample == 0u)
        return 2u;
    runtime.InitializeMilesSample(miles_sample);

    if (sample.audio_file != 0u &&
        runtime.FilePath(sample.audio_file) != path) {
        const auto crc = runtime.FileCrc(sample.audio_file);
        runtime.ReleasePoolFile(crc);
        sample.audio_file = 0u;
    }
    if (sample.audio_file == 0u)
        sample.audio_file = runtime.AddPoolFile(path);
    if (sample.audio_file == 0u) {
        (void)Stop(sample, runtime);
        return 1u;
    }

    if (!runtime.SetMilesSampleFile(
            sample.sample, runtime.FileData(sample.audio_file))) {
        (void)StopImmediate(sample, runtime);
        sample.sample = 0u;
        const auto crc = runtime.FileCrc(sample.audio_file);
        runtime.ReleasePoolFile(crc);
        sample.audio_file = 0u;
        return 4u;
    }

    (void)ApplyVolume(sample, runtime);
    sample.loop_count = loop_count;
    runtime.SetMilesSampleLoop(sample.sample, loop_count);
    runtime.StartMilesSample(sample.sample);
    const auto counter = runtime.PerformanceCounter();
    sample.started_at_words[0] = static_cast<std::uint32_t>(counter);
    sample.started_at_words[1] = static_cast<std::uint32_t>(counter >> 32u);
    runtime.AssignPath(sample, path);
    return 0u;
}

}  // namespace pal4
