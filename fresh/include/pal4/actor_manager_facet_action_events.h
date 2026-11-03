#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerFacetProgressRuntime {
public:
    virtual ~IActorManagerFacetProgressRuntime() = default;
    [[nodiscard]] virtual std::int32_t ActorStateSlot20(std::uint32_t actor) = 0;
    virtual void SetProperty2(std::uint32_t facet, std::int32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Dispatch(
        std::string_view event, std::uint32_t actor, std::uint32_t mode) = 0;
};
class IActorManagerFacetActionEventRuntime : public IActorManagerFacetProgressRuntime {
public:
    ~IActorManagerFacetActionEventRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void HandleStateEvent(std::uint32_t facet) = 0;
    [[nodiscard]] virtual std::uint8_t CheckState3(
        std::uint32_t actor) = 0;
    virtual void ProcessActorEvents(std::uint32_t actor) = 0;
};

class IActorManagerPlayerFacetActionEventRuntime
    : public IActorManagerFacetActionEventRuntime {
public:
    [[nodiscard]] virtual bool CombatSystemInactive() = 0;
    [[nodiscard]] virtual std::uint32_t ActorSlot532(
        std::uint32_t actor) = 0;
};

class ActorManagerFacetActionEvents final {
public:
    static constexpr std::uint32_t kActorOffset = 716u;
    static constexpr std::uint32_t kPendingOffset = 448u;

    [[nodiscard]] static std::uint8_t CanTakeAction(
        std::uint32_t facet,
        IActorManagerFacetActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t MovedAtActionSequence(
        std::uint32_t facet, std::int32_t value,
        IActorManagerFacetProgressRuntime& runtime);
};

class ActorManagerPlayerFacetActionEvents final {
public:
    static constexpr std::uint32_t kActorOffset = 716u;
    static constexpr std::uint32_t kCombatThresholdOffset = 116u;

    [[nodiscard]] static std::uint8_t CanTakeAction(
        std::uint32_t facet,
        IActorManagerPlayerFacetActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t MovedAtActionSequence(
        std::uint32_t facet, std::int32_t value,
        IActorManagerFacetProgressRuntime& runtime);
};

}  // namespace pal4
