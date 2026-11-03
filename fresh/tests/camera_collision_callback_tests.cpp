#include "pal4/camera_collision_callback.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraSubsystemRuntime,
                      public pal4::ICameraControllerRuntime,
                      public pal4::ICollectionCollisionRuntime {
public:
    pal4::CameraControllerState controller;
    const float* expected_query = nullptr;
    std::vector<std::string> calls;

    std::uint32_t SingletonManager() override {
        calls.emplace_back("singleton");
        return 0x1000u;
    }
    std::uint32_t Register(std::uint32_t manager, const char* name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t cleanup) override {
        Check(manager == 0x1000u && std::string(name) == "palCameraManager" &&
                  id == 1002u && factory == 0x5eae20u &&
                  cleanup == 0x5eae80u,
              "camera manager registration operands");
        calls.emplace_back("register");
        return 0x3000u;
    }
    std::uint32_t Allocate(std::uint32_t) override {
        throw std::runtime_error("unexpected controller allocation");
    }
    pal4::CameraControllerState& ControllerAt(
        std::uint32_t address) override {
        Check(address == 0x3000u, "registered controller address");
        calls.emplace_back("controller");
        return controller;
    }
    void CleanupCameraObject(std::uint32_t) override {
        throw std::runtime_error("unexpected camera cleanup");
    }
    std::uint32_t Free(std::uint32_t) override {
        throw std::runtime_error("unexpected memory free");
    }
    void SetName(std::uint32_t&, const char*) override {
        throw std::runtime_error("unexpected name initialization");
    }
    void DestroyNameObject(pal4::CameraControllerState&,
                           std::uint32_t) override {
        throw std::runtime_error("unexpected name cleanup");
    }
    std::uint32_t FreeData(std::uint32_t) override {
        throw std::runtime_error("unexpected data free");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x4000u || address == 0x4004u,
              "collision collection slot address");
        calls.emplace_back(address == 0x4000u ? "first" : "second");
        return address == 0x4000u ? 0x5000u : 0x5001u;
    }
    std::optional<pal4::CollectionCollisionHit> Probe(
        const float* query, std::uint32_t object) override {
        Check(query == expected_query &&
                  (object == 0x5000u || object == 0x5001u),
              "collision query and clump handle");
        calls.emplace_back(object == 0x5000u ? "probe first" : "probe second");
        if (object == 0x5000u)
            return pal4::CollectionCollisionHit{7.0f, {1u, 2u, 3u}};
        return pal4::CollectionCollisionHit{4.0f, {4u, 5u, 6u}};
    }
};

void Cases() {
    Runtime runtime;
    runtime.controller.At(4u) = 0x4000u;
    runtime.controller.At(8u) = 0x4008u;
    pal4::CameraController controller(runtime);
    pal4::CameraSubsystemLifecycle lifecycle(runtime, controller);
    pal4::CameraSubsystemRegistrationState registration{};
    pal4::CollectionCollisionScanner scanner(runtime);
    pal4::CameraCollisionCallback callback(
        lifecycle, registration, runtime, scanner);
    const std::array<float, 3> query{1.0f, 2.0f, 3.0f};
    runtime.expected_query = query.data();
    float limit = 10.0f;
    std::array<std::uint32_t, 3> point{9u, 9u, 9u};

    Check(callback.Probe(query.data(), limit, point) &&
              registration.guard == 1u && registration.instance == 0x3000u &&
              limit == 4.0f &&
              point == std::array<std::uint32_t, 3>{4u, 5u, 6u} &&
              runtime.calls == std::vector<std::string>{
                  "singleton", "register", "controller",
                  "first", "probe first", "second", "probe second"},
          "callback registers camera manager then scans collection");

    runtime.calls.clear();
    limit = 3.0f;
    point = {10u, 11u, 12u};
    Check(!callback.Probe(query.data(), limit, point) &&
              limit == 3.0f &&
              point == std::array<std::uint32_t, 3>{10u, 11u, 12u} &&
              runtime.calls == std::vector<std::string>{
                  "controller", "first", "probe first",
                  "second", "probe second"},
          "cached singleton still scans and preserves smaller caller limit");
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
