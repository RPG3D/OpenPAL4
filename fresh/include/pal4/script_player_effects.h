#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPlayerEffectsRuntime {
public:
    virtual ~IScriptPlayerEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t AttachEffect(
        std::uint32_t player, std::uint32_t effect,
        std::int32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t DetachEffect(
        std::uint32_t player) = 0;
};

class ScriptPlayerEffects final {
public:
    // IDA 0x5d8920, giPlayerHookEffect.
    [[nodiscard]] static std::uint32_t Hook(
        std::int32_t player_index, std::uint32_t effect_record,
        std::int32_t argument, IScriptPlayerEffectsRuntime& runtime);
    // IDA 0x5d8960, giPlayerDetachEffect.
    [[nodiscard]] static std::uint32_t Detach(
        std::int32_t player_index, IScriptPlayerEffectsRuntime& runtime);
};

}  // namespace pal4
