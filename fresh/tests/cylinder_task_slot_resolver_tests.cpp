#include "pal4/cylinder_task_slot_resolver.h"

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
        Expected{"ShowobjTask", 20u},
        Expected{"HideobjTask", 20u},
        Expected{"ScaleTask", 24u},
        Expected{"RotateTask", 28u},
        Expected{"PositionTask", 32u},
        Expected{"RevoluteTask", 36u},
        Expected{"TrackTask", 40u},
        Expected{"ChangeColorTask", 44u},
        Expected{"RumpShahdowTask", 48u},
        Expected{"MoveToTargetTask", 52u},
        Expected{"MessageTask", 56u},
        Expected{"SlowMotionTask", 56u},
    };
    for (const auto& entry : expected) {
        std::uint32_t output = 0xffffffffu;
        Check(pal4::CylinderTaskSlotResolver::Resolve(
                  0x1000u, entry.name, output) &&
                  output == 0x1000u + entry.offset,
              "task maps to native cylinder facet offset");
        output = 0xffffffffu;
        Check(!pal4::CylinderTaskSlotResolver::Resolve(
                  0u, entry.name, output) && output == 0u,
              "known task on null object clears output");
    }
    for (const auto name : {"", "ShowObjTask", "RumpShadowTask",
                            "UnknownTask", "MessageTaskX"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::CylinderTaskSlotResolver::Resolve(
                  0x1000u, name, output) && output == 0u,
              "unknown or changed spelling clears output");
    }
    std::uint32_t output = 0u;
    Check(pal4::CylinderTaskSlotResolver::Resolve(
              0xfffffff0u, "PositionTask", output) && output == 0x10u,
          "facet address wraps with x86 arithmetic");
    output = 0xffffffffu;
    Check(!pal4::CylinderTaskSlotResolver::Resolve(
              0xffffffe0u, "PositionTask", output) && output == 0u,
          "wrapped null facet returns false");
}
