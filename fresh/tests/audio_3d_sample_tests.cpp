#include "pal4/audio_3d_sample.h"

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

class Runtime final : public pal4::IAudio3DSampleRuntime {
public:
    std::array<std::uint32_t, 3u> DefaultVector() override {
        events.emplace_back("defaults");
        return {0x11u, 0x22u, 0x33u};
    }
    void InitializePath(pal4::Audio3DSampleState& sample) override {
        events.emplace_back("path-init");
        observed_vtables.push_back(sample.vtable);
        observed_vector48 = sample.vector48;
        sample.path = {0x9000u, 0u, 32u};
    }
    std::uint32_t CleanupPath(pal4::Audio3DSampleState& sample) override {
        events.emplace_back("path-clean");
        observed_vtables.push_back(sample.vtable);
        return 0x55u;
    }
    void Stop(pal4::Audio3DSampleState& sample) override {
        events.emplace_back("stop");
        observed_vtables.push_back(sample.vtable);
        sample.sample = 0u;
    }
    std::uint32_t FileCrc(std::uint32_t file) override {
        events.push_back("crc:" + std::to_string(file));
        return 0x1234u;
    }
    void ReleasePoolFile(std::uint32_t crc) override {
        events.push_back("release:" + std::to_string(crc));
    }
    std::uint32_t FreeObject(std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        return 0x77u;
    }

    std::vector<std::string> events;
    std::vector<std::uint32_t> observed_vtables;
    std::array<std::uint32_t, 3u> observed_vector48{};
};

void Lifecycle() {
    Runtime runtime;
    pal4::Audio3DSampleState sample{};
    sample.vector48 = {0xaau, 0xbbu, 0xccu};
    sample.unknown68 = 0x4444u;
    Check(&pal4::Audio3DSample::Initialize(
              sample, 0x3000u, runtime) == &sample &&
              sample.vtable == pal4::Audio3DSample::k3DVtable &&
              sample.parent == 0x3000u && sample.audio_file == 0u &&
              sample.sample == 0u && sample.loop_count == 1u &&
              sample.volume_bits == 0x3f800000u &&
              sample.vector24 ==
                  std::array<std::uint32_t, 3u>{0x11u, 0x22u, 0x33u} &&
              sample.vector36 == sample.vector24 &&
              sample.vector48 ==
                  std::array<std::uint32_t, 3u>{
                      0x11u, 0x22u, 0x3f800000u} &&
              sample.range60_bits == 1142292480u &&
              sample.range64_bits == 1133903872u &&
              sample.path.data == 0x9000u &&
              sample.unknown68 == 0x4444u &&
              runtime.events == std::vector<std::string>{
                  "defaults", "path-init"} &&
              runtime.observed_vtables ==
                  std::vector<std::uint32_t>{
                      pal4::Audio3DSample::kBaseVtable} &&
              runtime.observed_vector48 ==
                  std::array<std::uint32_t, 3u>{0xaau, 0xbbu, 0xccu},
          "constructor preserves order of vectors, string and vtables");
    runtime.events.clear();
    runtime.observed_vtables.clear();
    sample.sample = 0x600u;
    sample.audio_file = 0x700u;
    Check(pal4::Audio3DSample::Cleanup(sample, runtime) == 0x55u &&
              sample.vtable == pal4::Audio3DSample::kBaseVtable &&
              sample.sample == 0u && sample.audio_file == 0u &&
              runtime.events == std::vector<std::string>{
                  "stop", "crc:1792", "release:4660", "path-clean"} &&
              runtime.observed_vtables ==
                  std::vector<std::uint32_t>{
                      pal4::Audio3DSample::k3DVtable,
                      pal4::Audio3DSample::k3DVtable},
          "cleanup stops 3D sample and releases pool before path");
    runtime.events.clear();
    Check(pal4::Audio3DSample::DestroyAt(
              sample, 0x1000u, 0u, runtime) == 0x1000u &&
              runtime.events ==
                  std::vector<std::string>{"stop", "path-clean"},
          "nondelete destructor retains object allocation");
    runtime.events.clear();
    Check(pal4::Audio3DSample::DestroyAt(
              sample, 0x1000u, 2u, runtime) == 0x1000u &&
              runtime.events ==
                  std::vector<std::string>{"stop", "path-clean"},
          "unrelated delete flag leaves allocation intact");
    runtime.events.clear();
    Check(pal4::Audio3DSample::DestroyAt(
              sample, 0x1000u, 1u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>{
                  "stop", "path-clean", "free:4096"},
          "delete flag bit zero frees after nested cleanup");
}

}  // namespace

int main() {
    Lifecycle();
    std::cout << "Audio 3D sample tests passed\n";
}
