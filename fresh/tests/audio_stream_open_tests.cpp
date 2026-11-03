#include "pal4/audio_stream.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class OpenRuntime final : public pal4::IAudioStreamOpenRuntime {
public:
    void InitializePath(pal4::AudioStreamState&) override {}
    std::uint32_t CleanupPath(pal4::AudioStreamState&) override { return 0u; }
    void CloseMilesStream(std::uint32_t stream) override {
        events.emplace_back("miles-close:" + std::to_string(stream));
    }
    std::uint32_t InvokeCallback(std::uint32_t callback,
                                 std::uint32_t parent) override {
        events.emplace_back("callback:" + std::to_string(callback) + ":" +
                            std::to_string(parent));
        return 1u;
    }
    bool PackageMode() override {
        events.emplace_back("mode");
        return package_mode;
    }
    bool PackageAlreadyOpen() override {
        events.emplace_back("package-active");
        return package_active;
    }
    void ClosePackage() override { events.emplace_back("package-close"); }
    void SetPackagePointer(std::uint32_t value) override {
        events.emplace_back("package-pointer:" + std::to_string(value));
    }
    bool OpenPackage(std::string_view path) override {
        events.emplace_back("package-open:" + std::string(path));
        return package_opens;
    }
    std::string ResolveMusicPath(std::string_view path,
                                 std::uint32_t max_bytes) override {
        events.emplace_back("resolve:" + std::string(path) + ":" +
                            std::to_string(max_bytes));
        return "music/theme.mp3";
    }
    std::uint32_t OpenPackageFile(std::string_view path) override {
        events.emplace_back("file-open:" + std::string(path));
        return package_file;
    }
    std::uint32_t AllocatePackedBuffer(std::uint32_t bytes) override {
        events.emplace_back("buffer-allocate:" + std::to_string(bytes));
        return allocation;
    }
    std::uint32_t InitializePackedBuffer(std::uint32_t address) override {
        events.emplace_back("buffer-init:" + std::to_string(address));
        return address;
    }
    pal4::AudioPackedFile LoadPackageFile(
        std::uint32_t file, std::uint32_t buffer) override {
        events.emplace_back("file-load:" + std::to_string(file) + ":" +
                            std::to_string(buffer));
        return {0x8800u, 1024u};
    }
    void ClosePackageFile(std::uint32_t file) override {
        events.emplace_back("file-close:" + std::to_string(file));
    }
    void CleanupPackedBuffer(std::uint32_t buffer) override {
        events.emplace_back("buffer-clean:" + std::to_string(buffer));
    }
    void FreePackedBuffer(std::uint32_t buffer) override {
        events.emplace_back("buffer-free:" + std::to_string(buffer));
    }
    std::string TempDirectory(std::uint32_t max_bytes) override {
        events.emplace_back("temp-directory:" + std::to_string(max_bytes));
        return "C:\\Temp\\";
    }
    std::uint32_t TimeMilliseconds() override {
        events.emplace_back("time");
        return 100u + time_calls++;
    }
    std::uint32_t Crc32(std::string_view text) override {
        events.emplace_back("crc:" + std::string(text));
        return 77u;
    }
    std::uint32_t CreateTempFile(std::string_view path) override {
        events.emplace_back("temp-create:" + std::string(path));
        const auto index = temp_creates++;
        return index < temp_handles.size() ? temp_handles[index] : 0u;
    }
    std::uint32_t CloseTempFile(std::uint32_t file) override {
        events.emplace_back("temp-close:" + std::to_string(file));
        return 1u;
    }
    std::uint32_t WriteTempFile(std::uint32_t file, std::uint32_t data,
                                std::uint32_t bytes) override {
        events.emplace_back("temp-write:" + std::to_string(file) + ":" +
                            std::to_string(data) + ":" +
                            std::to_string(bytes));
        return written_bytes;
    }
    void RewindTempFile(std::uint32_t file) override {
        events.emplace_back("temp-rewind:" + std::to_string(file));
    }
    void RemovePath(std::string_view path) override {
        events.emplace_back("temp-remove:" + std::string(path));
    }
    std::uint32_t OpenMilesStream(std::uint32_t driver,
                                  std::string_view path) override {
        events.emplace_back("miles-open:" + std::to_string(driver) + ":" +
                            std::string(path));
        return miles_stream;
    }
    void StartMilesStream(std::uint32_t stream) override {
        events.emplace_back("miles-start:" + std::to_string(stream));
    }
    float MasterVolume() override {
        events.emplace_back("master-volume");
        return 0.5f;
    }
    void SetMilesVolume(std::uint32_t stream, std::uint32_t left,
                        std::uint32_t right) override {
        events.emplace_back("miles-volume:" + std::to_string(stream));
        Check(left == 0x3f000000u && right == left,
              "Miles receives matching half-volume float bits");
    }
    void SetMilesLoop(std::uint32_t stream,
                      std::uint32_t count) override {
        events.emplace_back("miles-loop:" + std::to_string(stream) + ":" +
                            std::to_string(count));
    }
    std::uint64_t PerformanceCounter() override {
        events.emplace_back("counter");
        return 0x1122334455667788ull;
    }
    void AssignPath(pal4::AudioStreamState& stream,
                    std::string_view path) override {
        events.emplace_back("path-assign:" + std::string(path));
        stream.path.length = static_cast<std::uint32_t>(path.size());
    }

    bool package_mode = false;
    bool package_active = false;
    bool package_opens = true;
    std::uint32_t package_file = 0x400u;
    std::uint32_t allocation = 0x500u;
    std::uint32_t miles_stream = 0x600u;
    std::uint32_t written_bytes = 1024u;
    std::vector<std::uint32_t> temp_handles{0x700u};
    std::size_t temp_creates = 0u;
    std::uint32_t time_calls = 0u;
    std::vector<std::string> events;
};

