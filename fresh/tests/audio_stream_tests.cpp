#include "pal4/audio_stream.h"

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

class StreamRuntime final : public pal4::IAudioStreamControlRuntime,
                            public pal4::IAudioStreamPauseRuntime {
public:
    void InitializePath(pal4::AudioStreamState& stream) override {
        events.push_back("path-init");
        observed_vtables.push_back(stream.vtable);
        observed_words.push_back(stream.remaining_threshold_bits);
        stream.path = {0x9000u, 0u, 32u};
    }
    std::uint32_t CleanupPath(pal4::AudioStreamState& stream) override {
        events.push_back("path-clean");
        observed_vtables.push_back(stream.vtable);
        return 0x1234u;
    }
    void CloseMilesStream(std::uint32_t stream) override {
        events.push_back("miles-close:" + std::to_string(stream));
    }
    std::uint32_t InvokeCallback(std::uint32_t callback,
                                 std::uint32_t parent) override {
        events.push_back("callback:" + std::to_string(callback) + ":" +
                         std::to_string(parent));
        return 0x77u;
    }
    std::uint32_t CloseTempFile(std::uint32_t file) override {
        events.push_back("temp-close:" + std::to_string(file));
        return 0x55u;
    }
    std::uint32_t PauseMilesStream(std::uint32_t stream,
                                   bool paused) override {
        events.push_back("pause:" + std::to_string(stream) + ":" +
                         (paused ? "1" : "0"));
        return paused ? 0x11u : 0x22u;
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> observed_vtables;
    std::vector<std::uint32_t> observed_words;
};

void Lifecycle() {
    StreamRuntime runtime;
    pal4::AudioStreamState stream{};
    stream.started_at_words[0] = 0x1111u;
    stream.started_at_words[1] = 0x2222u;
    stream.fade_level_bits = 0x3333u;
    stream.remaining_threshold_bits = 0x4444u;
    Check(&pal4::AudioStream::Initialize(
              stream, 0x3000u, 0x5000u, runtime) == &stream &&
              stream.vtable == pal4::AudioStream::kStreamVtable &&
              stream.parent == 0x3000u && stream.active == 0u &&
              stream.stream == 0u && stream.loop_count == 1u &&
              stream.volume_bits == 0x3f800000u &&
              stream.fade_rate_bits == 0u && stream.path.data == 0x9000u &&
              stream.path.capacity == 32u &&
              stream.remaining_threshold_bits == 0u &&
              stream.completion_callback == 0x5000u &&
              stream.temp_file == 0u &&
              stream.started_at_words[0] == 0x1111u &&
              stream.started_at_words[1] == 0x2222u &&
              stream.fade_level_bits == 0x3333u &&
              runtime.observed_vtables ==
                  std::vector<std::uint32_t>{
                      pal4::AudioStream::kBaseVtable} &&
              runtime.observed_words == std::vector<std::uint32_t>{0x4444u},
          "constructor initializes observed fields and preserves unknowns");
    runtime.events.clear();
    runtime.observed_vtables.clear();
    stream.stream = 0x600u;
    stream.active = 1u;
    stream.temp_file = 0x700u;
    Check(pal4::AudioStream::Cleanup(stream, runtime) == 0x1234u &&
              runtime.events == std::vector<std::string>{
                  "miles-close:1536", "callback:20480:12288",
                  "temp-close:1792", "path-clean"} &&
              runtime.observed_vtables ==
                  std::vector<std::uint32_t>{
                      pal4::AudioStream::kStreamVtable} &&
              stream.vtable == pal4::AudioStream::kBaseVtable &&
              stream.stream == 0u && stream.active == 0u &&
              stream.completion_callback == 0u && stream.temp_file == 0u,
          "destructor cleans stream before path then restores base vtable");

    stream.completion_callback = 0x5000u;
    stream.temp_file = 0x700u;
    runtime.events.clear();
    Check(pal4::AudioStream::Release(stream, runtime) == 0u &&
              stream.completion_callback == 0x5000u &&
              stream.temp_file == 0x700u &&
              runtime.events.empty(),
          "release with no Miles stream leaves callback and file untouched");
}

void PlaybackState() {
    StreamRuntime runtime;
    pal4::AudioStreamState stream{};
    Check(pal4::AudioStream::Pause(stream, runtime) == 0u &&
              pal4::AudioStream::Resume(stream, runtime) == 0u &&
              !pal4::AudioStream::IsActive(stream) &&
              runtime.events.empty(),
          "inactive stream skips Miles pause and resume");
    stream.stream = 0x600u;
    stream.active = 2u;
    Check(pal4::AudioStream::Pause(stream, runtime) == 0x11u &&
              pal4::AudioStream::Resume(stream, runtime) == 0x22u &&
              pal4::AudioStream::IsActive(stream) &&
              runtime.events == std::vector<std::string>{
                  "pause:1536:1", "pause:1536:0"},
          "pause/resume forward Miles result and active uses nonzero word");
}

}  // namespace

int main() {
    Lifecycle();
    PlaybackState();
    std::cout << "Audio stream tests passed\n";
}
