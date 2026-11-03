#include "pal4/camera_mode_configuration.h"
#include "pal4/camera_controller_access.h"
#include "pal4/camera_parameter_setters.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraModeConfigurationRuntime,
                      public pal4::ICameraControllerAccessRuntime,
                      public pal4::ICameraParameterRuntime {
public:
    std::array<pal4::CameraSystemObjectState, 2> objects{};
    std::array<pal4::CameraDataState, 2> cameras{};
    std::array<std::uint32_t, 2> updates{};
    std::vector<std::string> calls;
    bool checking_indexed_mode = false;

    pal4::CameraSystemObjectState& ObjectAt(
        std::uint32_t address) override {
        Check(address >= 0x1000u && address < 0x1002u,
              "object address");
        return objects[address - 0x1000u];
    }
    pal4::CameraDataState& CameraDataAt(
        std::uint32_t address) override {
        Check(address >= 0x2000u && address < 0x2002u,
              "camera address");
        return cameras[address - 0x2000u];
    }
    void SetFarClip(std::uint32_t, float) override {
        throw std::runtime_error("unexpected far clip update");
    }
    std::optional<std::uint32_t> FindNamedObject(
        const pal4::CameraControllerState&, std::uint32_t,
        const char*) override {
        throw std::runtime_error("unexpected dynamic lookup");
    }
    void ReportMissingCameraName(const char*) override {
        throw std::runtime_error("unexpected missing camera");
    }
    void ClearObjectCollection(std::uint32_t object,
                               std::uint32_t offset) override {
        Check(offset == 0x10u && object >= 0x1000u && object < 0x1002u,
              "collection address and offset");
        const auto index = object - 0x1000u;
        Check(updates[index] == 5u,
              "collection cleanup follows camera setters");
        calls.emplace_back("collection:" + std::to_string(index));
    }
    float MinimumDistance() override { return 10.0f; }
    float MaximumDistance() override { return 500.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState& camera,
                               std::uint32_t mode) override {
        Check(mode == 1u, "matrix update mode");
        const auto index = &camera == &cameras[0] ? 0u : 1u;
        Check(&camera == &cameras[index], "camera state identity");
        if (checking_indexed_mode) {
            Check(index == 0u && updates[index] < 4u,
                  "indexed mode updates only main camera four times");
            const auto step = updates[index]++;
            Check(camera.At(0x3cu) == std::bit_cast<std::uint32_t>(42.0f) &&
                      (step == 0u || camera.At(0x40u) ==
                           std::bit_cast<std::uint32_t>(270.0f)) &&
                      (step < 2u || camera.At(0x44u) == 0u) &&
                      (step < 3u || camera.At(0x4cu) ==
                           std::bit_cast<std::uint32_t>(430.0f)),
                  "indexed mode writes fields before each matrix update");
            constexpr std::array<const char*, 4> steps{
                "first", "second", "third", "distance"};
            calls.emplace_back(std::string("0:") + steps[step]);
            return 0x33u;
        }
        constexpr std::array<const char*, 5> steps{
            "lookat", "first", "second", "third", "distance"};
        Check(updates[index] < steps.size(), "matrix update count");
        calls.emplace_back(std::to_string(index) + ":" +
                           steps[updates[index]++]);
        return 0x33u;
    }
};

