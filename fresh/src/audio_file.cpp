#include "pal4/audio_file.h"

#include <cstring>

namespace pal4 {

AudioFileState& AudioFile::Initialize(
    AudioFileState& file, IAudioFileRuntime& runtime) {
    file.data = 0u;
    file.reference_count = 0u;
    runtime.InitializePath(file.path);
    return file;
}

std::uint32_t AudioFile::Cleanup(
    AudioFileState& file, IAudioFileRuntime& runtime) {
    if (file.data != 0u)
        runtime.FreeLocked(file.data);
    return runtime.CleanupPath(file.path);
}

std::uint8_t AudioFile::Load(
    AudioFileState& file, const char* path, IAudioFileRuntime& runtime) {
    const auto bytes = runtime.FileSize(path);
    const auto data = runtime.FileRead(path, 0u);
    if (data == 0u) {
        runtime.Log(97u);
        return 0u;
    }

    switch (runtime.FileType(data, bytes)) {
        case 1u:
            file.data = data;
            file.bytes = bytes;
            break;
        case 2u: {
            AudioWaveInfo info{};
            runtime.WaveInfo(data, info);
            if (!runtime.DecompressAdpcm(info, file.data, file.bytes)) {
                runtime.Log(99u);
                return 0u;
            }
            runtime.FreeLocked(data);
            break;
        }
        case 13u:
            if (!runtime.DecompressAsi(data, bytes, ".mp3", file.data,
                                       file.bytes, 0u)) {
                runtime.Log(98u);
                return 0u;
            }
            runtime.FreeLocked(data);
            break;
        default:
            runtime.FreeLocked(data);
            return 0u;
    }

    const auto length = static_cast<std::uint32_t>(std::strlen(path));
    file.path_crc = runtime.ComputeCrc(path, length);
    runtime.AssignPath(file.path, path, length);
    return 1u;
}

std::uint32_t AudioFile::DestroyAt(
    AudioFileState& file, const std::uint32_t address,
    const std::uint8_t flags, IAudioFileDestroyRuntime& runtime) {
    (void)Cleanup(file, runtime);
    if ((flags & 1u) != 0u)
        (void)runtime.FreeFile(address);
    return address;
}

}  // namespace pal4
