#pragma once

#include <cstdint>
#include <functional>

namespace pal4 {

class IScriptShadowRuntime {
public:
    using PlayerCallback = std::function<std::uint32_t(
        std::uint32_t player, std::uint32_t enabled)>;

    virtual ~IScriptShadowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t ForAllPlayers(
        std::uint32_t manager, const PlayerCallback& callback,
        std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void SetPlayerShadow(std::uint32_t player,
                                 std::uint32_t enabled,
                                 std::uint32_t arena_state) = 0;
};

class ScriptShadowControl final {
public:
    // IDA 0x5d8590, giEnableShadow; callback at 0x5d85c0.
    [[nodiscard]] static std::uint32_t Enable(
        std::uint8_t enabled, IScriptShadowRuntime& runtime);
    [[nodiscard]] static std::uint32_t ApplyToPlayer(
        std::uint32_t player, std::uint32_t enabled,
        IScriptShadowRuntime& runtime);
};

}  // namespace pal4
