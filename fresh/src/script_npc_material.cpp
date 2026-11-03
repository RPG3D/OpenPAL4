#include "pal4/script_npc_material.h"

namespace pal4 {

namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptNpcMaterialRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

std::uint32_t FindNpc(std::uint32_t payload,
                      IScriptNpcMaterialRuntime& runtime) {
    const auto name = ResolveName(payload, runtime);
    return runtime.FindNpc(runtime.NpcManager(), name);
}

}  // namespace

std::uint32_t ScriptNpcMaterial::Set(
    std::uint32_t npc_name_object, std::uint32_t material_name_object,
    IScriptNpcMaterialRuntime& runtime) {
    const auto npc = FindNpc(npc_name_object, runtime);
    return npc == 0u ? 0u :
        runtime.SetMaterial(npc, ResolveName(material_name_object, runtime));
}

std::uint32_t ScriptNpcMaterial::Apply(
    std::uint32_t npc_name_object, IScriptNpcMaterialRuntime& runtime) {
    const auto npc = FindNpc(npc_name_object, runtime);
    return npc == 0u ? 0u : runtime.ApplyMaterial(npc);
}

}  // namespace pal4
