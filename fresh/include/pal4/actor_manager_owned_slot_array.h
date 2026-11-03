#pragma once

#include "pal4/actor_manager_owned_slot.h"

#include <cstdint>

namespace pal4 {

class IActorManagerOwnedSlotArrayRuntime
    : public IActorManagerOwnedSlotRuntime {
public:
    virtual ~IActorManagerOwnedSlotArrayRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
};

class ActorManagerOwnedSlotArray final {
public:
    [[nodiscard]] static std::uint32_t InsertCopies(
        std::uint32_t array, std::uint32_t position, std::uint32_t count,
        std::uint32_t source, IActorManagerOwnedSlotArrayRuntime& runtime);
};

}  // namespace pal4
