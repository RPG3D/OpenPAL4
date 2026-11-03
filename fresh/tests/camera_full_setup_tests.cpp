#include "pal4/camera_allocation.h"
#include "pal4/camera_full_setup.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraAllocationRuntime,
                      public pal4::ICameraFullSetupRuntime {
public:
    std::vector<std::string> calls;
    bool allocation_fails = false;
    bool device_succeeds = true;

    void DestroyDevice(std::uint32_t old_data) override {
        Check(old_data == 0x1000u, "old camera device receiver");
        calls.emplace_back("destroy device");
    }
    void DestroyData(std::uint32_t old_data) override {
        Check(old_data == 0x1000u, "old camera data receiver");
        calls.emplace_back("destroy data");
    }
    std::uint32_t Free(std::uint32_t old_data) override {
        Check(old_data == 0x1000u, "old camera free receiver");
        calls.emplace_back("free old");
        return 0u;
    }
    std::uint32_t AllocateData(std::uint32_t bytes) override {
        Check(bytes == 0x2ecu, "new camera allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? 0u : 0x2000u;
    }
    void ConstructData(std::uint32_t camera_data) override {
        Check(camera_data == 0x2000u, "new camera constructor receiver");
        calls.emplace_back("construct");
    }
    float FarClip() override {
        calls.emplace_back("far clip");
        return 100.0f;
    }
    float NearClip() override {
        calls.emplace_back("near clip");
        return 0.5f;
    }
    bool InitializeDevice(std::uint32_t camera_data,
                          std::int32_t width, std::int32_t height,
                          bool depth_raster, float near_clip,
                          float far_clip) override {
        Check(camera_data == 0x2000u && width == 640 && height == 480 &&
                  depth_raster && near_clip == 0.5f && far_clip == 100.0f,
              "device setup operands from viewport and game state");
        calls.emplace_back("device");
        return device_succeeds;
    }
    std::int32_t ViewMode() override {
        calls.emplace_back("view mode");
        return 7;
    }
    float ViewScale() override {
        calls.emplace_back("view scale");
        return 1.25f;
    }
    void AdjustViewWindow(std::uint32_t camera_data,
                          pal4::CameraRect& rectangle, float scale,
                          std::int32_t mode) override {
        Check(camera_data == 0x2000u &&
                  rectangle == pal4::CameraRect{1, 2, 640, 480} &&
                  scale == 1.25f && mode == 7,
              "viewport setup operands");
        rectangle[0] = 99;
        calls.emplace_back("viewport");
    }
    void SetParameters(std::uint32_t camera_data,
                       const std::array<float, 3>& target,
                       float angle_y, float angle_x, float angle_z,
                       float distance) override {
        Check(camera_data == 0x2000u &&
                  target == std::array<float, 3>{10.0f, 20.0f, 30.0f} &&
                  angle_y == 2.0f && angle_x == 1.0f &&
                  angle_z == 3.0f && distance == 8.0f,
              "target, angle and distance argument order");
        calls.emplace_back("parameters");
    }
    void InitializeName(pal4::StringBufferState& name,
                        const char* text, std::size_t length) override {
        Check(std::string(text) == "main" && length == 4u &&
                  name.data == 0x3000u,
              "name string and exact length");
        name.length = static_cast<std::uint32_t>(length);
        calls.emplace_back("name");
    }
};

pal4::CameraSetupParameters Parameters() {
    return {"main", {1, 2, 640, 480}, true,
            {10.0f, 20.0f, 30.0f}, 8.0f,
            1.0f, 2.0f, 3.0f};
}

void SuccessAndDeviceFailure() {
    Runtime runtime;
    pal4::CameraAllocation allocation(runtime);
    pal4::CameraFullSetup setup(allocation, runtime);
    pal4::CameraSystemObjectState owner;
    owner.camera = 0x1000u;
    owner.name = {0x3000u, 0u, 0x20u};
    const auto parameters = Parameters();
    Check(setup.Initialize(owner, parameters) &&
              owner.camera == 0x2000u && owner.name.length == 4u &&
              parameters.viewport == pal4::CameraRect{1, 2, 640, 480} &&
              runtime.calls == std::vector<std::string>{
                  "destroy device", "destroy data", "free old",
                  "allocate", "construct", "far clip", "near clip",
                  "device", "view mode", "view scale", "viewport",
                  "parameters", "name"},
          "full setup replaces old camera and applies native call order");

    Runtime failed_device;
    failed_device.device_succeeds = false;
    pal4::CameraAllocation second_allocation(failed_device);
    pal4::CameraFullSetup second_setup(second_allocation, failed_device);
    pal4::CameraSystemObjectState second_owner;
    second_owner.name.data = 0x3000u;
    Check(!second_setup.Initialize(second_owner, parameters) &&
              second_owner.camera == 0x2000u &&
              failed_device.calls.back() == "name",
          "device failure still applies viewport, parameters and name");
}

void AllocationFailure() {
    Runtime runtime;
    runtime.allocation_fails = true;
    pal4::CameraAllocation allocation(runtime);
    pal4::CameraFullSetup setup(allocation, runtime);
    pal4::CameraSystemObjectState owner;
    owner.camera = 0x1000u;
    Check(!setup.Initialize(owner, Parameters()) && owner.camera == 0u &&
              runtime.calls == std::vector<std::string>{
                  "destroy device", "destroy data", "free old", "allocate"},
          "failed allocation leaves no owner and skips invalid receiver calls");
}

}  // namespace

int main() {
    try {
        SuccessAndDeviceFailure();
        AllocationFailure();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
