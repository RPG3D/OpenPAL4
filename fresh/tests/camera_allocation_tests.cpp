#include "pal4/camera_allocation.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraAllocationRuntime {
public:
    std::vector<std::string> calls;

    void DestroyDevice(std::uint32_t allocation) override {
        Check(allocation == 0x1000u, "device receiver");
        calls.emplace_back("device");
    }
    void DestroyData(std::uint32_t allocation) override {
        Check(allocation == 0x1000u &&
                  calls == std::vector<std::string>{"device"},
              "data destructor follows device teardown");
        calls.emplace_back("data");
    }
    std::uint32_t Free(std::uint32_t allocation) override {
        Check(allocation == 0x1000u &&
                  calls == std::vector<std::string>{"device", "data"},
              "memory release follows both destructors");
        calls.emplace_back("free");
        return 0x88u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraAllocation allocation(runtime);
    std::uint32_t camera_data = 0u;
    Check(!allocation.Release(camera_data).has_value() &&
              camera_data == 0u && runtime.calls.empty(),
          "empty owner slot skips all camera services");

    camera_data = 0x1000u;
    Check(allocation.Release(camera_data) == 0x88u &&
              camera_data == 0u &&
              runtime.calls == std::vector<std::string>{
                  "device", "data", "free"},
          "release calls device, data and free before clearing owner slot");
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
