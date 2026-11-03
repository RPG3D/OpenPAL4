#include "pal4/script_night_time_query.h"

namespace pal4 {

bool ScriptNightTimeQuery::IsNightTime(
    IScriptNightTimeQueryRuntime& runtime) {
    const auto extracted = runtime.ExtractTail(runtime.ArenaManager() + 0xe0u,
                                               1u);
    bool result;
    try {
        const auto yes = runtime.InitializeString("y");
        try {
            result = runtime.CompareCaseInsensitive(extracted, yes) == 0;
        } catch (...) {
            runtime.DestroyString(yes);
            throw;
        }
        runtime.DestroyString(yes);
    } catch (...) {
        runtime.DestroyString(extracted);
        throw;
    }
    runtime.DestroyString(extracted);
    return result;
}

}  // namespace pal4
