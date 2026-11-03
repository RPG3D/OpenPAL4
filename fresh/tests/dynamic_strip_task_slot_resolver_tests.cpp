#include "pal4/dynamic_strip_task_slot_resolver.h"

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
        Expected{"ShowobjTask", 40u},
        Expected{"HideobjTask", 40u},
        Expected{"ScaleTask", 20u},
        Expected{"RotateTask", 24u},
        Expected{"PositionTask", 28u},
        Expected{"RevoluteTask", 32u},
        Expected{"TrackTask", 36u},
        Expected{"ChangeColorTask", 44u},
        Expected{"MessageTask", 48u},
        Expected{"SlowMotionTask", 48u},
    };
    for (const auto& entry : expected) {
        std::uint32_t output = 0xffffffffu;
        Check(pal4::DynamicStripTaskSlotResolver::Resolve(
                  0x1000u, entry.name, output) &&
                  output == 0x1000u + entry.offset,
              "task maps to native dynamic strip facet offset");
        output = 0xffffffffu;
        Check(!pal4::DynamicStripTaskSlotResolver::Resolve(
                  0u, entry.name, output) && output == 0u,
              "known task on null object clears output");
    }
    for (const auto name : {"", "ShowObjTask", "RumpShadowTask",
                            "UnknownTask", "MessageTaskX"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::DynamicStripTaskSlotResolver::Resolve(
                  0x1000u, name, output) && output == 0u,
              "unknown or changed spelling clears output");
    }
    std::uint32_t output = 0u;
    Check(pal4::DynamicStripTaskSlotResolver::Resolve(
              0xfffffff0u, "PositionTask", output) && output == 0x0cu,
          "facet address wraps with x86 arithmetic");
    output = 0xffffffffu;
    Check(!pal4::DynamicStripTaskSlotResolver::Resolve(
              0xffffffe4u, "PositionTask", output) && output == 0u,
          "wrapped null facet returns false");
}