bool Has(const OpenRuntime& runtime, std::string_view event) {
    for (const auto& observed : runtime.events)
        if (observed == event)
            return true;
    return false;
}

void DirectBranches() {
    OpenRuntime runtime;
    pal4::AudioStreamState stream{};
    pal4::AudioOpenDevice device{};
    device.miles_driver = 0x123u;
    Check(pal4::AudioStream::Open(
              stream, "song.mp3", nullptr, 2u, runtime) == 3u &&
              runtime.events.empty(),
          "null audio device returns three before any side effect");
    stream.stream = 0x999u;
    stream.active = 1u;
    stream.volume_bits = 0x3f800000u;
    Check(pal4::AudioStream::Open(
              stream, "song.mp3", &device, 2u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "miles-close:2457", "mode", "miles-open:291:song.mp3",
                  "miles-start:1536", "master-volume", "miles-volume:1536",
                  "miles-loop:1536:2", "counter", "path-assign:song.mp3"} &&
              stream.stream == 0x600u && stream.active == 1u &&
              stream.loop_count == 2u &&
              stream.started_at_words[0] == 0x55667788u &&
              stream.started_at_words[1] == 0x11223344u &&
              stream.path.length == 8u,
          "direct open cleans old stream and initializes playback state");

    runtime.events.clear();
    runtime.miles_stream = 0u;
    stream.stream = 0u;
    stream.active = 0u;
    Check(pal4::AudioStream::Open(
              stream, "missing.mp3", &device, 5u, runtime) == 2u &&
              runtime.events == std::vector<std::string>{
                  "mode", "miles-open:291:missing.mp3"} &&
              stream.active == 0u,
          "failed direct Miles open returns two without starting playback");
}

