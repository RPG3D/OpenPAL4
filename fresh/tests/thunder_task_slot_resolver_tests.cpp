#include "pal4/thunder_task_slot_resolver.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 10u> cases{{
        {"ShowobjTask", 40u}, {"HideobjTask", 40u},
        {"ScaleTask", 20u}, {"RotateTask", 24u},
        {"PositionTask", 28u}, {"RevoluteTask", 32u},
        {"TrackTask", 36u}, {"ChangeColorTask", 44u},
        {"MessageTask", 52u}, {"SlowMotionTask", 52u}}};
    for (const auto& [name, offset] : cases) {
        std::uint32_t output = 0u;
        Check(pal4::ThunderTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0x5000u + offset,
              "task maps to IDA ThunderObject facet");
        output = 0xffffffffu;
        Check(!pal4::ThunderTaskSlotResolver::Resolve(
                  0u, name, output) && output == 0u,
              "null object clears known task output");
        output = 0xffffffffu;
        Check(!pal4::ThunderTaskSlotResolver::Resolve(
                  0u - offset, name, output) && output == 0u,
              "x86 address wrap yields null facet");
    }
    for (const auto name : {"showobjTask", "RumpShahdowTask",
                            "UnknownTask"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::ThunderTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0u,
              "unknown task clears output");
    }
}
