#include "pal4/script_object_research.h"

namespace pal4 {

std::uint32_t ScriptObjectResearch::Set(
    std::uint32_t name_payload, std::uint32_t enabled,
    IScriptObjectResearchRuntime& runtime) {
    auto name = runtime.ReadWord(name_payload + 4u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    const auto object = runtime.FindByName(runtime.ObjectManager(), name);
    if (object != 0u) runtime.WriteWord(object + 0x18cu, enabled & 0xffu);
    return object;
}

}  // namespace pal4
