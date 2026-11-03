#pragma once

#include <cstdint>

namespace pal4 {

class IScriptActionHoldRuntime {
public:
    virtual ~IScriptActionHoldRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyStringAddress() = 0;
    [[nodiscard]] virtual std::uint32_t NpcManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindNpc(
        std::uint32_t manager, std::uint32_t name) = 0;
    virtual void ClearAnimationFlag(
        std::uint32_t component, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t UpdatePlayerEffects(
        std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateNpcEffects(
        std::uint32_t npc) = 0;
};

class ScriptActionHoldRelease final {
public:
    // IDA 0x5d55b0, giPlayerUnHoldAct.
    [[nodiscard]] static std::uint32_t Player(
        std::int32_t index, IScriptActionHoldRuntime& runtime);
    // IDA 0x5d55e0, giNpcUnHoldAct; payload is a script string object.
    [[nodiscard]] static std::uint32_t Npc(
        std::uint32_t payload, IScriptActionHoldRuntime& runtime);
};

}  // namespace pal4
