#include "pal4/audio_stream.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

class UpdateRuntime final : public pal4::IAudioStreamUpdateRuntime {
public:
    void InitializePath(pal4::AudioStreamState&) override {}
    std::uint32_t CleanupPath(pal4::AudioStreamState&) override { return 0u; }
    void CloseMilesStream(std::uint32_t handle) override {
        events.push_back("close:" + std::to_string(handle));
    }
    std::uint32_t InvokeCallback(std::uint32_t callback,
                                 std::uint32_t parent) override {
        events.push_back("callback:" + std::to_string(callback) + ":" +
                         std::to_string(parent));
        return callback_result;
    }
    std::uint32_t CloseTempFile(std::uint32_t file) override {
        events.push_back("temp-close:" + std::to_string(file));
        return close_result;
    }
    std::uint32_t StreamStatus(std::uint32_t handle) override {
        events.push_back("status:" + std::to_string(handle));
        return status;
    }
    void StreamMilliseconds(std::uint32_t handle,
                            std::uint32_t& total,
                            std::uint32_t& current) override {
        events.push_back("position:" + std::to_string(handle));
        total = total_ms;
        current = current_ms;
    }
    float MasterVolume() override {
        events.emplace_back("master-volume");
        return master_volume;
    }
    std::uint32_t SetStreamVolume(std::uint32_t handle,
                                  std::uint32_t left,
                                  std::uint32_t right) override {
        events.push_back("volume:" + std::to_string(handle));
        Check(left == right, "Miles receives equal left/right levels");
        last_volume = left;
        return volume_result;
    }
    void ReadStreamVolume(std::uint32_t handle,
                          std::uint32_t& left,
                          std::uint32_t& right) override {
        events.push_back("read-volume:" + std::to_string(handle));
        left = current_volume;
        right = current_volume;
    }

    std::uint32_t status = 0u;
    std::uint32_t total_ms = 0u;
    std::uint32_t current_ms = 0u;
    std::uint32_t callback_result = 0x77u;
    std::uint32_t close_result = 0x55u;
    std::uint32_t volume_result = 0x66u;
    std::uint32_t last_volume = 0u;
    std::uint32_t current_volume = Bits(0.75f);
    float master_volume = 0.5f;
    std::vector<std::string> events;
};

void NormalPlayback() {
    UpdateRuntime runtime;
    pal4::AudioStreamState stream{};
    Check(pal4::AudioStream::Update(stream, 0.1f, runtime) == 0u &&
              runtime.events.empty(),
          "no stream skips every update action");
    stream.stream = 0x600u;
    stream.active = 1u;
    stream.parent = 0x300u;
    stream.completion_callback = 0x500u;
    stream.remaining_threshold_bits = Bits(0.3f);
    runtime.total_ms = 10000u;
    runtime.current_ms = 9000u;
    Check(pal4::AudioStream::Update(stream, 0.1f, runtime) == 0u &&
              runtime.events == std::vector<std::string>{
                  "status:1536", "position:1536"},
          "normal playback above threshold leaves callback dormant");
    runtime.events.clear();
    runtime.current_ms = 9800u;
    Check(pal4::AudioStream::Update(stream, 0.1f, runtime) == 0x77u &&
              runtime.events == std::vector<std::string>{
                  "status:1536", "position:1536",
                  "callback:1280:768"},
          "near-end playback invokes callback and forwards its result");
    runtime.events.clear();
    runtime.status = 8u;
    stream.temp_file = 0x700u;
    Check(pal4::AudioStream::Update(stream, 0.1f, runtime) == 0x55u &&
              runtime.events == std::vector<std::string>{
                  "status:1536", "close:1536", "callback:1280:768",
                  "temp-close:1792"} &&
              stream.stream == 0u && stream.active == 0u &&
              stream.completion_callback == 0u && stream.temp_file == 0u,
          "finished Miles stream releases callback and temp file");
}

