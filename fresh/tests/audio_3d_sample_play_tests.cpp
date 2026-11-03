#include "pal4/audio_3d_sample.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IAudio3DSamplePlayRuntime {
public:
    std::array<std::uint32_t, 3u> DefaultVector() override { return {}; }
    void InitializePath(pal4::Audio3DSampleState&) override {}
    std::uint32_t CleanupPath(pal4::Audio3DSampleState&) override { return 0u; }
    void Stop(pal4::Audio3DSampleState& state) override {
        events.emplace_back("stop");
        state.sample = 0u;
    }
    void EndMiles3DSample(std::uint32_t) override {
        events.emplace_back("end");
    }
    std::uint32_t RecycleMiles3DSample(std::uint32_t) override {
        events.emplace_back("recycle");
        return 0x44u;
    }
    std::uint32_t StopMiles3DSample(std::uint32_t) override { return 0u; }
    std::uint32_t ResumeMiles3DSample(std::uint32_t) override { return 0u; }
    std::uint32_t Miles3DSampleStatus(std::uint32_t) override { return 0u; }
    std::uint32_t FileCrc(std::uint32_t file) override {
        events.emplace_back("crc");
        return file + 1u;
    }
    void ReleasePoolFile(std::uint32_t crc) override {
        events.emplace_back("release");
        last_crc = crc;
    }
    std::uint32_t FreeObject(std::uint32_t) override { return 0u; }
    std::string FilePath(std::uint32_t) override {
        events.emplace_back("file-path");
        return cached_path;
    }
    std::uint32_t AddPoolFile(std::string_view path) override {
        events.emplace_back("add");
        added_path = path;
        return added_file;
    }
    std::uint32_t FileData(std::uint32_t) override {
        events.emplace_back("data");
        return 0x888u;
    }
    bool ReadWavInfo(std::uint32_t data, std::uint32_t& channels) override {
        events.emplace_back("wav");
        Check(data == 0x888u, "WAV info receives cached file data");
        channels = wav_channels;
        return wav_valid;
    }
    void LogWavFailure(std::uint32_t code) override {
        events.emplace_back("log");
        logged_failure = code;
    }
    void ReleaseMiles3DSample(std::uint32_t sample) override {
        events.emplace_back("miles-release");
        released_sample = sample;
    }
    bool SetMiles3DSampleFile(std::uint32_t sample,
                              std::uint32_t data) override {
        events.emplace_back("bind");
        Check(sample == 0x600u && data == 0x888u,
              "Miles bind uses current handle and file data");
        return bind_success;
    }
    float SampleMasterVolume() override {
        events.emplace_back("master");
        return 0.5f;
    }
    std::uint32_t SetMiles3DSampleVolume(
        std::uint32_t, std::uint32_t bits) override {
        events.emplace_back("volume");
        volume_bits = bits;
        return 0x11u;
    }
    void SetMiles3DSampleLoop(std::uint32_t, std::uint32_t count) override {
        events.emplace_back("loop");
        loop_count = count;
    }
    std::uint32_t SetMiles3DPosition(
        std::uint32_t, std::uint32_t x,
        std::uint32_t y, std::uint32_t z) override {
        events.emplace_back("position");
        position = {x, y, z};
        return 0x22u;
    }
    void SetMiles3DSampleDistances(std::uint32_t,
                                    std::uint32_t near_bits,
                                    std::uint32_t far_bits) override {
        events.emplace_back("distances");
        distances = {near_bits, far_bits};
    }
    void SetMiles3DSampleCone(std::uint32_t, std::uint32_t inner,
                              std::uint32_t outer,
                              std::uint32_t outside) override {
        events.emplace_back("cone");
        cone = {inner, outer, outside};
    }
    void SetMiles3DVelocity(std::uint32_t, std::uint32_t x,
                             std::uint32_t y, std::uint32_t z,
                             std::uint32_t scale) override {
        events.emplace_back("velocity");
        velocity = {x, y, z, scale};
    }
    void AutoUpdateMiles3DPosition(std::uint32_t,
                                    std::uint32_t enabled) override {
        events.emplace_back("auto-update");
        auto_update = enabled;
    }
    void StartMiles3DSample(std::uint32_t) override {
        events.emplace_back("start");
    }
    std::uint64_t PerformanceCounter() override {
        events.emplace_back("counter");
        return 0x1122334455667788ull;
    }
    void AssignPath(pal4::Audio3DSampleState& state,
                    std::string_view path) override {
        events.emplace_back("path-assign");
        state.path.length = static_cast<std::uint32_t>(path.size());
    }

    std::vector<std::string> events;
    std::string cached_path = "sound.wav";
    std::string added_path;
    std::uint32_t added_file = 0x701u;
    std::uint32_t wav_channels = 1u;
    bool wav_valid = true;
    bool bind_success = true;
    std::uint32_t last_crc = 0u;
    std::uint32_t logged_failure = 0u;
    std::uint32_t released_sample = 0u;
    std::uint32_t volume_bits = 0u;
    std::uint32_t loop_count = 0u;
    std::array<std::uint32_t, 3u> position{};
    std::array<std::uint32_t, 2u> distances{};
    std::array<std::uint32_t, 3u> cone{};
    std::array<std::uint32_t, 4u> velocity{};
    std::uint32_t auto_update = 1u;
};

