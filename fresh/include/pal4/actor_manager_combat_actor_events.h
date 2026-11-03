#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerCombatResourceRuntime {
public:
    virtual ~IActorManagerCombatResourceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void InvokeActorSlot592(std::uint32_t actor,
                                    std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot444(
        std::uint32_t actor, std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeChildSlot4(
        std::uint32_t child, std::uint32_t argument) = 0;
};

class IActorManagerCombatJoinRuntime {
public:
    virtual ~IActorManagerCombatJoinRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot48(
        std::uint32_t actor, std::uint32_t index) = 0;
    virtual void InvokeActorSlot68(std::uint32_t actor,
                                   std::uint32_t index,
                                   std::uint32_t value) = 0;
    virtual void InvokeActorSlot580(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchActor(
        std::string_view event, std::uint32_t actor,
        std::uint32_t mode) = 0;
};

class IActorManagerCombatSummonRuntime {
public:
    virtual ~IActorManagerCombatSummonRuntime() = default;
    virtual void DispatchPair(std::string_view event,std::uint32_t first,
                              std::uint32_t second,std::uint32_t mode) = 0;
};
class IActorManagerCombatEventRuntime : public IActorManagerCombatResourceRuntime,
                                       public IActorManagerCombatJoinRuntime,
                                       public IActorManagerCombatSummonRuntime {};

class ActorManagerCombatActorEvents final {
public:
    static constexpr std::uint32_t kEmptyText = 0x841964u;

    [[nodiscard]] static std::uint32_t RefreshResource(
        std::uint32_t actor, IActorManagerCombatResourceRuntime& runtime);
    static void Summon(std::uint32_t first, std::uint32_t second,
                       IActorManagerCombatSummonRuntime& runtime);
    [[nodiscard]] static std::uint32_t JoinStageAndScene(
        std::uint32_t actor, std::uint32_t unused,
        IActorManagerCombatJoinRuntime& runtime);
    [[nodiscard]] static std::uint32_t LeaveStageAndScene(
        std::uint32_t actor, IActorManagerCombatJoinRuntime& runtime);
};

}  // namespace pal4
