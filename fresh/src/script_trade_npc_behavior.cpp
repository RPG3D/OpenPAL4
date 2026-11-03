#include "pal4/script_trade_npc_behavior.h"

namespace pal4 {

std::uint32_t ScriptTradeSlots::Get(
    std::int32_t index, IScriptTradeSlotRuntime& runtime) {
    const auto trade = runtime.TradeSystem();
    return runtime.ReadWord(trade + static_cast<std::uint32_t>(index) * 4u + 4u);
}

std::uint32_t ScriptTradeSlots::Set(
    std::int32_t index, std::uint32_t value,
    IScriptTradeSlotRuntime& runtime) {
    const auto trade = runtime.TradeSystem();
    runtime.WriteWord(trade + static_cast<std::uint32_t>(index) * 4u + 4u,
                      value);
    return trade;
}

std::uint32_t ScriptNpcBehavior::Pause(
    const char* name, IScriptNpcBehaviorRuntime& runtime) {
    const auto npc = runtime.FindNpc(runtime.NpcManager(), name ? name : "");
    if (npc == 0u) return 0u;
    runtime.CleanupEventsAndScripts(npc, 0u);
    const auto component = npc + 0x170u;
    const auto result = runtime.SetActionCode(component, "C01");
    return result != 0u ? result : runtime.SetActionCode(component, "Z01");
}

void ScriptNpcBehavior::Resume(
    const char* name, IScriptNpcBehaviorRuntime& runtime) {
    const std::string_view npc_name = name ? name : "";
    const auto npc = runtime.FindNpc(runtime.NpcManager(), npc_name);
    if (npc == 0u) return;
    const auto info = runtime.FindNpcInfo(runtime.NpcManager(), npc_name);
    if (info != 0u) runtime.ProcessEventsAndClear(npc, info, 0u);
}

}  // namespace pal4
