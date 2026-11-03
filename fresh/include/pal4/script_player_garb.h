#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IScriptPlayerGarbRuntime {
public:
    virtual ~IScriptPlayerGarbRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
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

class ScriptPlayerGarb final {
public:
    // IDA 0x5d66d0, giAllPlayerGarb2.
    static void ApplySecondToAll(IScriptPlayerGarbRuntime& runtime);
    // IDA 0x5d68e0, giPlayerGarb2.
    static void ApplySecond(std::int32_t index,
                            IScriptPlayerGarbRuntime& runtime);
    // IDA 0x5d6aa0, giPlayerGarb1.
    static void ApplyFirst(std::int32_t index,
                           IScriptPlayerGarbRuntime& runtime);

private:
    static void ApplyKnownPlayer(std::uint32_t player,
                                 std::int32_t index, bool second,
                                 IScriptPlayerGarbRuntime& runtime);
};

}  // namespace pal4
