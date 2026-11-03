#include "pal4/audio_playback_handle.h"

#include <limits>
#include <stdexcept>

namespace pal4 {

void AudioPlaybackHandle::ResetThunk(
    IAudioPlaybackHandleRuntime& runtime) {
    Reset(runtime);
}

void AudioPlaybackHandle::Reset(
    IAudioPlaybackHandleRuntime& runtime) {
    runtime.WriteWord(kAddress, 0u);
}

std::uint8_t AudioPlaybackHandle::PlayMusic(
    const std::string_view file, const std::uint8_t flag,
    IPlaybackMusicRuntime& runtime) {
    const auto playing = runtime.IsPlaying(kAddress);
    if (playing != 0u && flag != 0u) return playing;

    AudioPathState path{};
    runtime.InitializePath(path, flag, "gamedata\\palSound\\");
    const auto file_name = file.substr(0u, file.find('\0'));
    if (file_name.size() >=
        static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max() - path.length))
        throw std::length_error("audio path too long");
    if (!file_name.empty()) {
        const auto new_length = path.length +
            static_cast<std::uint32_t>(file_name.size());
        if (runtime.AllocateText(path, new_length, 0u)) {
            runtime.CopyText(path.data + path.length, file_name);
            path.length = new_length;
            runtime.WriteByte(path.data + path.length, 0u);
        }
    }

    const auto name = path.data != 0u ? path.data : runtime.EmptyString();
    runtime.WriteWord(kAddress, runtime.Play(flag, name, 1u));
    if (path.data == 0u) return 0u;
    const auto header = path.data - 1u;
    const auto reference = runtime.ReadByte(header);
    if (reference == 0u || reference == 0xffu)
        return runtime.Free(header);
    runtime.WriteByte(header, static_cast<std::uint8_t>(reference - 1u));
    return static_cast<std::uint8_t>(header);
}

}  // namespace pal4
