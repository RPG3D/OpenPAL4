#include "pal4/camera_controller.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraControllerRuntime {
public:
    unsigned calls = 0u;
    unsigned destroy_calls = 0u;

    void SetName(std::uint32_t& pointer, const char* name) override {
        ++calls;
        Check(std::string(name) == "palCameraController set" &&
                  pointer == 0u,
              "camera controller name and empty string slot");
        pointer = 0x5000u;
    }
    void DestroyNameObject(pal4::CameraControllerState& state,
                           std::uint32_t offset) override {
        ++destroy_calls;
        Check(offset == 0x10u && state.At(0x30u) == 0u &&
                  state.At(0xbcu) == 0u && state.At(0x4u) == 0x6000u,
              "nested destructor follows status reset before data release");
    }
    std::uint32_t FreeData(std::uint32_t data) override {
        Check(data == 0x6000u && destroy_calls == 1u,
              "data release follows nested destructor");
        return 0x99u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraController controller(runtime);
    pal4::CameraControllerState state;
    state.words.fill(0xdeadbeefu);
    Check(&controller.Construct(state) == &state && runtime.calls == 1u,
          "controller constructor returns receiver and sets name once");
    for (const auto offset : {0x4u, 0x8u, 0xcu, 0x20u, 0x24u,
                              0x30u, 0x34u, 0x38u, 0x3cu, 0x40u, 0xbcu})
        Check(state.At(offset) == 0u,
              "controller initial state zero fields");
    Check(state.At(0x10u) == 0x5eaec0u &&
              state.At(0x14u) == 0x5eaea0u &&
              state.At(0x18u) == 0x5eaf50u &&
              state.At(0x1cu) == 0x5fc6c0u &&
              state.At(0x2cu) == 0x5000u &&
              state.At(0x28u) == 0xde010100u,
          "nested object callback slots, name and byte flags");
    Check(state.At(0u) == 0xdeadbeefu &&
              state.At(0x44u) == 0xdeadbeefu &&
              state.At(0xc0u) == 0xdeadbeefu,
          "unobserved controller fields remain untouched");

    state.At(0x4u) = 0x6000u;
    state.At(0x8u) = 0x6010u;
    state.At(0xcu) = 0x6020u;
    Check(pal4::CameraController::ClearDynamicCollection(state) == 0x6010u &&
              state.At(0x4u) == 0x6000u && state.At(0x8u) == 0x6000u &&
              state.At(0xcu) == 0x6020u,
          "collection clear returns old end and preserves capacity");

    state.At(0x4u) = 0x6000u;
    state.At(0x8u) = 0x7000u;
    state.At(0xcu) = 0x8000u;
    state.At(0x30u) = 1u;
    state.At(0xbcu) = 2u;
    Check(controller.DestroyComponent(state) == 0x99u &&
              runtime.destroy_calls == 1u &&
              state.At(0x4u) == 0u && state.At(0x8u) == 0u &&
              state.At(0xcu) == 0u && state.At(0x30u) == 0u &&
              state.At(0xbcu) == 0u && state.At(0x44u) == 0xdeadbeefu,
          "component cleanup resets slots after nested and data release");
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
