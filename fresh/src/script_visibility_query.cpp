#include "pal4/script_visibility_query.h"

namespace pal4 {

namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptVisibilityQueryRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

}  // namespace

bool ScriptVisibilityQuery::Object(
    std::uint32_t name_payload, IScriptVisibilityQueryRuntime& runtime) {
    const auto name = ResolveName(name_payload, runtime);
    const auto object = runtime.FindObject(runtime.ObjectManager(), name);
    return object != 0u && runtime.ReadWord(object + 0x184u) != 0u;
}

bool ScriptVisibilityQuery::Monster(
    std::uint32_t name_payload, IScriptVisibilityQueryRuntime& runtime) {
    const auto name = ResolveName(name_payload, runtime);
    const auto monster = runtime.FindMonster(runtime.MonsterManager(), name);
    return monster != 0u && runtime.ReadWord(monster + 0x1f0u) != 0u;
}

}  // namespace pal4
