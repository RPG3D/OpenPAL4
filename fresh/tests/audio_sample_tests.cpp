#include "pal4/audio_sample.h"

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

class SampleRuntime final : public pal4::IAudioSampleControlRuntime {
public:
    void InitializePath(pal4::AudioSampleState& sample) override {
        events.emplace_back("path-init");
        observed_vtables.push_back(sample.vtable);
        sample.path = {0x9000u, 0u, 32u};
    }
    std::uint32_t CleanupPath(pal4::AudioSampleState& sample) override {
        events.emplace_back("path-clean");
        observed_vtables.push_back(sample.vtable);
        return 0x55u;
    }
    void EndMilesSample(std::uint32_t sample) override {
        events.push_back("end:" + std::to_string(sample));
    }
    std::uint32_t RecycleMilesSample(std::uint32_t sample) override {
        events.push_back("recycle:" + std::to_string(sample));
        return 0x44u;
    }
    std::uint32_t StopMilesSample(std::uint32_t sample) override {
        events.push_back("raw-stop:" + std::to_string(sample));
        return 0x11u;
    }
    std::uint32_t ResumeMilesSample(std::uint32_t sample) override {
        events.push_back("resume:" + std::to_string(sample));
        return 0x22u;
    }
    std::uint32_t MilesSampleStatus(std::uint32_t sample) override {
        events.push_back("status:" + std::to_string(sample));
        return status;
    }
    float SampleMasterVolume() override {
        events.emplace_back("master-volume");
        return 0.5f;
    }
    std::uint32_t SetMilesSampleVolume(std::uint32_t sample,
                                       std::uint32_t left,
                                       std::uint32_t right) override {
        events.push_back("volume:" + std::to_string(sample));
        Check(left == 0x3f000000u && right == left,
              "sample volume forwards equal half-volume bits");
        return 0x33u;
    }
    std::uint32_t FileCrc(std::uint32_t audio_file) override {
        events.push_back("crc:" + std::to_string(audio_file));
        return 0x1234u;
    }
    void ReleasePoolFile(std::uint32_t crc) override {
        events.push_back("release:" + std::to_string(crc));
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> observed_vtables;
    std::uint32_t status = 0u;
};

void Lifecycle() {
    SampleRuntime runtime;
    pal4::AudioSampleState sample{};
    sample.started_at_words[0] = 0x1111u;
    sample.started_at_words[1] = 0x2222u;
    Check(&pal4::AudioSample::Initialize(
              sample, 0x3000u, runtime) == &sample &&
              sample.vtable == pal4::AudioSample::kSampleVtable &&
              sample.parent == 0x3000u && sample.audio_file == 0u &&
              sample.sample == 0u && sample.loop_count == 1u &&
              sample.volume_bits == 0x3f800000u &&
              sample.path.data == 0x9000u &&
              sample.started_at_words[0] == 0x1111u &&
              sample.started_at_words[1] == 0x2222u &&
              runtime.observed_vtables ==
                  std::vector<std::uint32_t>{
                      pal4::AudioSample::kBaseVtable},
          "constructor writes known fields and initializes path under base vtable");
    runtime.events.clear();
    runtime.observed_vtables.clear();
    sample.sample = 0x600u;
    sample.audio_file = 0x700u;
    Check(pal4::AudioSample::Cleanup(sample, runtime) == 0x55u &&
              sample.vtable == pal4::AudioSample::kBaseVtable &&
              sample.audio_file == 0u && sample.sample == 0u &&
              runtime.events == std::vector<std::string>{
                  "end:1536", "recycle:1536", "crc:1792",
                  "release:4660", "path-clean"} &&
              runtime.observed_vtables ==
                  std::vector<std::uint32_t>{
                      pal4::AudioSample::kSampleVtable},
          "destructor stops sample before pool release and path cleanup");
    runtime.events.clear();
    Check(pal4::AudioSample::Cleanup(sample, runtime) == 0x55u &&
              runtime.events ==
                  std::vector<std::string>{"path-clean"},
          "destructor skips pool release when file pointer is null");
}

void PlaybackControls() {
    SampleRuntime runtime;
    pal4::AudioSampleState sample{};
    sample.volume_bits = 0x3f800000u;
    Check(pal4::AudioSample::Stop(sample, runtime) == 0u &&
              pal4::AudioSample::Resume(sample, runtime) == 0u &&
              pal4::AudioSample::ApplyVolume(sample, runtime) == 0u &&
              runtime.events.empty(),
          "guarded controls skip a missing sample");
    Check(pal4::AudioSample::StopImmediate(sample, runtime) == 0x11u &&
              runtime.events == std::vector<std::string>{"raw-stop:0"},
          "raw stop forwards even a null sample to Miles");
    runtime.events.clear();
    Check(!pal4::AudioSample::IsPlaying(sample, runtime) &&
              runtime.events == std::vector<std::string>{"status:0"},
          "status query also forwards a null sample");
    sample.sample = 0x600u;
    runtime.events.clear();
    runtime.status = 4u;
    Check(pal4::AudioSample::Resume(sample, runtime) == 0x22u &&
              pal4::AudioSample::IsPlaying(sample, runtime) &&
              pal4::AudioSample::ApplyVolume(sample, runtime) == 0x33u &&
              pal4::AudioSample::Stop(sample, runtime) == 0x44u &&
              sample.sample == 0u &&
              runtime.events == std::vector<std::string>{
                  "resume:1536", "status:1536", "master-volume",
                  "volume:1536", "end:1536", "recycle:1536"},
          "active sample controls forward Miles and recycling results");
}

}  // namespace

int main() {
    Lifecycle();
    PlaybackControls();
    std::cout << "Audio sample tests passed\n";
}