void PackageBranches() {
    pal4::AudioOpenDevice device{};
    device.miles_driver = 0x123u;
    pal4::AudioStreamState stream{};
    stream.volume_bits = 0x3f800000u;
    OpenRuntime runtime;
    runtime.package_mode = true;
    runtime.package_active = true;
    stream.temp_file = 0xabcdu;
    Check(pal4::AudioStream::Open(
              stream, "song.mp3", &device, 3u, runtime) == 0u &&
              stream.temp_file == 0x700u && stream.stream == 0x600u &&
              Has(runtime, "package-open:song.smp") &&
              Has(runtime, "resolve:song.mp3:256") &&
              Has(runtime, "file-open:music/theme.mp3") &&
              Has(runtime, "temp-close:43981") &&
              Has(runtime, "crc:music/theme") &&
              Has(runtime, "temp-create:C:\\Temp\\~77_100.tmp") &&
              Has(runtime, "temp-write:1792:34816:1024") &&
              Has(runtime, "temp-remove:C:\\Temp\\~77_100.tmp") &&
              Has(runtime, "miles-open:291:\\\\\\\\1792,1024.mp3") &&
              Has(runtime, "buffer-clean:1280") &&
              Has(runtime, "buffer-free:1280") &&
              runtime.events.back() == "path-assign:song.mp3",
          "package mode stages music in a temp handle and opens Miles stream");
    Check(runtime.events[2] == "package-close" &&
              runtime.events[3] == "package-pointer:1" &&
              runtime.events[4] == "package-open:song.smp",
          "existing package closes before global pointer and new open");

    OpenRuntime retry;
    retry.package_mode = true;
    retry.temp_handles = {0u, 0x701u};
    pal4::AudioStreamState retry_stream{};
    retry_stream.volume_bits = 0x3f800000u;
    Check(pal4::AudioStream::Open(
              retry_stream, "song.mp3", &device, 1u, retry) == 0u &&
              retry_stream.temp_file == 0x701u &&
              Has(retry, "temp-create:C:\\Temp\\~77_100.tmp") &&
              Has(retry, "temp-create:~77_101.tmp") &&
              Has(retry, "temp-remove:~77_101.tmp") &&
              retry.time_calls == 2u,
          "temp-file failure retries without directory and recomputes time");

    OpenRuntime package_failure;
    package_failure.package_mode = true;
    package_failure.package_opens = false;
    Check(pal4::AudioStream::Open(
              stream, "song.mp3", &device, 1u, package_failure) == 2u &&
              package_failure.events.back() == "package-open:song.smp" &&
              !Has(package_failure, "package-close"),
          "package-open failure does not enter package cleanup branch");

    OpenRuntime no_extension;
    no_extension.package_mode = true;
    no_extension.package_opens = false;
    pal4::AudioStreamState no_extension_stream{};
    Check(pal4::AudioStream::Open(
              no_extension_stream, "song", &device, 1u,
              no_extension) == 2u &&
              no_extension.events.back() == "package-open:.smp",
          "native last-dot helper truncates extensionless package name");

    OpenRuntime missing_file;
    missing_file.package_mode = true;
    missing_file.package_file = 0u;
    pal4::AudioStreamState missing_stream{};
    Check(pal4::AudioStream::Open(
              missing_stream, "song.mp3", &device, 1u, missing_file) == 2u &&
              missing_file.events.back() == "package-close" &&
              !Has(missing_file, "buffer-allocate:20"),
          "missing package entry closes package without allocating buffer");
}

void ResourceFailures() {
    pal4::AudioOpenDevice device{};
    device.miles_driver = 0x123u;
    OpenRuntime runtime;
    runtime.package_mode = true;
    runtime.temp_handles = {0u, 0u};
    pal4::AudioStreamState stream{};
    Check(pal4::AudioStream::Open(
              stream, "song.mp3", &device, 1u, runtime) == 2u &&
              !Has(runtime, "temp-write:0:34816:1024") &&
              Has(runtime, "buffer-clean:1280") &&
              Has(runtime, "buffer-free:1280") &&
              runtime.events.back() == "package-close",
          "both temp-file failures still release packed buffer and package");

    OpenRuntime allocation_failure;
    allocation_failure.package_mode = true;
    allocation_failure.allocation = 0u;
    Check(pal4::AudioStream::Open(
              stream, "song.mp3", &device, 1u,
              allocation_failure) == 2u &&
              Has(allocation_failure, "buffer-allocate:20") &&
              Has(allocation_failure, "file-close:1024") &&
              allocation_failure.events.back() == "package-close",
          "allocation failure exits without dereferencing a null buffer");
}

}  // namespace

int main() {
    DirectBranches();
    PackageBranches();
    ResourceFailures();
    std::cout << "Audio stream open tests passed\n";
}
