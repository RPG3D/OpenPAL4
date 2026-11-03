#include "pal4/camera_config_loader.h"
#include "pal4/camera_controller_access.h"
#include "pal4/camera_mode_configuration.h"
#include "pal4/camera_parameter_setters.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraConfigLoadRuntime,
                      public pal4::ICameraModeConfigurationRuntime,
                      public pal4::ICameraControllerAccessRuntime,
                      public pal4::ICameraParameterRuntime {
public:
    pal4::CameraSystemObjectState object{};
    pal4::CameraDataState camera{};
    std::array<bool, 3> existing{};
    std::vector<std::string> calls;
    bool compatible = true;
    bool invalid_mode = false;
    std::string throw_on;
    std::uint32_t destroys = 0u;
    std::uint32_t matrix_updates = 0u;

    static std::size_t Index(std::string_view key) {
        if (key == "current-camera-mode") return 0u;
        if (key == "current-camera-PIT") return 1u;
        if (key == "current-camera-YAW") return 2u;
        throw std::runtime_error("unexpected camera config key");
    }
    static std::string Label(std::size_t index) {
        return std::array<const char*, 3>{"mode", "pit", "yaw"}[index];
    }
    void Record(const std::string& call) {
        calls.push_back(call);
        if (call == throw_on) throw std::runtime_error("injected failure");
    }
    std::uint32_t CreateNode(std::string_view name) override {
        Check(name == "CAM_MGR", "load node name");
        Record("create");
        return 0x4000u;
    }
    std::uint32_t PrepareReader(std::uint32_t argument) override {
        Check(argument == 0x1234u, "reader argument");
        Record("prepare");
        return 0x5678u;
    }
    bool IsCompatible(std::uint32_t reader,
                      std::uint32_t node) override {
        Check(reader == 0x5678u && node == 0x4000u,
              "version check operands");
        Record("compatible");
        return compatible;
    }
    std::optional<std::uint32_t> FindAttribute(
        std::uint32_t node, std::string_view name) override {
        Check(node == 0x4000u, "attribute node");
        const auto index = Index(name);
        Record("find:" + Label(index));
        return existing[index] ?
            std::optional<std::uint32_t>(0x5000u +
                                         static_cast<std::uint32_t>(index)) :
            std::nullopt;
    }
    std::uint32_t ParseInt(std::uint32_t attribute,
                           std::uint32_t fallback) override {
        Check(attribute == 0x5000u && fallback == 0u,
              "mode parse operands");
        Record("parse:mode");
        return invalid_mode ? 6u : 2u;
    }
    float ParseFloat(std::uint32_t attribute,
                     float fallback) override {
        Check(attribute == 0x5001u || attribute == 0x5002u,
              "angle parse attribute");
        const auto selected = existing[0] && !invalid_mode;
        const auto expected = attribute == 0x5001u ?
            (selected ? 40.0f : 20.0f) :
            (selected ? 30.0f : 10.0f);
        Check(fallback == expected, "angle parse fallback from mode table");
        Record(attribute == 0x5001u ? "parse:pit" : "parse:yaw");
        return attribute == 0x5001u ? 88.0f : 77.0f;
    }
    std::uint32_t ReadWord(std::uint32_t) override {
        throw std::runtime_error("unexpected external table read");
    }
    pal4::CameraSystemObjectState& ObjectAt(
        std::uint32_t address) override {
        Check(address == 0x1000u, "load uses Main camera");
        Record("object");
        return object;
    }
    pal4::CameraDataState& CameraDataAt(
        std::uint32_t address) override {
        Check(address == 0x2000u, "load uses Main inner camera");
        Record("camera");
        return camera;
    }
    void DestroyNode(std::uint32_t node) override {
        Check(node == 0x4000u, "load node destruction");
        ++destroys;
        calls.emplace_back("destroy");
    }
    void SetFarClip(std::uint32_t, float) override {
        throw std::runtime_error("unexpected far clip");
    }
    std::optional<std::uint32_t> FindNamedObject(
        const pal4::CameraControllerState&, std::uint32_t,
        const char*) override {
        throw std::runtime_error("unexpected dynamic camera lookup");
    }
    void ReportMissingCameraName(const char*) override {
        throw std::runtime_error("unexpected camera name error");
    }
    void ClearObjectCollection(std::uint32_t,
                               std::uint32_t) override {
        throw std::runtime_error("unexpected collection cleanup");
    }
    float MinimumDistance() override { return 0.0f; }
    float MaximumDistance() override { return 1000.0f; }
    std::uint32_t UpdateMatrix(pal4::CameraDataState& state,
                               std::uint32_t mode) override {
        Check(&state == &camera && mode == 1u,
              "matrix update camera and mode");
        ++matrix_updates;
        calls.emplace_back("matrix");
        return 0x55u;
    }
};

void SetTable(pal4::CameraControllerState& state,
              std::uint32_t index, float yaw, float pitch,
              float distance, std::uint32_t flag) {
    const auto offset = 0x44u + index * 20u;
    state.At(offset) = std::bit_cast<std::uint32_t>(yaw);
    state.At(offset + 4u) = std::bit_cast<std::uint32_t>(pitch);
    state.At(offset + 8u) = std::bit_cast<std::uint32_t>(distance);
    state.At(offset + 12u) = flag;
}

