#pragma once

#include "pal4/actor_manager_owned_slot.h"

#include <cstdint>

namespace pal4 {

class IActorManagerPlayerStatRuntime : public IActorManagerOwnedSlotRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructDamage(
        std::uint32_t storage) = 0;
    [[nodiscard]] virtual std::uint32_t TemporarySlotAddress() = 0;
    [[nodiscard]] virtual std::int32_t ActorSlot52(
        std::uint32_t actor, std::uint32_t mode) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source, std::uint32_t count) = 0;
    virtual void ProcessInventoryItemSlots(
        std::uint32_t slots, std::uint32_t destination,
        std::uint32_t source) = 0;
    virtual void ProcessSkills(std::uint32_t actor,
                               std::uint32_t mode,
                               std::uint32_t context) = 0;
    [[nodiscard]] virtual std::int32_t DataElement(
        std::uint32_t data, std::uint32_t index) = 0;
    virtual void SetValidatedProperty(std::uint32_t data,
                                      std::uint32_t index,
                                      std::uint32_t value) = 0;
    [[nodiscard]] virtual std::int32_t GetProperty(
        std::uint32_t data, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::int32_t UpdateHealthStatus(
        std::uint32_t actor) = 0;
};

class ActorManagerPlayerStatRefresh final {
public:
    [[nodiscard]] static std::int32_t Rebuild(
        std::uint32_t actor, IActorManagerPlayerStatRuntime& runtime);
};

}  // namespace pal4
