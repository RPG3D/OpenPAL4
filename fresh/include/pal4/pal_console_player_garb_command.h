#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPalConsolePlayerGarbRuntime {
public:
    virtual ~IPalConsolePlayerGarbRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::uint32_t index) = 0;
    virtual void AssignModelText(std::uint32_t buffer,
                                 std::string_view model) = 0;
    [[nodiscard]] virtual bool AllocateModelText(
        std::uint32_t buffer, std::uint32_t length,
        std::uint32_t preserve) = 0;
    virtual void WriteModelText(std::uint32_t buffer,
                                std::string_view model) = 0;
    [[nodiscard]] virtual bool NeedsModelReload(std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    virtual void ScriptOnRemove(std::uint32_t arena,
                                std::uint32_t player) = 0;
    virtual void ProcessAndCleanup(std::uint32_t player) = 0;
    virtual void SetStateFlag(std::uint32_t player,
                              std::uint32_t flag) = 0;
    virtual void ScriptOnAdd(std::uint32_t arena,
                             std::uint32_t player) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    virtual void SetActor(std::uint32_t control,
                          std::uint32_t player) = 0;
};

class PalConsolePlayerGarbCommand final {
public:
    // IDA 0x5bcfa0, playerGarb1 command.
    static void ApplyToAll(IPalConsolePlayerGarbRuntime& runtime);
};

}  // namespace pal4
