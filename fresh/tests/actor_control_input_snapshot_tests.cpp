#include "pal4/actor_control_input_snapshot.h"
#include "pal4/input_manager.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void WriteGroup(pal4::InputManager& input, std::uint32_t offset,
                std::uint32_t mode, std::uint32_t release,
                std::uint32_t count) {
    input.WriteDword(offset, mode);
    input.WriteDword(offset + 8u, release);
    input.WriteDword(offset + 12u, count);
}

}  // namespace

int main() {
    pal4::InputManager input;
    WriteGroup(input, 0x1978u, 0x11u, 2u, 3u);
    WriteGroup(input, 0x1998u, 0x22u, 4u, 5u);
    WriteGroup(input, 0x1ed8u, 0x33u, 6u, 7u);
    WriteGroup(input, 0x1ee8u, 0x44u, 8u, 9u);
    WriteGroup(input, 0x18a8u, 0x55u, 10u, 11u);
    WriteGroup(input, 0x18b8u, 0x66u, 12u, 13u);
    WriteGroup(input, 0x21c8u, 0x77u, 14u, 15u);

    const auto yaw = pal4::ActorControlInputSnapshot::Yaw(input);
    const auto drag = pal4::ActorControlInputSnapshot::Drag(input);
    const auto interaction =
        pal4::ActorControlInputSnapshot::Interaction(input);
    Check(yaw.primary_mode == 0x11u && yaw.primary_release == 2u &&
              yaw.primary_count == 3 && yaw.secondary_mode == 0x22u &&
              yaw.secondary_release == 4u && yaw.secondary_count == 5,
          "camera yaw uses its own input records");
    Check(drag.primary_mode == 0x33u && drag.primary_release == 6u &&
              drag.primary_count == 7 && drag.secondary_mode == 0x44u &&
              drag.secondary_release == 8u && drag.secondary_count == 9,
          "camera drag uses its own input records");
    Check(interaction.blocking_mode == 0x55u &&
              interaction.blocking_count == 11 &&
              interaction.secondary_mode == 0x66u &&
              interaction.secondary_release == 12u &&
              interaction.secondary_count == 13 &&
              interaction.primary_mode == 0x77u &&
              interaction.primary_release == 14u &&
              interaction.primary_count == 15,
          "interaction reads its blocking and click records");
}
