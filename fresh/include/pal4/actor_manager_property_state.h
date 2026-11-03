#pragma once

#include <cstdint>

namespace pal4 {

struct ActorPropertyHandle {
    bool owned = false;
    std::uint32_t property = 0u;
};

class IActorManagerPropertyStateRuntime {
public:
    virtual ~IActorManagerPropertyStateRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void NotifyPropertyChange(std::uint32_t actor,
                                      ActorPropertyHandle previous,
                                      ActorPropertyHandle incoming) = 0;
    virtual void RefreshPropertyCount(std::uint32_t property,
                                      std::uint32_t actor) = 0;
    virtual void BindProperty(std::uint32_t property,
                              std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint8_t ReleaseProperty(
        std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyId(
        std::uint32_t property) = 0;
    virtual void FinishProperty(std::uint32_t property,
                                std::uint32_t actor) = 0;
    virtual void DispatchProperty(std::uint32_t property,
                                  std::uint32_t actor,
                                  std::uint32_t event_id) = 0;
    [[nodiscard]] virtual bool HasBlockingObjectState(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t InvokePropertySlot(
        std::uint32_t property, std::uint32_t actor,
        std::uint32_t slot, std::uint32_t argument) = 0;
    virtual void CleanupConditionalList(std::uint32_t actor,
                                        std::uint32_t id,
                                        std::uint32_t value) = 0;
    [[nodiscard]] virtual ActorPropertyHandle MakePropertyHandle(
        std::uint32_t first, std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ActorMode(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CombatActorData() = 0;
    virtual void SetClumpAlpha(float alpha, std::uint32_t actor_data) = 0;
    [[nodiscard]] virtual bool CheckReviveStatus(std::uint32_t actor) = 0;
    virtual void ClearReviveStatus(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot48(
        std::uint32_t actor, std::uint32_t argument) = 0;
    virtual void InvokeActorSlot68(std::uint32_t actor,
                                   std::uint32_t kind,
                                   std::uint32_t value) = 0;
    virtual void DispatchRevive(std::uint32_t actor) = 0;
    virtual void InvokeActorSlot272(std::uint32_t actor,
                                    std::uint32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot168(
        std::uint32_t actor) = 0;
};

class ActorManagerPropertyState final {
public:
    [[nodiscard]] static std::uint8_t Set(
        std::uint32_t actor, ActorPropertyHandle incoming,
        IActorManagerPropertyStateRuntime& runtime);
    static void HandleEvent(std::uint32_t actor, std::uint32_t event_id,
                            IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static ActorPropertyHandle MoveEmbedded(
        std::uint32_t property, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static bool IsIdle(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static bool IsReady(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static bool IsUnavailable(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot236(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot244(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction1(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot252(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction2(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot260(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction3(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot268(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction4(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot276(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction5(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot284(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction6(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot292(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction7(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot300(
        std::uint32_t actor, IActorManagerPropertyStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessPropertyAction8(
        std::uint32_t actor, std::uint32_t argument,
        IActorManagerPropertyStateRuntime& runtime);
};

}  // namespace pal4
