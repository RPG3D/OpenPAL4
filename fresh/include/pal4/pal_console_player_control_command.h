#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsolePlayerControlRuntime {
public:
    virtual ~IPalConsolePlayerControlRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::int32_t PlayerId(
        std::uint32_t manager, std::string_view name) = 0;
    virtual void UpdatePlayerStates(std::uint32_t manager,
                                    std::int32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ScriptOnRemove(std::uint32_t arena,
                                std::uint32_t player) = 0;
    virtual void SetStateFlag(std::uint32_t player,
                              std::uint32_t flag) = 0;
    virtual void ScriptOnAdd(std::uint32_t arena,
                             std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void SetActor(std::uint32_t control,
                          std::uint32_t player) = 0;
    [[nodiscard]] virtual std::int32_t ResetAndSync(
        std::uint32_t control, std::uint32_t enabled) = 0;
    [[nodiscard]] virtual std::uint32_t Console() = 0;
    [[nodiscard]] virtual std::int32_t AddOutputLine(
        std::uint32_t console, std::string_view format) = 0;
};

class PalConsolePlayerControlCommand final {
public:
    // IDA 0x5bc7a0. The decompiler's extra parameters are EBP/ST0 artifacts.
    [[nodiscard]] static std::int32_t Execute(
        const char* command, IPalConsolePlayerControlRuntime& runtime);
};

}  // namespace pal4
