#include "pal4/script_npc_lifecycle.h"

namespace pal4 {

std::uint32_t ScriptNpcLifecycle::Create(
    const char* name, const char* resource,
    std::array<std::uint32_t, 3> position,
    IScriptNpcLifecycleRuntime& runtime) {
    const std::string_view npc_name = name ? name : "";
    const auto existing = runtime.FindNpc(runtime.NpcManager(), npc_name);
    if (existing != 0u) return existing;

    const auto info_manager = runtime.NpcManager();
    const auto info = runtime.CreateOrGetInfo(
        info_manager, npc_name, resource ? resource : "", position,
        {0u, 0u, 0u});
    const auto resource_text = runtime.ReadWord(info + 12u);
    const auto name_text = runtime.ReadWord(info);
    runtime.WriteWord(info + 72u, 1u);
    const auto manager = runtime.NpcManager();
    const auto npc = runtime.CreateAndLoadNpc(
        manager, name_text, resource_text, info + 36u, info + 48u,
        info, 1u);
    runtime.SetStateFlag(npc, 1u);
    return runtime.EnterNpcScript(runtime.ArenaManager(), npc);
}

std::uint32_t ScriptNpcLifecycle::Delete(
    const char* name, IScriptNpcLifecycleRuntime& runtime) {
    const auto npc = runtime.FindNpc(runtime.NpcManager(), name ? name : "");
    if (npc == 0u) return 0u;

    (void)runtime.LeaveNpcScript(runtime.ArenaManager(), npc);
    const auto info = runtime.ReadWord(npc + 0x654u);
    const auto first_name = runtime.ReadWord(info);
    runtime.DeleteNpc(runtime.NpcManager(), first_name);
    const auto second_name = runtime.ReadWord(info);
    return runtime.RemoveNpcInfo(runtime.NpcManager(), second_name);
}

}  // namespace pal4
