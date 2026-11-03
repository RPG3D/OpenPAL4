#pragma once

#include <cstdint>

namespace pal4 {

class IScriptMonsterEffectsRuntime {
public:
    virtual ~IScriptMonsterEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t MonsterManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindMonster(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t AttachEffect(
        std::uint32_t monster, std::uint32_t effect,
        std::int32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t DetachEffect(
        std::uint32_t monster) = 0;
};

class ScriptMonsterEffects final {
public:
    // IDA 0x5d88a0, giMstAttachEffect.
    [[nodiscard]] static std::uint32_t Attach(
        std::uint32_t name_record, std::uint32_t effect_record,
        std::int32_t argument, IScriptMonsterEffectsRuntime& runtime);
    // IDA 0x5d88f0, giMstDetachEffect.
    [[nodiscard]] static std::uint32_t Detach(
        std::uint32_t name_record, IScriptMonsterEffectsRuntime& runtime);
};

}  // namespace pal4
