#include "pal4/audio_stream.h"

#include <bit>
#include <string>

namespace pal4 {
namespace {

std::string WithoutExtension(const std::string_view path) {
    const auto dot = path.find_last_of('.');
    return std::string(path.substr(0u, dot == std::string_view::npos
                                    ? 0u : dot));
}

std::string TempFileName(const std::string_view prefix,
                         const std::uint32_t crc,
                         const std::uint32_t time) {
    return std::string(prefix) + "~" + std::to_string(crc) + "_" +
           std::to_string(time) + ".tmp";
}

std::string MilesHandlePath(const std::uint32_t handle,
                            const std::uint32_t bytes) {
    return "\\\\\\\\" + std::to_string(handle) + "," +
           std::to_string(bytes) + ".mp3";
}

}  // namespace

std::uint32_t AudioStream::Open(
    AudioStreamState& stream, const char* path,
    const AudioOpenDevice* device, const std::uint32_t loop_count,
    IAudioStreamOpenRuntime& runtime) {
    if (device == nullptr)
        return 3u;
    if (stream.stream != 0u)
        (void)Release(stream, runtime);

    if (runtime.PackageMode()) {
        const auto package_name = WithoutExtension(path) + ".smp";
        if (runtime.PackageAlreadyOpen())
            runtime.ClosePackage();
        runtime.SetPackagePointer(1u);
        if (runtime.OpenPackage(package_name)) {
            const auto music_path = runtime.ResolveMusicPath(path, 256u);
            const auto package_file = runtime.OpenPackageFile(music_path);
            if (package_file != 0u) {
                const auto memory = runtime.AllocatePackedBuffer(20u);
                const auto buffer = memory == 0u ? 0u
                    : runtime.InitializePackedBuffer(memory);
                AudioPackedFile packed{};
                if (buffer != 0u)
                    packed = runtime.LoadPackageFile(package_file, buffer);
                runtime.ClosePackageFile(package_file);

                if (stream.temp_file != 0u)
                    (void)runtime.CloseTempFile(stream.temp_file);
                stream.temp_file = 0u;
                if (buffer != 0u) {
                    const auto basename = WithoutExtension(music_path);
                    const auto directory = runtime.TempDirectory(256u);
                    const auto first_time = runtime.TimeMilliseconds();
                    const auto first_crc = runtime.Crc32(basename);
                    auto filename = TempFileName(
                        directory, first_crc, first_time);
                    stream.temp_file = runtime.CreateTempFile(filename);
                    if (stream.temp_file == 0u) {
                        const auto retry_time = runtime.TimeMilliseconds();
                        const auto retry_crc = runtime.Crc32(basename);
                        filename = TempFileName("", retry_crc, retry_time);
                        stream.temp_file = runtime.CreateTempFile(filename);
                    }
                    if (stream.temp_file != 0u) {
                        const auto written = runtime.WriteTempFile(
                            stream.temp_file, packed.data, packed.bytes);
                        runtime.RewindTempFile(stream.temp_file);
                        runtime.RemovePath(filename);
                        stream.stream = runtime.OpenMilesStream(
                            device->miles_driver,
                            MilesHandlePath(stream.temp_file, written));
                    }
                    runtime.CleanupPackedBuffer(buffer);
                    runtime.FreePackedBuffer(buffer);
                }
            }
            runtime.ClosePackage();
        }
    } else {
        stream.stream = runtime.OpenMilesStream(
            device->miles_driver, path);
    }
    if (stream.stream == 0u)
        return 2u;

    runtime.StartMilesStream(stream.stream);
    const auto volume = runtime.MasterVolume() *
        std::bit_cast<float>(stream.volume_bits);
    const auto volume_bits = std::bit_cast<std::uint32_t>(volume);
    runtime.SetMilesVolume(stream.stream, volume_bits, volume_bits);
    stream.loop_count = loop_count;
    runtime.SetMilesLoop(stream.stream, loop_count);
    stream.active = 1u;
    const auto counter = runtime.PerformanceCounter();
    stream.started_at_words[0] = static_cast<std::uint32_t>(counter);
    stream.started_at_words[1] = static_cast<std::uint32_t>(counter >> 32u);
    runtime.AssignPath(stream, path);
    return 0u;
}

}  // namespace pal4
