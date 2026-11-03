#pragma once

#include <cstdint>

namespace pal4 {

struct ActorManagerEffectHandle {
    std::uint32_t id;
    std::uint32_t owner;
};

class IActorManagerCombatPrepareRuntime {
public:
    virtual ~IActorManagerCombatPrepareRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    [[nodiscard]] virtual bool CombatInactive(std::uint32_t combat) = 0;
    [[nodiscard]] virtual ActorManagerEffectHandle CreateEffect(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t position, std::uint32_t mode,
        std::uint32_t flags) = 0;
    virtual void ResetTemporaryEffect(ActorManagerEffectHandle& handle) = 0;
    [[nodiscard]] virtual std::uint32_t CreateText(
        std::uint32_t literal) = 0;
    virtual void ReleaseText(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t RandomIndex(
        std::uint32_t count) = 0;
    [[nodiscard]] virtual bool EffectStillActive(
        std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t DispatchAddress(
        std::uint32_t router, std::uint32_t key,
        std::uint32_t payload, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchByte(
        std::uint32_t router, std::uint32_t key,
        std::uint8_t payload, std::uint32_t mode) = 0;
};

class ActorManagerCombatPrepare final {
public:
    static constexpr std::uint32_t kPosition = 0x84b1dcu;
    static constexpr std::uint32_t kEmptyText = 0x841964u;
    static constexpr std::uint32_t kCombatStartKey = 0x8b8b88u;

    [[nodiscard]] static std::uint32_t Handle(
        std::uint32_t manager, std::uint32_t command_address,
        IActorManagerCombatPrepareRuntime& runtime);
    [[nodiscard]] static std::uint32_t HandleTypeOne(
        std::uint32_t manager, std::uint32_t command_address,
        IActorManagerCombatPrepareRuntime& runtime);
};

}  // namespace pal4
