#include "pal4/audio_stream.h"

#include <bit>
#include <cstdint>

namespace pal4 {

std::uint32_t AudioStream::Update(
    AudioStreamState& stream, const float seconds,
    IAudioStreamUpdateRuntime& runtime) {
    const auto handle = stream.stream;
    if (handle == 0u)
        return 0u;

    switch (stream.active) {
        case 1u: {
            if (runtime.StreamStatus(handle) == 8u)
                return Release(stream, runtime);
            std::uint32_t total_ms = 0u;
            std::uint32_t current_ms = 0u;
            runtime.StreamMilliseconds(handle, total_ms, current_ms);
            const auto remaining_ms = std::bit_cast<std::int32_t>(
                total_ms - current_ms);
            const auto remaining_seconds =
                static_cast<double>(remaining_ms) *
                static_cast<double>(0.001f);
            if (remaining_seconds <= static_cast<double>(
                    std::bit_cast<float>(stream.remaining_threshold_bits)) &&
                stream.completion_callback != 0u)
                return runtime.InvokeCallback(
                    stream.completion_callback, stream.parent);
            return 0u;
        }
        case 2u: {
            const auto target = runtime.MasterVolume() *
                std::bit_cast<float>(stream.volume_bits);
            const auto level = seconds *
                std::bit_cast<float>(stream.fade_rate_bits) +
                std::bit_cast<float>(stream.fade_level_bits);
            stream.fade_level_bits = std::bit_cast<std::uint32_t>(level);
            const auto actual = level >= target ? target : level;
            const auto bits = std::bit_cast<std::uint32_t>(actual);
            const auto result = runtime.SetStreamVolume(handle, bits, bits);
            if (level >= target)
                stream.active = 1u;
            return result;
        }
        case 3u: {
            const auto level =
                std::bit_cast<float>(stream.fade_level_bits) -
                seconds * std::bit_cast<float>(stream.fade_rate_bits);
            stream.fade_level_bits = std::bit_cast<std::uint32_t>(level);
            if (level > 0.0f) {
                const auto bits = std::bit_cast<std::uint32_t>(level);
                return runtime.SetStreamVolume(handle, bits, bits);
            }
            return Release(stream, runtime);
        }
        default:
            return stream.active - 3u;
    }
}

std::uint32_t AudioStream::FadeOut(
    AudioStreamState& stream, const float seconds,
    IAudioStreamUpdateRuntime& runtime) {
    if (stream.stream == 0u || !(seconds > 0.0001f))
        return 0u;
    std::uint32_t left_bits = 0u;
    std::uint32_t right_bits = 0u;
    runtime.ReadStreamVolume(stream.stream, left_bits, right_bits);
    stream.active = 3u;
    stream.fade_level_bits = left_bits;
    stream.fade_rate_bits = std::bit_cast<std::uint32_t>(
        std::bit_cast<float>(left_bits) / seconds);
    return left_bits;
}

std::uint32_t AudioStream::FadeIn(
    AudioStreamState& stream, const float seconds,
    IAudioStreamUpdateRuntime& runtime) {
    if (stream.stream == 0u || !(seconds > 0.0001f))
        return 0u;
    const auto rate = runtime.MasterVolume() *
        std::bit_cast<float>(stream.volume_bits) / seconds;
    stream.fade_rate_bits = std::bit_cast<std::uint32_t>(rate);
    const auto result = runtime.SetStreamVolume(
        stream.stream, 0u, 0u);
    stream.fade_level_bits = 0u;
    stream.active = 2u;
    return result;
}

std::uint32_t AudioStream::SetVolume(
    const AudioStreamState& stream, const std::uint32_t volume_bits,
    IAudioStreamUpdateRuntime& runtime) {
    if (stream.stream == 0u)
        return 0u;
    return runtime.SetStreamVolume(
        stream.stream, volume_bits, volume_bits);
}

}  // namespace pal4
