#include "pal4/sequence_anim_task_slot_resolver.h"

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

}  // namespace

int main() {
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 10> expected{{
        {"ShowobjTask", 20u}, {"HideobjTask", 20u},
        {"PositionTask", 24u}, {"RevoluteTask", 28u},
        {"TrackTask", 32u}, {"ChangeColorTask", 36u},
        {"ScaleTask", 40u}, {"RotateTask", 44u},
        {"MessageTask", 48u}, {"SlowMotionTask", 48u}}};
    for (const auto& [name, offset] : expected) {
        std::uint32_t output = 0u;
        Check(pal4::SequenceAnimTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0x5000u + offset,
              "task maps to sequence facet offset");
        output = 0xffffffffu;
        Check(!pal4::SequenceAnimTaskSlotResolver::Resolve(
                  0u, name, output) && output == 0u,
              "null owner clears known task output");
    }
    for (const auto name : {"showobjTask", "UnknownTask", "MoveToTargetTask"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::SequenceAnimTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0u,
              "unknown and case-mismatched task clears output");
    }
    std::uint32_t output = 0xffffffffu;
    Check(!pal4::SequenceAnimTaskSlotResolver::Resolve(
              0xffffffe8u, "PositionTask", output) && output == 0u,
          "x86 address wrap yields null facet");
}
