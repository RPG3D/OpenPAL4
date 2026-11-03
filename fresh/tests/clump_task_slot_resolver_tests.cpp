#include "pal4/clump_task_slot_resolver.h"

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
        Expected{"ScaleTask", 20u},
        Expected{"RotateTask", 24u},
        Expected{"PositionTask", 28u},
        Expected{"ShowobjTask", 40u},
        Expected{"HideobjTask", 40u},
        Expected{"UVRotateTask", 44u},
        Expected{"UVTranslateTask", 48u},
        Expected{"RevoluteTask", 32u},
        Expected{"ChangeColorTask", 36u},
        Expected{"TrackTask", 52u},
        Expected{"MoveToTargetTask", 56u},
        Expected{"RumpShahdowTask", 60u},
        Expected{"MessageTask", 64u},
        Expected{"SlowMotionTask", 64u},
    };
    for (const auto& entry : expected) {
        std::uint32_t output = 0xffffffffu;
        Check(pal4::ClumpTaskSlotResolver::Resolve(
                  0x1000u, entry.name, output) &&
                  output == 0x1000u + entry.offset,
              "task resolves to its native facet offset");
        output = 0xffffffffu;
        Check(!pal4::ClumpTaskSlotResolver::Resolve(
                  0u, entry.name, output) && output == 0u,
              "known task on null object clears output");
    }
    for (const auto name : {"", "ShowObjTask", "RumpShadowTask",
                            "UnknownTask", "MessageTaskX"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::ClumpTaskSlotResolver::Resolve(
                  0x1000u, name, output) && output == 0u,
              "unknown or differently spelled task clears output");
    }
    std::uint32_t output = 0u;
    Check(pal4::ClumpTaskSlotResolver::Resolve(
              0xfffffff0u, "PositionTask", output) && output == 0xcu,
          "facet pointer uses 32-bit arithmetic");
    output = 0xffffffffu;
    Check(!pal4::ClumpTaskSlotResolver::Resolve(
              0xffffffd8u, "ShowobjTask", output) && output == 0u,
          "wrapped null facet returns false");
}
