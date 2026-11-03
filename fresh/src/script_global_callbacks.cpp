#include "pal4/script_global_callbacks.h"
#include "pal4/movie_flag_value.h"

namespace pal4 {

void ScriptGlobalCallbacks::OpenMovieFlag(
    std::uint32_t bit, IScriptMovieFlagRuntime& runtime) {
    const auto global = runtime.GlobalData();
    if (bit >= 32u) return;
    const auto address = global + 0x18u + 4u * (bit >> 5u);
    runtime.WriteWord(address,*OpenMovieFlagValue(runtime.ReadWord(address),bit));
    runtime.SaveDialogSettings(runtime.GlobalData());
}

void ScriptGlobalCallbacks::AddQuestCompletionPercentage(
    std::int32_t delta, IScriptQuestCompletionRuntime& runtime) {
    const auto value = runtime.GetGameDataEntry(runtime.GameInstance(), 7u) +
                       static_cast<std::uint32_t>(delta);
    runtime.SetGameDataEntry(runtime.GameInstance(), 71u, value);
}

}  // namespace pal4
