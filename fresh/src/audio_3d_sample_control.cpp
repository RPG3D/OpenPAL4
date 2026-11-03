#include "pal4/audio_3d_sample.h"

#include <bit>

namespace pal4 {

std::uint32_t Audio3DSample::Stop(
    Audio3DSampleState& sample, IAudio3DSampleControlRuntime& runtime) {
    if (sample.sample == 0u)
        return 0u;
    runtime.EndMiles3DSample(sample.sample);
    const auto result = runtime.RecycleMiles3DSample(sample.sample);
    sample.sample = 0u;
    return result;
}

std::uint32_t Audio3DSample::StopImmediate(
    const Audio3DSampleState& sample,
    IAudio3DSampleControlRuntime& runtime) {
    return runtime.StopMiles3DSample(sample.sample);
}

std::uint32_t Audio3DSample::Resume(
    const Audio3DSampleState& sample,
    IAudio3DSampleControlRuntime& runtime) {
    return sample.sample == 0u ? 0u :
           runtime.ResumeMiles3DSample(sample.sample);
}

bool Audio3DSample::IsPlaying(
    const Audio3DSampleState& sample,
    IAudio3DSampleControlRuntime& runtime) {
    return runtime.Miles3DSampleStatus(sample.sample) == 4u;
}

std::uint32_t Audio3DSample::SetPosition(
    Audio3DSampleState& sample,
    const std::array<std::uint32_t, 3u>& vector,
    IAudio3DSampleControlRuntime& runtime) {
    sample.vector24 = vector;
    return runtime.SetMiles3DPosition(
        sample.sample, vector[0], vector[1],
        std::bit_cast<std::uint32_t>(-
            std::bit_cast<float>(vector[2])));
}

std::uint32_t Audio3DSample::ApplyVolume(
    const Audio3DSampleState& sample,
    IAudio3DSampleControlRuntime& runtime) {
    if (sample.sample == 0u)
        return 0u;
    const auto volume = runtime.SampleMasterVolume() *
                        std::bit_cast<float>(sample.volume_bits);
    return runtime.SetMiles3DSampleVolume(
        sample.sample, std::bit_cast<std::uint32_t>(volume));
}

}  // namespace pal4
