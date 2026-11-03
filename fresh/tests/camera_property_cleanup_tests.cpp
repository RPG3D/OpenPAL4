#include "pal4/camera_property_cleanup.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraPropertyCleanupRuntime {
public:
    std::uint32_t empty_result = 1u;
    std::vector<std::string> calls;

    std::uint32_t IsEmpty(pal4::CameraControllerState& state,
                          std::uint32_t offset) override {
        Check(offset == 0x10u && state.At(0x30u) == 0xdeadbeefu,
              "collection check precedes active-camera update");
        calls.emplace_back("empty");
        return empty_result;
    }
    void ClearCollection(pal4::CameraControllerState& state,
                         std::uint32_t offset) override {
        Check(offset == 0x10u && state.At(0x30u) == 0xdeadbeefu,
              "collection clear precedes active-camera update");
        calls.emplace_back("clear");
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraPropertyCleanup cleanup(runtime);
    pal4::CameraControllerState state;
    state.At(0x30u) = 0xdeadbeefu;
    state.At(0x34u) = 0x12345678u;

    Check(cleanup.Cleanup(state) == 1u &&
              state.At(0x30u) == 0x12345678u &&
              runtime.calls == std::vector<std::string>{"empty"},
          "empty collection skips clear and returns check result");

    runtime.calls.clear();
    runtime.empty_result = 0x100u;
    state.At(0x30u) = 0xdeadbeefu;
    Check(cleanup.Cleanup(state) == 0x12345678u &&
              state.At(0x30u) == 0x12345678u &&
              runtime.calls == std::vector<std::string>{"empty", "clear"},
          "low byte controls nonempty branch and returns first camera");

    runtime.calls.clear();
    runtime.empty_result = 0x101u;
    state.At(0x30u) = 0xdeadbeefu;
    state.At(0x34u) = 0u;
    Check(cleanup.Cleanup(state) == 0x101u &&
              state.At(0x30u) == 0u &&
              runtime.calls == std::vector<std::string>{"empty"},
          "empty check preserves complete result and copies null slot");
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
