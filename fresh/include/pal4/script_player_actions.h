#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPlayerActionsRuntime {
public:
    virtual ~IScriptPlayerActionsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t UpdatePlayerStates(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptOnRemove(
        std::uint32_t arena, std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptOnAdd(
        std::uint32_t arena, std::uint32_t player) = 0;
    virtual void SetStateFlag(std::uint32_t player,
                              std::uint32_t flag) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    [[nodiscard]] virtual std::uint32_t SetClumpProperties(
        std::uint32_t adapter, std::uint32_t clump,
        std::uint32_t first, std::uint32_t second,
        std::uint32_t third, std::uint32_t fourth) = 0;
    [[nodiscard]] virtual std::uint32_t SetClumpRenderCallback(
        std::uint32_t adapter, std::uint32_t clump) = 0;
};

class ScriptPlayerActions final {
public:
    // IDA 0x5d3a00, 0x5d3a20, 0x5d3a80 and 0x5d3ad0.
    [[nodiscard]] static std::uint32_t SetLeader(
        std::int32_t index, IScriptPlayerActionsRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVisible(
        std::int32_t index, bool visible, IScriptPlayerActionsRuntime& runtime);
    [[nodiscard]] static std::uint32_t AttachCollision(
        std::int32_t index, IScriptPlayerActionsRuntime& runtime);
    [[nodiscard]] static std::uint32_t DetachCollision(
        std::int32_t index, IScriptPlayerActionsRuntime& runtime);
};

}  // namespace pal4
