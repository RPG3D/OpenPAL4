#include "pal4/audio_stream.h"

namespace pal4 {

AudioStreamState& AudioStream::Initialize(
    AudioStreamState& stream, const std::uint32_t parent,
    const std::uint32_t parameter, IAudioStreamRuntime& runtime) {
    stream.parent = parent;
    stream.vtable = kBaseVtable;
    stream.active = 0u;
    stream.stream = 0u;
    stream.loop_count = 1u;
    stream.volume_bits = 0x3f800000u;
    stream.fade_rate_bits = 0u;
    runtime.InitializePath(stream);
    stream.remaining_threshold_bits = 0u;
    stream.completion_callback = parameter;
    stream.temp_file = 0u;
    stream.vtable = kStreamVtable;
    return stream;
}

std::uint32_t AudioStream::Cleanup(
    AudioStreamState& stream, IAudioStreamControlRuntime& runtime) {
    stream.vtable = kStreamVtable;
    (void)Release(stream, runtime);
    const auto result = runtime.CleanupPath(stream);
    stream.vtable = kBaseVtable;
    return result;
}

std::uint32_t AudioStream::Release(
    AudioStreamState& stream, IAudioStreamControlRuntime& runtime) {
    const auto miles_stream = stream.stream;
    if (miles_stream == 0u)
        return 0u;
    runtime.CloseMilesStream(miles_stream);
    stream.stream = 0u;
    stream.active = 0u;
    if (stream.completion_callback != 0u) {
        (void)runtime.InvokeCallback(
            stream.completion_callback, stream.parent);
        stream.completion_callback = 0u;
    }
    const auto temp_file = stream.temp_file;
    if (temp_file == 0u)
        return 0u;
    const auto result = runtime.CloseTempFile(temp_file);
    stream.temp_file = 0u;
    return result;
}

std::uint32_t AudioStream::Pause(
    const AudioStreamState& stream, IAudioStreamPauseRuntime& runtime) {
    if (stream.stream == 0u)
        return 0u;
    return runtime.PauseMilesStream(stream.stream, true);
}

std::uint32_t AudioStream::Resume(
    const AudioStreamState& stream, IAudioStreamPauseRuntime& runtime) {
    if (stream.stream == 0u)
        return 0u;
    return runtime.PauseMilesStream(stream.stream, false);
}

bool AudioStream::IsActive(const AudioStreamState& stream) {
    return stream.active != 0u;
}

}  // namespace pal4
