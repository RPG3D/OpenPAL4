#pragma once

#include "pal4/actor_animation_control.h"

#include <cstdint>

namespace pal4 {

class IActorManagerTurnStateRuntime {
public:
    virtual ~IActorManagerTurnStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetStateZero(std::uint32_t actor) = 0;
    virtual void CountScriptProperties(std::uint32_t script,
                                       std::uint32_t actor) = 0;
    virtual void InvokeActorSlot260(std::uint32_t actor,
                                    std::uint32_t selector,
                                    std::uint32_t argument) = 0;
    virtual void ProcessDynamicList(std::uint32_t actor,
                                    std::uint32_t mode) = 0;
    [[nodiscard]] virtual bool ObjectCheckState3(std::uint32_t actor) = 0;
    virtual void HandleEvent14(std::uint32_t actor,
                               std::uint32_t selector,
                               std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptCombatDead(
        std::uint32_t actor) = 0;
    virtual void RegisterActorEvent(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot152(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CombatActorDataPtr() = 0;
    [[nodiscard]] virtual std::uint32_t InvokeChildSlot(
        std::uint32_t child, std::uint32_t slot,
        std::uint32_t argument) = 0;
};

class ActorManagerCombatEvent final {
public:
    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t facet, IActorManagerTurnStateRuntime& runtime,
        IActorAnimationControlRuntime& animation_runtime);
    [[nodiscard]] static std::uint32_t ChildSlot56(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerTurnStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ChildSlot52(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerTurnStateRuntime& runtime);
};

}  // namespace pal4
