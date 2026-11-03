#include "pal4/script_gob_position.h"

namespace pal4 {

std::uint32_t ScriptGobPosition::Set(
    std::uint32_t name_object, float x, float y, float z,
    IScriptGobPositionRuntime& runtime) {
    auto name = runtime.ReadWord(name_object + 4u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    const auto object = runtime.FindObject(runtime.ObjectManager(), name);
    if (object == 0u) return 0u;
    return runtime.SetPosition(object, {x, y, z}, 0u);
}

}  // namespace pal4
