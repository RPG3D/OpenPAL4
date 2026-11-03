#pragma once

#include "pal4/actor_manager_owned_slot.h"

#include <cstdint>

namespace pal4 {

class ActorManagerPlayerChildOwnership final {
public:
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t actor, std::uint32_t child,
        IActorManagerOwnedSlotRuntime& runtime);
};

}  // namespace pal4
