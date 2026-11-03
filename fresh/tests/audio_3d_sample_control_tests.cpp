#include "pal4/audio_3d_sample.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class Runtime final : public pal4::IAudio3DSampleControlRuntime {
public:
    std::array<std::uint32_t, 3u> DefaultVector() override { return {}; }
    void InitializePath(pal4::Audio3DSampleState&) override {}
    std::uint32_t CleanupPath(pal4::Audio3DSampleState&) override { return 0u; }
    void Stop(pal4::Audio3DSampleState&) override {}
    std::uint32_t FileCrc(std::uint32_t) override { return 0u; }
    void ReleasePoolFile(std::uint32_t) override {}
    std::uint32_t FreeObject(std::uint32_t) override { return 0u; }
    void EndMiles3DSample(std::uint32_t handle) override {
        events.emplace_back("end");
        observed_handle = handle;
    }
    std::uint32_t RecycleMiles3DSample(std::uint32_t handle) override {
        events.emplace_back("recycle");
        observed_handle = handle;
        return 0x41u;
    }
    std::uint32_t StopMiles3DSample(std::uint32_t handle) override {
        events.emplace_back("stop-immediate");
        observed_handle = handle;
        return 0x42u;
    }
    std::uint32_t ResumeMiles3DSample(std::uint32_t handle) override {
        events.emplace_back("resume");
        observed_handle = handle;
        return 0x43u;
    }
    std::uint32_t Miles3DSampleStatus(std::uint32_t handle) override {
        events.emplace_back("status");
        observed_handle = handle;
        return status;
    }
    float SampleMasterVolume() override {
        events.emplace_back("master");
        return 0.5f;
    }
    std::uint32_t SetMiles3DSampleVolume(
        std::uint32_t handle, std::uint32_t bits) override {
        events.emplace_back("volume");
        observed_handle = handle;
        observed_volume = bits;
        return 0x44u;
    }
    std::uint32_t SetMiles3DPosition(
        std::uint32_t handle, std::uint32_t x,
        std::uint32_t y, std::uint32_t z) override {
        events.emplace_back("position");
        observed_handle = handle;
        observed_position = {x, y, z};
        return 0x45u;
    }

    std::vector<std::string> events;
    std::uint32_t observed_handle = 0u;
    std::uint32_t observed_volume = 0u;
    std::uint32_t status = 4u;
    std::array<std::uint32_t, 3u> observed_position{};
};

void ControlMethods() {
    Runtime runtime;
    pal4::Audio3DSampleState state{};
    Check(pal4::Audio3DSample::Stop(state, runtime) == 0u &&
              pal4::Audio3DSample::Resume(state, runtime) == 0u &&
              pal4::Audio3DSample::ApplyVolume(state, runtime) == 0u &&
              runtime.events.empty(),
          "null handle gates stop, resume and volume");
    Check(pal4::Audio3DSample::StopImmediate(state, runtime) == 0x42u &&
              pal4::Audio3DSample::IsPlaying(state, runtime) &&
              runtime.observed_handle == 0u &&
              runtime.events == std::vector<std::string>{
                  "stop-immediate", "status"},
          "raw stop and status forward zero handle");
    runtime.events.clear();
    runtime.status = 3u;
    Check(!pal4::Audio3DSample::IsPlaying(state, runtime),
          "status other than four is not playing");
    runtime.events.clear();
    state.sample = 0x600u;
    state.volume_bits = 0x3f800000u;
    Check(pal4::Audio3DSample::Resume(state, runtime) == 0x43u &&
              pal4::Audio3DSample::ApplyVolume(state, runtime) == 0x44u &&
              runtime.observed_handle == 0x600u &&
              runtime.observed_volume == 0x3f000000u &&
              runtime.events == std::vector<std::string>{
                  "resume", "master", "volume"},
          "resume and master-adjusted volume forward Miles return values");
    runtime.events.clear();
    Check(pal4::Audio3DSample::SetPosition(
              state, {0x3f800000u, 0x40000000u, 0x40400000u},
              runtime) == 0x45u &&
              state.vector24 == std::array<std::uint32_t, 3u>{
                  0x3f800000u, 0x40000000u, 0x40400000u} &&
              runtime.observed_position == std::array<std::uint32_t, 3u>{
                  0x3f800000u, 0x40000000u, 0xc0400000u} &&
              runtime.events == std::vector<std::string>{"position"},
          "position stores original vector and negates Z for Miles");
    runtime.events.clear();
    Check(pal4::Audio3DSample::Stop(state, runtime) == 0x41u &&
              state.sample == 0u && runtime.observed_handle == 0x600u &&
              runtime.events == std::vector<std::string>{
                  "end", "recycle"},
          "stop ends and recycles handle before clearing it");
}

}  // namespace

int main() {
    ControlMethods();
    std::cout << "Audio 3D sample control tests passed\n";
}
