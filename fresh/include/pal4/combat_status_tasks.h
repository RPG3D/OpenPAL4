#pragma once

#include <cstdint>

namespace pal4 {

class ICombatStatusActorRuntime {
public:
    virtual ~ICombatStatusActorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void CallActor(std::uint32_t actor, std::uint32_t vtable_offset,
                           std::uint32_t argument) = 0;
};
class ICombatStatusTaskRuntime : public ICombatStatusActorRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void SetStateOne(std::uint32_t actor) = 0;
};

class CombatStatusTasks final {
public:
    // IDA 0x558670, 0x5587f0, 0x558810 and 0x5589a0.
    [[nodiscard]] static std::uint8_t Defend(
        std::uint32_t task, ICombatStatusTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Flee(
        std::uint32_t task, ICombatStatusTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetFleeTarget(
        std::uint32_t task, ICombatStatusTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Dead(
        std::uint32_t task, ICombatStatusTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t FendOff(
        std::uint32_t task, ICombatStatusActorRuntime& runtime);
};

}  // namespace pal4
