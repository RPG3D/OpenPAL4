#pragma once

#include <cstdint>

namespace pal4 {

class IScriptMovieFlagRuntime {
public:
    virtual ~IScriptMovieFlagRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GlobalData() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SaveDialogSettings(std::uint32_t global_data) = 0;
};

class IScriptQuestCompletionRuntime {
public:
    virtual ~IScriptQuestCompletionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GetGameDataEntry(
        std::uint32_t game, std::uint32_t index) = 0;
    virtual void SetGameDataEntry(std::uint32_t game,
                                  std::uint32_t index,
                                  std::uint32_t value) = 0;
};

class IScriptGlobalCallbackRuntime : public IScriptMovieFlagRuntime,
                                     public IScriptQuestCompletionRuntime {};

class ScriptGlobalCallbacks final {
public:
    // IDA 0x5d5280, giOpenMovieFlag.
    static void OpenMovieFlag(std::uint32_t bit,
                              IScriptMovieFlagRuntime& runtime);
    // IDA 0x5d52c0, giAddQuestComplatePercentage.
    static void AddQuestCompletionPercentage(
        std::int32_t delta, IScriptQuestCompletionRuntime& runtime);
};

}  // namespace pal4
