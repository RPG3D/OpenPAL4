#pragma once

#include <cstdint>

namespace pal4 {

class ICombatGameEventCallbackRuntime {
public:
    virtual ~ICombatGameEventCallbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromFacet(
        std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateStat(
        std::uint32_t object, std::uint32_t stat,
        std::uint32_t increment) = 0;
};

class CombatGameEventCallbacks final {
public:
    // IDA 0x574600/0x574650/0x5746a0/0x5746f0.
    [[nodiscard]] static std::uint32_t OnEvent21(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnEvent7(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnEvent6(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnEvent8(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    // IDA 0x574740/0x574790/0x5747e0.
    [[nodiscard]] static std::uint32_t OnFlee(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnGuard(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnDeath(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    // IDA 0x574800/0x574830/0x574860/0x574890.
    [[nodiscard]] static std::uint32_t OnRoundStart(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnBattleStart(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnSpecial(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnEscape(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    // IDA 0x574b10/0x574b40/0x574b60/0x574b90.
    [[nodiscard]] static std::uint32_t OnPlayerDeath(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnDodge(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnAttack(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnItemUse(
        std::uint32_t object, std::uint32_t event,
        ICombatGameEventCallbackRuntime& runtime);
};

}  // namespace pal4
