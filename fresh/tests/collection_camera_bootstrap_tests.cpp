#include "pal4/collection_camera_bootstrap.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CollectionCameraRuntimeStub {
    std::vector<char> events;
    bool found{true};
    int internal_reads{};

    std::uint32_t InitializeCameraSubsystem() override {
        events.push_back('I');
        return 0x1000u;
    }
    std::uint32_t FindCamera(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x1000u && name == "UiCamera",
              "camera lookup uses registered manager and UiCamera");
        events.push_back('F');
        return found ? 0x2000u : 0u;
    }
    std::uint32_t InternalObject(std::uint32_t camera) override {
        Check(camera == 0x2000u, "internal object reads camera first word");
        ++internal_reads;
        events.push_back('O');
        return 0x3000u;
    }
    void SetAngles(std::uint32_t internal,
                   std::uint32_t first_angle_bits,
                   std::uint32_t second_angle_bits,
                   std::uint32_t zero, std::uint32_t mode) override {
        Check(internal == 0x3000u && first_angle_bits == 0x43350000u &&
                  second_angle_bits == 0x43b0c000u && zero == 0u &&
                  mode == 1u,
              "angle update preserves the four original raw arguments");
        events.push_back('A');
    }
    void SetLookAt(std::uint32_t internal,
                   const std::array<std::uint32_t, 3>& words,
                   std::uint32_t mode) override {
        Check(internal == 0x3000u && mode == 1u &&
                  words == std::array<std::uint32_t, 3>{
                      0x412220c5u, 0x43d20000u, 0xc400f0c5u},
              "look-at update preserves the three stack float words");
        events.push_back('L');
    }
    void SetScalar(std::uint32_t internal,
                   std::uint32_t scalar_bits,
                   std::uint32_t mode) override {
        Check(internal == 0x3000u && scalar_bits == 0x448cc000u &&
                  mode == 1u,
              "scalar update uses 1126.0f and mode one");
        events.push_back('S');
    }
    std::uint32_t CameraContainer(std::uint32_t camera) override {
        Check(camera == 0x2000u,
              "container lookup receives camera object");
        events.push_back('C');
        return 0x4000u;
    }
    void SetWorldCamera(pal4::CollectionState&,
                        std::uint32_t container) override {
        Check(container == 0x4000u,
              "world camera receives recovered container");
        events.push_back('W');
    }
    std::uint32_t InitializeLogger() override {
        events.push_back('G');
        return 0x5000u;
    }
    std::uint32_t AppendLog(std::uint32_t logger,
                            std::string_view format,
                            std::string_view source,
                            std::uint32_t line) override {
        Check(logger == 0x5000u && line == 154u &&
                  format == "FILE:  [%s]\nLINE:  [%d]\nINFO:  "
                            "uiSceneManager::initializeUiCamera OK\n\n\n" &&
                  source == "D:\\VcProject\\Pal4\\project\\Sourcecode\\"
                            "PAL4\\PAL4Game\\UiFrame\\uiScene\\"
                            "uiSceneManager.cpp",
              "logger receives exact IDA strings and source line");
        events.push_back('P');
        return 0x77u;
    }
};

void CheckSetup() {
    pal4::CollectionState state{};
    Runtime runtime;
    Check(pal4::CollectionCameraBootstrap::InitializeAndSetup(
              state, runtime) == 0x77u &&
              runtime.events == std::vector<char>{
                  'I', 'F', 'O', 'A', 'O', 'L', 'O', 'S',
                  'C', 'W', 'G', 'P'} &&
              runtime.internal_reads == 3,
          "setup configures camera in IDA order and forwards log result");
    runtime.events.clear();
    runtime.internal_reads = 0;
    Check(pal4::CollectionCameraBootstrap::GetActiveCamera(runtime) ==
              0x3000u &&
              runtime.events == std::vector<char>{'I', 'F', 'O'} &&
              runtime.internal_reads == 1,
          "active camera returns the first object word");
}

void CheckMissingCamera() {
    pal4::CollectionState state{};
    Runtime runtime;
    runtime.found = false;
    Check(pal4::CollectionCameraBootstrap::InitializeAndSetup(
              state, runtime) == 0u &&
              runtime.events == std::vector<char>{'I', 'F'},
          "missing camera skips setup and logging");
    runtime.events.clear();
    Check(pal4::CollectionCameraBootstrap::GetActiveCamera(runtime) ==
              0u &&
              runtime.events == std::vector<char>{'I', 'F'},
          "active camera returns zero when lookup fails");
}

}  // namespace

int main() {
    CheckSetup();
    CheckMissingCamera();
    std::cout << "collection camera bootstrap tests passed\n";
}
