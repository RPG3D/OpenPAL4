#include "pal4/base_object_scene_parameter.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::IBaseObjectSceneParameterRuntime {
public:
    std::uint32_t requested_clump{};
    std::array<float, 3> position{12.0f, 34.0f, -56.0f};

    std::array<float, 3> ClumpPosition(const std::uint32_t clump) override {
        requested_clump = clump;
        return position;
    }
};

void RefreshFromClump() {
    pal4::BaseObjectState actor;
    actor.Write(0x14u, 0x12340000u);
    actor.Write(0xecu, std::bit_cast<std::uint32_t>(10.0f));
    actor.Write(0x1ecu, 0x8433bcu);
    actor.Write(0x1f4u, std::bit_cast<std::uint32_t>(99.0f));
    Runtime runtime;
    const auto target = pal4::BaseObjectSceneParameter::Refresh(actor, runtime);
    Check(runtime.requested_clump == 0x12340000u &&
              target == std::array<float, 3>{12.0f, 52.0f, -56.0f} &&
              actor.Read(0x1ecu) == 0x8433bcu &&
              actor.Read(0x1f4u) == std::bit_cast<std::uint32_t>(12.0f) &&
              actor.Read(0x1f8u) == std::bit_cast<std::uint32_t>(52.0f) &&
              actor.Read(0x1fcu) == std::bit_cast<std::uint32_t>(-56.0f),
          "scene parameter refresh uses Clump LTM and actor height");
}

void M05NegativeHeight() {
    pal4::BaseObjectState actor;
    actor.Write(0x14u, 0x12340000u);
    actor.Write(0xecu, 0x421cfe9du);
    Runtime runtime;
    runtime.position = {1101.8671875f, -315.669097900390625f, 565.03741455078125f};
    const auto target = pal4::BaseObjectSceneParameter::Refresh(actor, runtime);
    // Read from the original same-save actor+1F8, live175, FPCW007F.
    Check(std::bit_cast<std::uint32_t>(target[1]) == 0xc3750584u &&
              actor.Read(0x1f8u) == 0xc3750584u &&
              target[0] == runtime.position[0] && target[2] == runtime.position[2],
          "M05 scene parameter preserves original PC24 intermediate rounding");
}

}  // namespace

int main() {
    try {
        RefreshFromClump();
        M05NegativeHeight();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