void FadeBranches() {
    UpdateRuntime runtime;
    pal4::AudioStreamState stream{};
    stream.stream = 0x600u;
    stream.volume_bits = Bits(1.0f);
    stream.fade_rate_bits = Bits(0.5f);
    stream.fade_level_bits = Bits(0.1f);
    stream.active = 2u;
    Check(pal4::AudioStream::Update(stream, 0.2f, runtime) == 0x66u &&
              stream.active == 2u &&
              stream.fade_level_bits == Bits(0.2f) &&
              runtime.last_volume == Bits(0.2f) &&
              runtime.events == std::vector<std::string>{
                  "master-volume", "volume:1536"},
          "fade-in below target stores and applies current level");
    runtime.events.clear();
    Check(pal4::AudioStream::Update(stream, 1.0f, runtime) == 0x66u &&
              stream.active == 1u &&
              stream.fade_level_bits == Bits(0.7f) &&
              runtime.last_volume == Bits(0.5f),
          "fade-in clamps output to target and enters active playback");

    runtime.events.clear();
    stream.active = 3u;
    stream.fade_level_bits = Bits(0.6f);
    Check(pal4::AudioStream::Update(stream, 0.2f, runtime) == 0x66u &&
              stream.fade_level_bits == Bits(0.5f) &&
              runtime.last_volume == Bits(0.5f) &&
              runtime.events == std::vector<std::string>{"volume:1536"},
          "positive fade-out level updates Miles volume");
    runtime.events.clear();
    Check(pal4::AudioStream::Update(stream, 2.0f, runtime) == 0u &&
              stream.stream == 0u && stream.active == 0u &&
              runtime.events == std::vector<std::string>{"close:1536"},
          "fade-out reaching zero releases stream");
}

void UnknownState() {
    UpdateRuntime runtime;
    pal4::AudioStreamState stream{};
    stream.stream = 0x600u;
    stream.active = 4u;
    Check(pal4::AudioStream::Update(stream, 0.1f, runtime) == 1u &&
              runtime.events.empty(),
          "unsupported active state returns state minus three");
    stream.active = 0u;
    Check(pal4::AudioStream::Update(stream, 0.1f, runtime) ==
              0xfffffffdu,
          "zero active state keeps native unsigned subtraction");
}

void FadeSetupAndSetVolume() {
    UpdateRuntime runtime;
    pal4::AudioStreamState stream{};
    stream.volume_bits = Bits(1.0f);
    Check(pal4::AudioStream::FadeOut(stream, 1.0f, runtime) == 0u &&
              pal4::AudioStream::FadeIn(stream, 1.0f, runtime) == 0u &&
              pal4::AudioStream::SetVolume(
                  stream, Bits(0.25f), runtime) == 0u &&
              runtime.events.empty(),
          "fade and volume setters skip missing stream");
    stream.stream = 0x600u;
    Check(pal4::AudioStream::FadeOut(stream, 0.0001f, runtime) == 0u &&
              pal4::AudioStream::FadeIn(stream, 0.0001f, runtime) == 0u &&
              runtime.events.empty(),
          "fade durations at native threshold do not change stream");
    Check(pal4::AudioStream::FadeOut(stream, 1.5f, runtime) ==
                  Bits(0.75f) &&
              stream.active == 3u &&
              stream.fade_level_bits == Bits(0.75f) &&
              stream.fade_rate_bits == Bits(0.5f) &&
              runtime.events ==
                  std::vector<std::string>{"read-volume:1536"},
          "fade-out captures current Miles level and rate");
    runtime.events.clear();
    Check(pal4::AudioStream::FadeIn(stream, 2.0f, runtime) == 0x66u &&
              stream.active == 2u &&
              stream.fade_rate_bits == Bits(0.25f) &&
              stream.fade_level_bits == 0u &&
              runtime.events == std::vector<std::string>{
                  "master-volume", "volume:1536"} &&
              runtime.last_volume == 0u,
          "fade-in computes target rate and starts at zero volume");
    runtime.events.clear();
    Check(pal4::AudioStream::SetVolume(
              stream, Bits(0.25f), runtime) == 0x66u &&
              runtime.last_volume == Bits(0.25f) &&
              runtime.events ==
                  std::vector<std::string>{"volume:1536"},
          "volume setter forwards exact float bits to both channels");
}

}  // namespace

int main() {
    NormalPlayback();
    FadeBranches();
    UnknownState();
    FadeSetupAndSetVolume();
    std::cout << "Audio stream update tests passed\n";
}
