#include "pal4/script_round_times.h"

namespace pal4 {

void ScriptRoundTimes::Increment(IScriptRoundTimesRuntime& runtime) {
    const auto value = runtime.ReadWord(runtime.GlobalData() + 0x28u) + 1u;
    runtime.WriteWord(runtime.GlobalData() + 0x28u, value);
    runtime.SaveSettings(runtime.GlobalData());
}

}  // namespace pal4
