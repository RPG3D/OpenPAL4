#include "pal4/effect_object_task_slot_resolver.h"

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

void TestTaskNames() {
    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 12> expected{{
        {"ScaleTask", 20u}, {"RotateTask", 24u},
        {"PositionTask", 28u}, {"RevoluteTask", 32u},
        {"PlayEffectTask", 36u}, {"PauseEffectTask", 36u},
        {"StopEffectTask", 36u}, {"RewindEffectTask", 36u},
        {"TrackTask", 40u}, {"MessageTask", 44u},
        {"SlowMotionTask", 44u}, {"MoveToTargetTask", 48u}}};
    for (const auto& [name, offset] : expected) {
        std::uint32_t output = 0u;
        Check(pal4::EffectObjectTaskSlotResolver::Resolve(
                  0x5000u, name, output) && output == 0x5000u + offset,
              "task maps to exact EffectObject facet offset");
        output = 0xffffffffu;
        Check(!pal4::EffectObjectTaskSlotResolver::Resolve(
                  0u, name, output) && output == 0u,
              "known task with null owner clears output");
    }
}

void TestMissesAndWrap() {
    std::uint32_t output = 0xffffffffu;
    Check(!pal4::EffectObjectTaskSlotResolver::Resolve(
              0x5000u, "playeffecttask", output) && output == 0u,
          "task spelling is case sensitive");
    output = 0xffffffffu;
    Check(!pal4::EffectObjectTaskSlotResolver::Resolve(
              0x5000u, "UnknownTask", output) && output == 0u,
          "unknown task clears output");
    output = 0xffffffffu;
    Check(!pal4::EffectObjectTaskSlotResolver::Resolve(
              0xffffffeCu, "ScaleTask", output) && output == 0u,
          "x86 pointer wrap produces null facet");
}

}  // namespace

int main() {
    TestTaskNames();
    TestMissesAndWrap();
}
