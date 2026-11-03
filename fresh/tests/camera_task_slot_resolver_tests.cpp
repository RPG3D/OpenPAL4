#include "pal4/camera_task_slot_resolver.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    struct Expected final {
        std::string_view name;
        std::uint32_t offset;
    };
    constexpr std::array expected{
        Expected{"StartCameraTask", 24u},
        Expected{"StopCameraTask", 24u},
        Expected{"TrackTask", 20u},
        Expected{"RevoluteTask", 28u},
        Expected{"PositionTask", 32u},
        Expected{"WobbleTask", 32u},
        Expected{"RotateTask", 36u},
        Expected{"MessageTask", 40u},
        Expected{"SlowMotionTask", 40u},
    };
    for (const auto& entry : expected) {
        std::uint32_t output = 0xffffffffu;
        Check(pal4::CameraTaskSlotResolver::Resolve(
                  0x1000u, entry.name, output) &&
                  output == 0x1000u + entry.offset,
              "camera task resolves to its native slot");
        output = 0xffffffffu;
        Check(!pal4::CameraTaskSlotResolver::Resolve(
                  0u, entry.name, output) && output == 0u,
              "known task on null object clears output");
    }
    for (const auto name : {"", "startCameraTask", "WobbleTaskX",
                            "ChangeColorTask", "UnknownTask"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::CameraTaskSlotResolver::Resolve(
                  0x1000u, name, output) && output == 0u,
              "unknown or differently cased task clears output");
    }
    std::uint32_t output = 0u;
    Check(pal4::CameraTaskSlotResolver::Resolve(
              0xfffffff0u, "PositionTask", output) && output == 0x10u,
          "native slot arithmetic wraps at 32 bits");
    output = 0xffffffffu;
    Check(!pal4::CameraTaskSlotResolver::Resolve(
              0xffffffe8u, "StartCameraTask", output) && output == 0u,
          "wrapped zero slot returns false");
}
