#pragma once

#include "pal4/actor_manager_effect_setup.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerCombatSceneChangeRuntime
    : public IActorManagerEffectLoadRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot20(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot52(
        std::uint32_t actor, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FindDatabaseRecord(
        std::uint32_t key) = 0;
    virtual void DispatchEvent(std::string_view event,
                               std::uint32_t actor,
                               std::uint32_t mode) = 0;
    virtual void ActorSlotNoArgument(std::uint32_t actor,
                                     std::uint32_t slot) = 0;
    virtual void CopyFrom(std::uint32_t actor, std::uint32_t source) = 0;
    virtual void ActorSlot24(std::uint32_t actor,
                             std::uint32_t value) = 0;
    virtual void ActorSlot68(std::uint32_t actor,
                             std::uint32_t index,
                             std::uint32_t value) = 0;
    virtual void ResetEffect(std::uint32_t actor,
                             std::uint32_t handle_address) = 0;
};

class ActorManagerCombatSceneChange final {
public:
    static void ApplyPendingRecord(
        std::uint32_t actor, IActorManagerCombatSceneChangeRuntime& runtime);
};

}  // namespace pal4
