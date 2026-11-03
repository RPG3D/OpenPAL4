#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorManagerActionLifecycleRuntime {
public:
    virtual ~IActorManagerActionLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void HandleEvent14(std::uint32_t actor,std::uint32_t selector,std::uint32_t flag)=0;
    virtual std::uint32_t Dispatch(std::string_view event,std::uint32_t actor,std::uint32_t mode)=0;
    virtual void InvokeActorSlot396(std::uint32_t actor,std::uint32_t argument)=0;
    virtual std::uint32_t InvokePropertySlot(std::uint32_t property,std::uint32_t slot,
        std::uint32_t actor,std::uint32_t argument)=0;
};
class IActorManagerActionEventRuntime : public IActorManagerActionLifecycleRuntime {
public:
    [[nodiscard]] virtual std::int32_t ReadMetric(
        std::uint32_t actor, std::uint32_t target,
        std::uint32_t virtual_slot) = 0;
    [[nodiscard]] virtual std::int64_t FloatToInteger(float value) = 0;
    [[nodiscard]] virtual std::uint32_t AddMetric8(
        std::uint32_t actor, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot144(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot148(
        std::uint32_t actor, std::uint32_t argument) = 0;
};

class ActorManagerActionEvents final {
public:
    [[nodiscard]] static std::uint32_t TransferMetric8(
        std::uint32_t actor, std::uint32_t target, float factor,
        IActorManagerActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t StartAction(
        std::uint32_t actor, IActorManagerActionLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t EndAction(
        std::uint32_t actor, IActorManagerActionLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t ResetForTarget(
        std::uint32_t actor, std::uint32_t target,
        IActorManagerActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot36(
        std::uint32_t actor, std::uint32_t target,
        IActorManagerActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot40(
        std::uint32_t actor, std::uint32_t target,
        IActorManagerActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot44(
        std::uint32_t actor, std::uint32_t target,
        IActorManagerActionEventRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot48(
        std::uint32_t actor, std::uint32_t target,
        IActorManagerActionEventRuntime& runtime);
};

}  // namespace pal4