void Cases() {
    Runtime runtime;
    runtime.objects[0].camera = 0x2000u;
    runtime.objects[1].camera = 0x2001u;
    for (auto& camera : runtime.cameras) {
        camera.At(0) = 0x9000u;
        camera.At(0x78u) = std::bit_cast<std::uint32_t>(12.0f);
        camera.At(0x2bcu) = 0xdeadbeefu;
    }
    pal4::CameraParameterSetters parameters(runtime);
    pal4::CameraControllerAccess access(runtime);
    pal4::CameraModeConfiguration modes(parameters, access, runtime);
    pal4::CameraControllerState state;
    state.At(0x34u) = 0x1000u;
    state.At(0x38u) = 0x1001u;
    state.At(0x44u) = std::bit_cast<std::uint32_t>(30.0f);
    state.At(0x48u) = std::bit_cast<std::uint32_t>(225.0f);
    state.At(0x4cu) = std::bit_cast<std::uint32_t>(350.0f);
    state.At(0x50u) = 3u;
    state.At(0x4u) = 0x66u;
    state.At(0x8u) = 0x77u;
    state.At(0xbcu) = 0u;
    state.At(0xc0u) = 0xdeadbeefu;

    Check(modes.Configure(state, 0x3000u) == 0x77u,
          "mode configuration returns old dynamic collection end");
    Check(state.At(0xbcu) == 0x3044u && state.At(0xc0u) == 0u &&
              state.At(0x30u) == 0x1000u && state.At(0x8u) == 0x66u,
          "mode configuration sets table pointer and clears collection");
    const std::vector<std::string> expected{
        "0:lookat", "0:first", "0:second", "0:third",
        "0:distance", "collection:0", "1:lookat", "1:first",
        "1:second", "1:third", "1:distance", "collection:1"};
    Check(runtime.calls == expected, "two camera mode setup order");
    for (std::size_t index = 0; index < runtime.cameras.size(); ++index) {
        const auto& camera = runtime.cameras[index];
        Check(camera.At(0x8cu) == 0u &&
                  camera.At(0xa4u) == std::bit_cast<std::uint32_t>(12.0f) &&
                  camera.At(0x74u) == 0u && camera.At(0x78u) == 0u &&
                  camera.At(0x7cu) == 0u &&
                  camera.At(0x3cu) == state.At(0x44u) &&
                  camera.At(0x40u) == state.At(0x48u) &&
                  camera.At(0x44u) == 0u &&
                  camera.At(0x48u) == state.At(0x4cu) &&
                  camera.At(0x4cu) == state.At(0x4cu) &&
                  camera.At(0x1a8u) == 3u,
              "camera mode parameters and active-state snapshot");
    }
    Check(runtime.cameras[0].At(0x2bcu) == 0x5ec270u &&
              runtime.cameras[1].At(0x2bcu) == 0xdeadbeefu,
          "environment callback applies only to main camera");

    const auto before_invalid = state;
    runtime.calls.clear();
    Check(!modes.SetModeByIndex(state, 0x3000u, 6u) &&
              state.words == before_invalid.words && runtime.calls.empty(),
          "invalid mode index leaves state and runtime untouched");

    state.At(0x44u + 5u * 20u) = std::bit_cast<std::uint32_t>(42.0f);
    state.At(0x48u + 5u * 20u) = std::bit_cast<std::uint32_t>(270.0f);
    state.At(0x4cu + 5u * 20u) = std::bit_cast<std::uint32_t>(430.0f);
    state.At(0x50u + 5u * 20u) = 9u;
    runtime.cameras[0].At(0x44u) =
        std::bit_cast<std::uint32_t>(17.0f);
    runtime.updates[0] = 0u;
    runtime.checking_indexed_mode = true;
    runtime.calls.clear();
    Check(modes.SetModeByIndex(state, 0x3000u, 5u) &&
              state.At(0xc0u) == 5u && state.At(0xbcu) == 0x30a8u &&
              runtime.calls == std::vector<std::string>{
                  "0:first", "0:second", "0:third", "0:distance"},
          "last mode updates only main camera in setter order");
    Check(runtime.cameras[0].At(0x3cu) ==
                  std::bit_cast<std::uint32_t>(42.0f) &&
              runtime.cameras[0].At(0x40u) ==
                  std::bit_cast<std::uint32_t>(270.0f) &&
              runtime.cameras[0].At(0x44u) == 0u &&
              runtime.cameras[0].At(0x4cu) ==
                  std::bit_cast<std::uint32_t>(430.0f) &&
              runtime.cameras[0].At(0x1a8u) == 9u &&
              runtime.updates[1] == 5u,
          "indexed entry writes angles distance and flag only to main camera");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
