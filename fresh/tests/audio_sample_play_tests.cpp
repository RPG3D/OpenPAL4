#include "pal4/audio_sample.h"

#include <cstdlib>
#include <iostream>
#include <map>
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

class PlayRuntime final : public pal4::IAudioSamplePlayRuntime {
public:
    void InitializePath(pal4::AudioSampleState&) override {}
    std::uint32_t CleanupPath(pal4::AudioSampleState&) override { return 0u; }
    void EndMilesSample(std::uint32_t sample) override {
        events.push_back("end:" + std::to_string(sample));
    }
    std::uint32_t RecycleMilesSample(std::uint32_t sample) override {
        events.push_back("recycle:" + std::to_string(sample));
        return 0x44u;
    }
    std::uint32_t ResumeMilesSample(std::uint32_t sample) override {
        events.push_back("resume:" + std::to_string(sample));
        return 0x22u;
    }
    std::uint32_t MilesSampleStatus(std::uint32_t sample) override {
        events.push_back("status:" + std::to_string(sample));
        return 4u;
    }
    std::uint32_t FileCrc(std::uint32_t file) override {
        events.push_back("crc:" + std::to_string(file));
        return file == 0x700u ? 0x1234u : 0x5678u;
    }
    void ReleasePoolFile(std::uint32_t crc) override {
        events.push_back("release:" + std::to_string(crc));
    }
    void InitializeMilesSample(std::uint32_t sample) override {
        events.push_back("sample-init:" + std::to_string(sample));
    }
    std::string FilePath(std::uint32_t file) override {
        events.push_back("file-path:" + std::to_string(file));
        return paths[file];
    }
    std::uint32_t AddPoolFile(std::string_view path) override {
        events.push_back("pool-add:" + std::string(path));
        if (new_file != 0u)
            paths[new_file] = path;
        return new_file;
    }
    std::uint32_t FileData(std::uint32_t file) override {
        events.push_back("file-data:" + std::to_string(file));
        return file + 0x100u;
    }
    bool SetMilesSampleFile(std::uint32_t sample,
                            std::uint32_t data) override {
        events.push_back("sample-file:" + std::to_string(sample) + ":" +
                         std::to_string(data));
        return bind_success;
    }
    std::uint32_t StopMilesSample(std::uint32_t sample) override {
        events.push_back("sample-stop:" + std::to_string(sample));
        return 0x11u;
    }
    float SampleMasterVolume() override {
        events.emplace_back("master-volume");
        return 0.5f;
    }
    std::uint32_t SetMilesSampleVolume(std::uint32_t sample,
                                       std::uint32_t left,
                                       std::uint32_t right) override {
        events.push_back("sample-volume:" + std::to_string(sample));
        Check(left == 0x3f000000u && right == left,
              "sample receives equal half-volume float bits");
        return 0x33u;
    }
    void SetMilesSampleLoop(std::uint32_t sample,
                            std::uint32_t loop_count) override {
        events.push_back("sample-loop:" + std::to_string(sample) + ":" +
                         std::to_string(loop_count));
    }
    void StartMilesSample(std::uint32_t sample) override {
        events.push_back("sample-start:" + std::to_string(sample));
    }
    std::uint64_t PerformanceCounter() override {
        events.emplace_back("counter");
        return 0x1122334455667788ull;
    }
    void AssignPath(pal4::AudioSampleState& sample,
                    std::string_view path) override {
        events.push_back("path-assign:" + std::string(path));
        sample.path.length = static_cast<std::uint32_t>(path.size());
    }

    std::uint32_t new_file = 0x701u;
    bool bind_success = true;
    std::map<std::uint32_t, std::string> paths{
        {0x700u, "song.wav"}};
    std::vector<std::string> events;
};

void DeviceAndSampleGates() {
    PlayRuntime runtime;
    pal4::AudioSampleState sample{};
    sample.sample = 0x500u;
    int device = 1;
    Check(pal4::AudioSample::Play(
              sample, 0x600u, "song.wav", nullptr, 2u, runtime) == 3u &&
              sample.sample == 0x500u && runtime.events.empty(),
          "null device returns three before stopping old sample");
    Check(pal4::AudioSample::Play(
              sample, 0u, "song.wav", &device, 2u, runtime) == 2u &&
              sample.sample == 0u &&
              runtime.events == std::vector<std::string>{
                  "end:1280", "recycle:1280"},
          "null Miles sample stops old sample then returns two");
}

void CachedAndChangedPaths() {
    int device = 1;
    PlayRuntime runtime;
    pal4::AudioSampleState sample{};
    sample.audio_file = 0x700u;
    sample.volume_bits = 0x3f800000u;
    Check(pal4::AudioSample::Play(
              sample, 0x600u, "song.wav", &device, 3u,
              runtime) == 0u &&
              sample.audio_file == 0x700u && sample.sample == 0x600u &&
              sample.loop_count == 3u && sample.path.length == 8u &&
              sample.started_at_words[0] == 0x55667788u &&
              sample.started_at_words[1] == 0x11223344u &&
              runtime.events == std::vector<std::string>{
                  "sample-init:1536", "file-path:1792", "file-data:1792",
                  "sample-file:1536:2048", "master-volume",
                  "sample-volume:1536", "sample-loop:1536:3",
                  "sample-start:1536", "counter",
                  "path-assign:song.wav"},
          "same path retains cached file and initializes sample playback");

    runtime.events.clear();
    Check(pal4::AudioSample::Play(
              sample, 0x601u, "other.wav", &device, 1u,
              runtime) == 0u &&
              sample.audio_file == 0x701u &&
              runtime.events == std::vector<std::string>{
                  "end:1536", "recycle:1536", "sample-init:1537",
                  "file-path:1792",
                  "crc:1792", "release:4660", "pool-add:other.wav",
                  "file-data:1793", "sample-file:1537:2049",
                  "master-volume", "sample-volume:1537",
                  "sample-loop:1537:1", "sample-start:1537", "counter",
                  "path-assign:other.wav"},
          "changed path releases old pool reference before loading another");
}

void LoadAndBindFailures() {
    int device = 1;
    PlayRuntime missing;
    missing.new_file = 0u;
    pal4::AudioSampleState sample{};
    Check(pal4::AudioSample::Play(
              sample, 0x600u, "missing.wav", &device, 1u,
              missing) == 1u && sample.sample == 0u &&
              sample.audio_file == 0u &&
              missing.events == std::vector<std::string>{
                  "sample-init:1536", "pool-add:missing.wav",
                  "end:1536", "recycle:1536"},
          "pool-load failure invokes sample stop and returns one");

    PlayRuntime binding;
    binding.bind_success = false;
    Check(pal4::AudioSample::Play(
              sample, 0x600u, "bad.wav", &device, 1u,
              binding) == 4u && sample.sample == 0u &&
              sample.audio_file == 0u &&
              binding.events == std::vector<std::string>{
                  "sample-init:1536", "pool-add:bad.wav",
                  "file-data:1793", "sample-file:1536:2049",
                  "sample-stop:1536", "crc:1793", "release:22136"},
          "Miles file-bind failure stops handle and releases new pool file");
}

}  // namespace

int main() {
    DeviceAndSampleGates();
    CachedAndChangedPaths();
    LoadAndBindFailures();
    std::cout << "Audio sample play tests passed\n";
}
