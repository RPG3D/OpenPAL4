#pragma once

#include <cstdint>

namespace pal4 {

class IScriptNpcEffectsRuntime {
public:
    virtual ~IScriptNpcEffectsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyString() = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t AttachEffect(
        std::uint32_t npc, std::uint32_t effect,
        std::int32_t argument) = 0;
    [[nodiscard]] virtual std::uint32_t DetachEffect(
        std::uint32_t npc) = 0;
};

class ScriptNpcEffects final {
public:
    // IDA 0x5d8820, giNpcAttachEffect.
    [[nodiscard]] static std::uint32_t Attach(
        std::uint32_t name_record, std::uint32_t effect_record,
        std::int32_t argument, IScriptNpcEffectsRuntime& runtime);
    // IDA 0x5d8870, giNpcDetachEffect.
    [[nodiscard]] static std::uint32_t Detach(
        std::uint32_t name_record, IScriptNpcEffectsRuntime& runtime);
};

}  // namespace pal4