void GatesAndPoolFailure() {
    Runtime runtime;
    pal4::Audio3DSampleState state{};
    state.sample = 0x500u;
    int device = 1;
    Check(pal4::Audio3DSample::PlayWithDistances(
              state, 0x600u, "sound.wav", nullptr, 2u,
              0x41u, 0x42u, runtime) == 3u &&
              state.sample == 0x500u && state.range60_bits == 0x41u &&
              state.range64_bits == 0x42u && runtime.events.empty(),
          "null device updates distances but leaves playback alone");
    Check(pal4::Audio3DSample::Play(
              state, 0u, "sound.wav", &device, 2u, runtime) == 2u &&
              state.sample == 0u && runtime.events ==
                  std::vector<std::string>{"end", "recycle"},
          "zero Miles handle stops old sample and returns two");
    runtime.events.clear();
    runtime.added_file = 0u;
    Check(pal4::Audio3DSample::Play(
              state, 0x600u, "sound.wav", &device, 2u, runtime) == 1u &&
              state.sample == 0u && runtime.events ==
                  std::vector<std::string>{"add", "end", "recycle"},
          "pool miss recycles new handle");
}

void WavAndBindingFailures() {
    int device = 1;
    for (std::uint32_t failure = 1u; failure <= 3u; ++failure) {
        Runtime runtime;
        pal4::Audio3DSampleState state{};
        state.audio_file = 0x700u;
        runtime.wav_valid = failure != 1u;
        runtime.wav_channels = failure == 2u ? 2u : 1u;
        runtime.bind_success = failure != 3u;
        const auto result = pal4::Audio3DSample::Play(
            state, 0x600u, "sound.wav", &device, 2u, runtime);
        const std::vector<std::string> prefix{
            "file-path", "data", "wav"};
        Check(result == (failure == 3u ? 4u : 1u) &&
                  state.sample == 0u && state.audio_file == 0u &&
                  runtime.logged_failure == failure &&
                  runtime.released_sample == 0x600u &&
                  runtime.last_crc == 0x701u &&
                  std::equal(prefix.begin(), prefix.end(),
                             runtime.events.begin()) &&
                  runtime.events[runtime.events.size() - 3u] ==
                      "miles-release" &&
                  runtime.events[runtime.events.size() - 2u] == "crc" &&
                  runtime.events.back() == "release",
              "WAV and binding failures release Miles and pool ownership");
    }
}

void SpatialPlaybackAndCacheReplacement() {
    int device = 1;
    Runtime runtime;
    pal4::Audio3DSampleState state{};
    state.audio_file = 0x700u;
    state.volume_bits = 0x3f800000u;
    state.range60_bits = 0x111u;
    state.range64_bits = 0x222u;
    state.vector24 = {0x3f800000u, 0x40000000u, 0x40400000u};
    state.vector36 = {0x40800000u, 0x40a00000u, 0x40c00000u};
    Check(pal4::Audio3DSample::Play(
              state, 0x600u, "sound.wav", &device, 3u, runtime) == 0u &&
              state.audio_file == 0x700u && state.sample == 0x600u &&
              state.loop_count == 3u &&
              state.started_at_words[0] == 0x55667788u &&
              state.started_at_words[1] == 0x11223344u &&
              state.path.length == 9u &&
              runtime.volume_bits == 0x3f000000u &&
              runtime.loop_count == 3u &&
              runtime.position == std::array<std::uint32_t, 3u>{
                  0x3f800000u, 0x40000000u, 0xc0400000u} &&
              runtime.velocity == std::array<std::uint32_t, 4u>{
                  0x40800000u, 0x40a00000u, 0xc0c00000u,
                  0x3f800000u} &&
              runtime.distances == std::array<std::uint32_t, 2u>{
                  0x111u, 0x222u} &&
              runtime.cone == std::array<std::uint32_t, 3u>{
                  1119092736u, 1123024896u, 0u} &&
              runtime.auto_update == 0u &&
              runtime.events.back() == "path-assign",
          "3D playback uses object distances and configured position");
    runtime.events.clear();
    runtime.cached_path = "old.wav";
    state.sample = 0u;
    Check(pal4::Audio3DSample::PlayWithDistances(
              state, 0x600u, "sound.wav", &device, 1u,
              0x333u, 0x444u, runtime) == 0u &&
              state.audio_file == 0x701u &&
              runtime.last_crc == 0x701u &&
              runtime.added_path == "sound.wav" &&
              runtime.distances == std::array<std::uint32_t, 2u>{
                  0x333u, 0x444u} &&
              runtime.events[0] == "file-path" &&
              runtime.events[1] == "crc" &&
              runtime.events[2] == "release" &&
              runtime.events[3] == "add",
          "path change releases old pool file before loading new one");
}

}  // namespace

int main() {
    GatesAndPoolFailure();
    WavAndBindingFailures();
    SpatialPlaybackAndCacheReplacement();
    std::cout << "Audio 3D sample play tests passed\n";
}
