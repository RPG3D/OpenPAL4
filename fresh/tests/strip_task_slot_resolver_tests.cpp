#include "pal4/strip_task_slot_resolver.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestNames() {
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 12> expected{{
        {"ShowobjTask", 20u}, {"HideobjTask", 20u},
        {"ScaleTask", 24u}, {"RotateTask", 28u},
        {"PositionTask", 32u}, {"RevoluteTask", 36u},
        {"TrackTask", 40u}, {"ChangeColorTask", 44u},
        {"RumpShahdowTask", 48u}, {"MoveToTargetTask", 52u},
        {"MessageTask", 56u}, {"SlowMotionTask", 56u}}};
    for (const auto& [name, offset] : expected) {
        std::uint32_t output = 0u;
        Check(pal4::StripTaskSlotResolver::Resolve(0x5000u, name, output) &&
                  output == 0x5000u + offset,
              "task maps to native strip facet offset");
        output = 0xffffffffu;
        Check(!pal4::StripTaskSlotResolver::Resolve(0u, name, output) &&
                  output == 0u,
              "null owner clears known task output");
    }
}

void TestMisses() {
    for (const auto name : {"showobjTask", "RumpShadowTask", "UnknownTask"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::StripTaskSlotResolver::Resolve(0x5000u, name, output) &&
                  output == 0u,
              "unknown and corrected spellings clear output");
    }
    std::uint32_t output = 0xffffffffu;
    Check(!pal4::StripTaskSlotResolver::Resolve(
              0xffffffecu, "ShowobjTask", output) && output == 0u,
          "x86 address wrap yields null facet");
}

}  // namespace

int main() {
    TestNames();
    TestMisses();
}
