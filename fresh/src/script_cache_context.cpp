#include "pal4/script_cache_context.h"

namespace pal4 {

std::int32_t ScriptCacheContext::ClearExceptCurrent(
    IScriptCacheContextRuntime& runtime) {
    const auto count = runtime.EntryCount(runtime.ScriptExecutor() + 0x54u);
    if (count <= 1) return count;
    return runtime.RemoveExceptCurrent(runtime.ScriptExecutor() + 0x54u);
}

}  // namespace pal4
