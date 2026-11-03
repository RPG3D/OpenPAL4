#include "pal4/script_gob_reset.h"

namespace pal4 {

std::uint32_t ScriptGobReset::ByName(
    const char* name, IScriptGobResetRuntime& runtime) {
    const auto object = runtime.FindObject(runtime.ObjectManager(),
                                           name ? name : "");
    return object == 0u ? 0u : runtime.InvokeReset(object);
}

}  // namespace pal4
