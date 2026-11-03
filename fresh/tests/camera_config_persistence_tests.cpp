#include "pal4/camera_config_persistence.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraConfigRuntime {
public:
    pal4::CameraSystemObjectState object{};
    pal4::CameraDataState camera{};
    std::array<bool, 3> existing{};
    std::vector<std::string> calls;
    std::string throw_on;
    std::uint32_t destroys = 0u;

    static std::size_t Index(std::string_view key) {
        if (key == "current-camera-mode") return 0u;
        if (key == "current-camera-YAW") return 1u;
        if (key == "current-camera-PIT") return 2u;
        throw std::runtime_error("unexpected camera config key");
    }
    static std::string Label(std::string_view key) {
        constexpr std::array<const char*, 3> labels{
            "mode", "yaw", "pit"};
        return labels[Index(key)];
    }
    void Record(const std::string& call) {
        calls.push_back(call);
        if (call == throw_on) throw std::runtime_error("injected failure");
    }
    std::uint32_t CreateNode(std::string_view name) override {
        Check(name == "CAM_MGR", "camera config node name");
        Record("create");
        return 0x4000u;
    }
    bool HasAttribute(std::uint32_t node,
                      std::string_view name) override {
        Check(node == 0x4000u, "attribute lookup node");
        Record("has:" + Label(name));
        return existing[Index(name)];
    }
    void SetInt(std::uint32_t node, std::string_view name,
                std::uint32_t value) override {
        Check(node == 0x4000u && Index(name) == 0u && value == 4u,
              "existing mode value");
        Record("set:mode");
    }
    void AddInt(std::uint32_t node, std::string_view name,
                std::uint32_t value) override {
        Check(node == 0x4000u && Index(name) == 0u && value == 4u,
              "new mode value");
        Record("add:mode");
    }
    void SetFloat(std::uint32_t node, std::string_view name,
                  float value) override {
        Check(node == 0x4000u, "existing float node");
        CheckFloat(name, value);
        Record("set:" + Label(name));
    }
    void AddFloat(std::uint32_t node, std::string_view name,
                  float value) override {
        Check(node == 0x4000u, "new float node");
        CheckFloat(name, value);
        Record("add:" + Label(name));
    }
    void CheckFloat(std::string_view name, float value) {
        const auto expected = Index(name) == 1u ?
            std::bit_cast<std::uint32_t>(-30.5f) :
            std::bit_cast<std::uint32_t>(42.25f);
        Check(Index(name) != 0u &&
                  std::bit_cast<std::uint32_t>(value) == expected,
              "camera angle bits");
    }
    pal4::CameraSystemObjectState& ObjectAt(
        std::uint32_t address) override {
        Check(address == 0x1000u, "config reads Main camera slot");
        Record("object");
        return object;
    }
    pal4::CameraDataState& CameraDataAt(
        std::uint32_t address) override {
        Check(address == 0x2000u, "config reads Main inner camera");
        Record("camera");
        return camera;
    }
    std::uint32_t PrepareWriter(std::uint32_t argument) override {
        Check(argument == 0x1234u, "writer argument");
        Record("prepare");
        return 0x5678u;
    }
    std::uint8_t Write(std::uint32_t node,
                       std::uint32_t writer) override {
        Check(node == 0x4000u && writer == 0x5678u,
              "prepared writer passed to config output");
        Record("write");
        return 0xa5u;
    }
    void DestroyNode(std::uint32_t node) override {
        Check(node == 0x4000u, "destroy camera config node");
        ++destroys;
        calls.emplace_back("destroy");
    }
};

void Cases() {
    Runtime runtime;
    runtime.object.camera = 0x2000u;
    runtime.camera.At(0x3cu) = std::bit_cast<std::uint32_t>(-30.5f);
    runtime.camera.At(0x40u) = std::bit_cast<std::uint32_t>(42.25f);
    pal4::CameraControllerState state;
    state.At(0x34u) = 0x1000u;
    state.At(0x30u) = 0x9999u;
    state.At(0xc0u) = 4u;
    pal4::CameraConfigPersistence config(runtime);

    for (unsigned mask = 0u; mask < 8u; ++mask) {
        runtime.calls.clear();
        runtime.destroys = 0u;
        for (unsigned index = 0u; index < 3u; ++index)
            runtime.existing[index] = (mask & (1u << index)) != 0u;
        Check(config.Save(state, 0x1234u) == 0xa5u &&
                  runtime.destroys == 1u,
              "config save returns low write byte and destroys node");
        const std::vector<std::string> expected{
            "create", "has:mode",
            runtime.existing[0] ? "set:mode" : "add:mode",
            "object", "camera", "has:yaw",
            runtime.existing[1] ? "set:yaw" : "add:yaw",
            "has:pit", runtime.existing[2] ? "set:pit" : "add:pit",
            "prepare", "write", "destroy"};
        Check(runtime.calls == expected,
              "mode yaw pitch create/update and write order");
    }

    runtime.existing = {false, false, false};
    for (const auto& failure : {"add:yaw", "write"}) {
        runtime.calls.clear();
        runtime.destroys = 0u;
        runtime.throw_on = failure;
        bool caught = false;
        try {
            static_cast<void>(config.Save(state, 0x1234u));
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.destroys == 1u &&
                  runtime.calls.back() == "destroy",
              "save failure destroys node once");
    }
    runtime.throw_on.clear();

    pal4::CameraConfigRuntimeStub stub;
    pal4::CameraConfigPersistence stub_config(stub);
    Check(stub_config.Save(state, 0u) == 0u,
          "native config stub is linkable and side-effect free");
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
