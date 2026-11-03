#include "pal4/actor_control_input_snapshot.h"

#include "pal4/input_manager.h"

namespace pal4 {
namespace {

ActorControlMouseInput MouseGroup(const InputManager& input,
                                  std::uint32_t primary,
                                  std::uint32_t secondary) {
    return {
        static_cast<std::int32_t>(input.ReadDword(primary + 12u)),
        input.ReadDword(primary),
        input.ReadDword(primary + 8u),
        static_cast<std::int32_t>(input.ReadDword(secondary + 12u)),
        input.ReadDword(secondary),
        input.ReadDword(secondary + 8u),
    };
}

}  // namespace

ActorControlMouseInput ActorControlInputSnapshot::Yaw(
    const InputManager& input) {
    return MouseGroup(input, 0x1978u, 0x1998u);
}

ActorControlMouseInput ActorControlInputSnapshot::Drag(
    const InputManager& input) {
    return MouseGroup(input, 0x1ed8u, 0x1ee8u);
}

ActorInteractionInput ActorControlInputSnapshot::Interaction(
    const InputManager& input) {
    return {
        static_cast<std::int32_t>(input.ReadDword(0x18b4u)),
        input.ReadDword(0x18a8u),
        static_cast<std::int32_t>(input.ReadDword(0x21d4u)),
        input.ReadDword(0x21c8u),
        input.ReadDword(0x21d0u),
        static_cast<std::int32_t>(input.ReadDword(0x18c4u)),
        input.ReadDword(0x18b8u),
        input.ReadDword(0x18c0u),
    };
}

}  // namespace pal4
