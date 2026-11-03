#include "pal4/sound3d_task_slot_resolver.h"

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
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 7> expected{{
        {"PlaySound3DTask", 28u}, {"StopSound3DTask", 28u},
        {"RevoluteTask", 24u}, {"PositionTask", 20u},
        {"TrackTask", 32u}, {"MessageTask", 36u},
        {"SlowMotionTask", 36u}}};
    for (const auto& [name, offset] : expected) {
        std::uint32_t output = 0u;
        Check(pal4::Sound3DTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0x5000u + offset,
              "task maps to sound facet offset");
        output = 0xffffffffu;
        Check(!pal4::Sound3DTaskSlotResolver::Resolve(
                  0u, name, output) && output == 0u,
              "null owner clears known task output");
    }
    for (const auto name : {"playsound3DTask", "UnknownTask", "ScaleTask"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::Sound3DTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0u,
              "unknown and case-mismatched task clears output");
    }
    std::uint32_t output = 0xffffffffu;
    Check(!pal4::Sound3DTaskSlotResolver::Resolve(
              0xffffffe4u, "PlaySound3DTask", output) && output == 0u,
          "x86 address wrap yields null facet");
}
