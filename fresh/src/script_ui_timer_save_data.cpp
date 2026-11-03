#include "pal4/script_ui_timer_save_data.h"

namespace pal4 {

std::uint32_t ScriptUiTimerSaveData::Get(
    IScriptUiTimerSaveDataRuntime& runtime) {
    return runtime.GameDataEntry(runtime.Game(), 70u);
}

}  // namespace pal4