void Cases() {
    Runtime runtime;
    runtime.object.camera = 0x2000u;
    runtime.camera.At(0u) = 0x9000u;
    pal4::CameraParameterSetters parameters(runtime);
    pal4::CameraControllerAccess access(runtime);
    pal4::CameraModeConfiguration modes(parameters, access, runtime);
    pal4::CameraConfigLoader loader(runtime, modes, parameters);
    pal4::CameraControllerState state;
    state.At(0x34u) = 0x1000u;
    state.At(0x30u) = 0x9999u;
    SetTable(state, 0u, 10.0f, 20.0f, 200.0f, 1u);
    SetTable(state, 2u, 30.0f, 40.0f, 300.0f, 2u);

    runtime.compatible = false;
    state.At(0xc0u) = 5u;
    state.At(0xbcu) = 0x3044u;
    const auto before_mismatch = state.words;
    Check(!loader.Load(state, 0x3000u, 0x1234u) &&
              state.words == before_mismatch && runtime.destroys == 1u &&
              runtime.calls == std::vector<std::string>{
                  "create", "prepare", "compatible", "destroy"},
          "incompatible config returns before mutating controller");
    runtime.compatible = true;

    for (unsigned mask = 0u; mask < 8u; ++mask) {
        runtime.calls.clear();
        runtime.destroys = 0u;
        runtime.matrix_updates = 0u;
        runtime.invalid_mode = false;
        for (unsigned index = 0u; index < 3u; ++index)
            runtime.existing[index] = (mask & (1u << index)) != 0u;
        state.At(0xc0u) = 5u;
        state.At(0xbcu) = 0x3044u;
        const auto selected = runtime.existing[0] ? 2u : 0u;
        const auto default_yaw = selected == 2u ? 30.0f : 10.0f;
        const auto default_pitch = selected == 2u ? 40.0f : 20.0f;
        Check(loader.Load(state, 0x3000u, 0x1234u) &&
                  state.At(0xc0u) == selected &&
                  state.At(0xbcu) == 0x3044u + selected * 20u &&
                  runtime.destroys == 1u &&
                  runtime.matrix_updates == 7u &&
                  runtime.camera.At(0x3cu) == std::bit_cast<std::uint32_t>(
                      runtime.existing[2] ? 77.0f : default_yaw) &&
                  runtime.camera.At(0x40u) == std::bit_cast<std::uint32_t>(
                      runtime.existing[1] ? 88.0f : default_pitch) &&
                  runtime.camera.At(0x44u) == 0u &&
                  state.At(0x30u) == 0x9999u,
              "mode and optional pitch/yaw load into Main camera");
        std::vector<std::string> expected{
            "create", "prepare", "compatible", "find:mode"};
        if (runtime.existing[0]) expected.emplace_back("parse:mode");
        expected.insert(expected.end(),
                        {"object", "camera", "matrix", "matrix",
                         "matrix", "matrix", "find:pit"});
        if (runtime.existing[1]) expected.emplace_back("parse:pit");
        expected.emplace_back("find:yaw");
        if (runtime.existing[2]) expected.emplace_back("parse:yaw");
        expected.insert(expected.end(),
                        {"object", "camera", "matrix", "matrix",
                         "matrix", "destroy"});
        Check(runtime.calls == expected,
              "version, mode, PIT, YAW and matrix update order");
    }

    runtime.calls.clear();
    runtime.destroys = 0u;
    runtime.matrix_updates = 0u;
    runtime.existing = {true, false, false};
    runtime.invalid_mode = true;
    state.At(0xbcu) = 0x3044u;
    Check(loader.Load(state, 0x3000u, 0x1234u) &&
              state.At(0xc0u) == 6u && state.At(0xbcu) == 0x3044u &&
              runtime.matrix_updates == 3u &&
              runtime.camera.At(0x3cu) ==
                  std::bit_cast<std::uint32_t>(10.0f) &&
              runtime.camera.At(0x40u) ==
                  std::bit_cast<std::uint32_t>(20.0f),
          "invalid parsed mode keeps previous table pointer for defaults");

    runtime.invalid_mode = false;
    runtime.existing = {true, true, true};
    runtime.calls.clear();
    runtime.destroys = 0u;
    runtime.throw_on = "parse:yaw";
    bool failed = false;
    try {
        static_cast<void>(loader.Load(state, 0x3000u, 0x1234u));
    } catch (const std::runtime_error&) {
        failed = true;
    }
    Check(failed && runtime.destroys == 1u &&
              runtime.calls.back() == "destroy",
          "parse exception releases XML node once");

    pal4::CameraConfigLoadRuntimeStub stub;
    pal4::CameraConfigLoader stub_loader(stub, modes, parameters);
    Check(!stub_loader.Load(state, 0x3000u, 0u),
          "linkable load stub rejects unavailable native XML");
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
