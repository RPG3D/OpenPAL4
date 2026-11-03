#include "pal4/camera_subsystem_lifecycle.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraSubsystemRuntime,
                      public pal4::ICameraControllerRuntime {
public:
    std::vector<std::string> calls;
    bool fail_allocation = false;
    pal4::CameraControllerState controller_state;

    std::uint32_t SingletonManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t Register(std::uint32_t manager, const char* name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t cleanup) override {
        Check(manager == 0x1000u && std::string(name) == "palCameraManager" &&
                  id == 1002u && factory == 0x5eae20u &&
                  cleanup == 0x5eae80u,
              "camera singleton registration operands");
        calls.emplace_back("register");
        return 0x2000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0xc4u, "camera controller allocation size");
        calls.emplace_back("allocate");
        return fail_allocation ? 0u : 0x3000u;
    }
    pal4::CameraControllerState& ControllerAt(std::uint32_t memory) override {
        Check(memory == 0x3000u, "camera controller constructor receiver");
        calls.emplace_back("resolve controller");
        return controller_state;
    }
    void SetName(std::uint32_t& pointer, const char* name) override {
        Check(pointer == 0u && std::string(name) ==
                  "palCameraController set", "controller name initializer");
        pointer = 0x5000u;
        calls.emplace_back("set name");
    }
    void DestroyNameObject(pal4::CameraControllerState& state,
                           std::uint32_t offset) override {
        Check(offset == 0x10u && state.At(0x30u) == 0u &&
                  state.At(0xbcu) == 0u,
              "controller nested cleanup observes reset state");
        calls.emplace_back("destroy name");
    }
    std::uint32_t FreeData(std::uint32_t data) override {
        Check(data == 0x6000u, "controller data release receiver");
        calls.emplace_back("free data");
        return 0x99u;
    }
    void CleanupCameraObject(std::uint32_t camera_object) override {
        Check(camera_object == 0x5000u,
              "camera object destructor receiver");
        calls.emplace_back("cleanup camera");
    }
    std::uint32_t Free(std::uint32_t memory) override {
        Check(memory == 0x3000u || memory == 0x5000u,
              "camera free receiver");
        calls.emplace_back("free");
        return 0x88u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraController controller(runtime);
    pal4::CameraSubsystemLifecycle lifecycle(runtime, controller);
    pal4::CameraSubsystemRegistrationState state{0x80u, 0x7777u};
    Check(lifecycle.Register(state) == 0x2000u &&
              state.guard == 0x81u && state.instance == 0x2000u &&
              runtime.calls == std::vector<std::string>{
                  "manager", "register"},
          "first registration sets guard and caches singleton");
    runtime.calls.clear();
    Check(lifecycle.Register(state) == 0x2000u &&
              state.guard == 0x81u && runtime.calls.empty(),
          "repeated registration returns cached singleton");

    Check(lifecycle.Create() == 0x3000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate", "resolve controller", "set name"} &&
              runtime.controller_state.At(0x10u) == 0x5eaec0u,
          "controller factory allocates then constructs");
    runtime.calls.clear();
    runtime.fail_allocation = true;
    Check(lifecycle.Create() == 0u &&
              runtime.calls == std::vector<std::string>{"allocate"},
          "factory allocation failure skips constructor");

    runtime.calls.clear();
    Check(!lifecycle.Destroy(0u).has_value() && runtime.calls.empty(),
          "empty controller skips cleanup and has unspecified native return");
    runtime.controller_state.At(0x4u) = 0x6000u;
    Check(lifecycle.Destroy(0x3000u) == 0x88u &&
              runtime.calls == std::vector<std::string>{
                  "resolve controller", "destroy name", "free data", "free"},
          "controller cleanup precedes free and forwards return");
    runtime.calls.clear();
    Check(!lifecycle.DestroyCameraObject(0u).has_value() &&
              runtime.calls.empty(),
          "empty camera object skips destructor and free");
    Check(lifecycle.DestroyCameraObject(0x5000u) == 0x88u &&
              runtime.calls == std::vector<std::string>{
                  "cleanup camera", "free"},
          "camera object destructor precedes outer free");
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
