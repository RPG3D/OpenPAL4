#include "pal4/effect_task_slot_resolver.h"

#include <array>
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

void TestNativeTaskNames() {
    struct Expected {
        std::string_view name;
        std::uint32_t offset;
    };
    constexpr std::array expected{
        Expected{"ShowobjTask", 0x14u},
        Expected{"HideobjTask", 0x14u},
        Expected{"PositionTask", 0x18u},
        Expected{"RevoluteTask", 0x1cu},
        Expected{"TrackTask", 0x20u},
        Expected{"ChangeColorTask", 0x24u},
        Expected{"ScaleTask", 0x28u},
        Expected{"RotateTask", 0x2cu},
        Expected{"RumpShahdowTask", 0x30u},
        Expected{"MessageTask", 0x34u},
        Expected{"SlowMotionTask", 0x34u},
    };
    for (const auto& entry : expected) {
        std::uint32_t output = 0xffffffffu;
        Check(pal4::EffectTaskSlotResolver::Resolve(
                  0x1000u, entry.name, output) &&
                  output == 0x1000u + entry.offset,
              "known task resolves to exact native slot");
        output = 0xffffffffu;
        Check(!pal4::EffectTaskSlotResolver::Resolve(
                  0u, entry.name, output) && output == 0u,
              "known task with null object clears output");
    }
}

void TestUnknownAndCase() {
    for (const auto name : {"", "showobjTask", "ShowobjTaskX",
                            "RumpShadowTask", "UnknownTask"}) {
        std::uint32_t output = 0xffffffffu;
        Check(!pal4::EffectTaskSlotResolver::Resolve(
                  0x1000u, name, output) && output == 0u,
              "unknown or misspelled task clears output");
    }
    std::uint32_t output = 0u;
    Check(pal4::EffectTaskSlotResolver::Resolve(
              0xfffffff0u, "PositionTask", output) && output == 8u,
          "native address arithmetic wraps at 32 bits");
}

}  // namespace

int main() {
    TestNativeTaskNames();
    TestUnknownAndCase();
}
